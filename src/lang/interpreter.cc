#include "ankh/lang/expr_result.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <cerrno>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include <ankh/def.h>
#include <ankh/log.h>

#include <ankh/lang/interpreter.h>
#include <ankh/lang/parser.h>

#include <ankh/lang/token.h>
#include <ankh/lang/expr.h>
#include <ankh/lang/exceptions.h>

#include <ankh/lang/types/array.h>
#include <ankh/lang/types/dictionary.h>
#include <ankh/lang/builtins.h>

#define ANKH_DEFINE_BUILTIN(name, arity, type) do {\
    functions_[(name)] = make_callable<type<ExprResult, Interpreter>>(this, global_, (name), (arity));\
    ANKH_VERIFY(global_->declare((name), functions_[(name)].get()));\
} while(0)

struct ReturnException
    : public std::runtime_error
{
    explicit ReturnException(ankh::lang::ExprResult result)
        : std::runtime_error(""), result(std::move(result)) {}

    ankh::lang::ExprResult result;
};

struct BreakException
    : public std::runtime_error
{
    BreakException() 
        : std::runtime_error("") {}
};

static bool operands_are(ankh::lang::ExprResultType type, std::initializer_list<ankh::lang::ExprResult> elems) noexcept
{
    return std::all_of(elems.begin(), elems.end(), [=](const ankh::lang::ExprResult& result) {
        return type == result.type;
    });
}

static ankh::lang::Number to_num(const ankh::lang::LiteralExpression *expr)
{
    char *end;

    ankh::lang::Number n = std::strtod(expr->literal.str.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    const std::string errno_msg(std::strerror(errno));

    ankh::lang::panic<ankh::lang::InterpretationException>(expr->literal, "runtime error: '{}' could not be turned into a number because '{}'", expr->stringify(), errno_msg);
}

static bool is_integer(ankh::lang::Number n) noexcept
{
    double intpart;
    return modf(n, &intpart) == 0.0;
}

static ankh::lang::ExprResult negate(const ankh::lang::Token& marker, const ankh::lang::ExprResult& result)
{
    if (result.type == ankh::lang::ExprResultType::RT_NUMBER) {
        return -1 * result.n;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unary operator(-) expects a number, not a {}", ankh::lang::expr_result_type_str(result.type));
}

static ankh::lang::ExprResult invert(const ankh::lang::Token& marker, const ankh::lang::ExprResult& result)
{
    if (result.type == ankh::lang::ExprResultType::RT_BOOL) {
        return !(result.b);
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: operator(!) expects a boolean expression, not a {}", ankh::lang::expr_result_type_str(result.type));
}

static ankh::lang::ExprResult eqeq(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n == right.n;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str == right.str;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_BOOL, {left, right})) {
        return left.b == right.b;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_NIL, {left, right})) {
        return true;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator(==) with LHS as {} and RHS as {}", ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

template <class BinaryOperation>
static ankh::lang::ExprResult 
arithmetic(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, BinaryOperation op)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return op(left.n, right.n);
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator({}) with LHS as {} and RHS as {}", marker.str, ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

static ankh::lang::ExprResult division(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        if (right.n == 0) {
            ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: division by zero");
        }
        return left.n / right.n;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator({}) with LHS as {} and RHS as {}", marker.str, ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static ankh::lang::ExprResult plus(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n + right.n;
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str + right.str;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator(+) with LHS as {} and RHS as {}", ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

template <class Compare>
static ankh::lang::ExprResult compare(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(ankh::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return cmp(left.n, right.n);
    }

    if (operands_are(ankh::lang::ExprResultType::RT_STRING, {left, right})) {
        return cmp(left.str, right.str);
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator({}) with LHS as {} and RHS as {}", marker.str, ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

template <class Compare>
static ankh::lang::ExprResult logical(const ankh::lang::Token& marker, const ankh::lang::ExprResult& left, const ankh::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(ankh::lang::ExprResultType::RT_BOOL, {left, right})) {
        return cmp(left.b, right.b);
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: unknown overload of operator({}) with LHS as {} and RHS as {}", marker.str, ankh::lang::expr_result_type_str(left.type), ankh::lang::expr_result_type_str(right.type));
}

static bool truthy(const ankh::lang::Token& marker, const ankh::lang::ExprResult& result) noexcept
{
    if (result.type == ankh::lang::ExprResultType::RT_BOOL) {
        return result.b;
    }

    ankh::lang::panic<ankh::lang::InterpretationException>(marker, "runtime error: '{}' is not a boolean expression", result.stringify());
}


ankh::lang::Interpreter::Interpreter()
    : current_env_(make_env<ExprResult>())
    , global_(current_env_) 
{
    ANKH_DEFINE_BUILTIN("print", 1, PrintFn);
    ANKH_DEFINE_BUILTIN("exit", 1, ExitFn);
    ANKH_DEFINE_BUILTIN("len", 1, LengthFn);
    ANKH_DEFINE_BUILTIN("int", 1, IntFn);
    ANKH_DEFINE_BUILTIN("append", 2, AppendFn);
    ANKH_DEFINE_BUILTIN("str", 1, StrFn);
    ANKH_DEFINE_BUILTIN("keys", 1, KeysFn);
}

void ankh::lang::Interpreter::interpret(Program&& program)
{
    programs_.push_back(std::forward<Program>(program));

    const auto& statements = programs_.back().statements;
    for (const auto& stmt : statements) {
#ifndef NDEBUG
        ANKH_DEBUG("{}", stmt->stringify());
#endif
        execute(stmt);
    }
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////// BUILTINS //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ankh::lang::Interpreter::print(const std::vector<ExprResult>& args) const
{
    const std::string stringy = args[0].stringify();
    std::puts(stringy.c_str());
}

void ankh::lang::Interpreter::exit(const std::vector<ExprResult>& args) const
{
    const ExprResult& result = args[0];

    if (result.type != ExprResultType::RT_NUMBER) {
        builtin_panic<InterpretationException>("exit", "{} is not a viable argument type", expr_result_type_str(result.type));
    }

    if (!is_integer(result.n)) {
        builtin_panic<InterpretationException>("exit", "'{}' is not an integer", result.n);
    }
    
    std::exit(result.n);
}

void ankh::lang::Interpreter::length(const std::vector<ExprResult>& args) const
{
    const ExprResult& result = args[0];
    if (result.type == ExprResultType::RT_ARRAY) {
        throw ReturnException(static_cast<Number>(result.array.size()));
    }
    if (result.type == ExprResultType::RT_DICT) {
        throw ReturnException(static_cast<Number>(result.dict.size()));
    }
    if (result.type == ExprResultType::RT_STRING) {
        throw ReturnException(static_cast<Number>(result.str.size()));
    }

    builtin_panic<InterpretationException>("length", "{} is not a viable argument type", expr_result_type_str(result.type));
}

void ankh::lang::Interpreter::cast_int(const std::vector<ExprResult>& args) const
{
    const ExprResult& result = args[0];
    if (result.type == ExprResultType::RT_NUMBER) {
        Number e = static_cast<std::int64_t>(result.n);
        ANKH_DEBUG("cast_int(), from {} to {}", result.n, e);
        throw ReturnException(e);
    }

    if (result.type == ExprResultType::RT_BOOL) {
        Number e = result.b ? 1 : 0;
        throw ReturnException(e);
    }

    builtin_panic<InterpretationException>("int", "{} is not a viable argument type", expr_result_type_str(result.type));
}

void ankh::lang::Interpreter::str(const std::vector<ExprResult>& args) const
{
    throw ReturnException(args[0].stringify());
}

void ankh::lang::Interpreter::append(const std::vector<ExprResult>& args) const
{
    ExprResult container = args[0];
    const ExprResult& value = args[1];

    if (container.type == ExprResultType::RT_STRING) {
        try {
            str({ value });
        } catch (const ReturnException& e) {
            container.str += e.result.str;
        }

        throw ReturnException(container);
    }

    if (container.type == ExprResultType::RT_ARRAY) {
        container.array.append(value);
        throw ReturnException(container);
    }

    builtin_panic<InterpretationException>("append", "{} is not a viable argument type", expr_result_type_str(container.type));
}

void ankh::lang::Interpreter::keys(const std::vector<ExprResult>& args) const
{
    ExprResult container = args[0];
    if (container.type == ExprResultType::RT_DICT) {
        Array<ExprResult> arr;
        for (const auto& x : container.dict) {
            arr.append(x.key);
        }
        throw ReturnException(arr);
    }

    builtin_panic<InterpretationException>("keys", "{} is not a viable argument type", expr_result_type_str(container.type));
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////// END BUILTINS //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ankh::lang::ExprResult ankh::lang::Interpreter::visit(BinaryExpression *expr)
{
    const ExprResult left  = evaluate(expr->left);
    const ExprResult right = evaluate(expr->right);

    switch (expr->op.type) {
    case TokenType::EQEQ:
        return eqeq(expr->op, left, right);
    case TokenType::NEQ:
        return invert(expr->op, eqeq(expr->op, left, right));
    case TokenType::GT:
        return compare(expr->op, left, right, std::greater<>{});
    case TokenType::GTE:
        return compare(expr->op, left, right, std::greater_equal<>{});
    case TokenType::LT:
        return compare(expr->op, left, right, std::less<>{});
    case TokenType::LTE:
        return compare(expr->op, left, right, std::less_equal<>{});
    case TokenType::MINUS:
        return arithmetic(expr->op, left, right, std::minus<>{});
    case TokenType::PLUS:
        return plus(expr->op, left, right);
    case TokenType::STAR:
        return arithmetic(expr->op, left, right, std::multiplies<>{});
    case TokenType::AND:
        return logical(expr->op, left, right, std::logical_and<>{});
    case TokenType::OR:
        return logical(expr->op, left, right, std::logical_or<>{});
    case TokenType::FSLASH:
        return division(expr->op, left, right);
    default:
        panic<InterpretationException>(expr->op, "runtime error: unknown binary operator '{}'", expr->op.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(UnaryExpression *expr)
{
    const ExprResult result = evaluate(expr->right);
    switch (expr->op.type) {
    case TokenType::MINUS:
        return negate(expr->op, result);
    case TokenType::BANG:
        return invert(expr->op, result);
    default:
        panic<InterpretationException>(expr->op, "runtime error: unknown unary operator '{}'", expr->op.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(LiteralExpression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return to_num(expr);
    case TokenType::STRING:
        return expr->literal.str;
    case TokenType::ANKH_TRUE:
        return true;
    case TokenType::ANKH_FALSE:
        return false;
    case TokenType::NIL:
        return {};
    default:
        panic<InterpretationException>(expr->literal, "runtime error: unknown literal expression '{}''", expr->literal.str);
    }
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ParenExpression *expr)
{
    return evaluate(expr->expr);
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(IdentifierExpression *expr)
{
    ANKH_DEBUG("evaluating identifier expression '{}'", expr->name.str);

    auto possible_value = current_env_->value(expr->name.str);
    if (possible_value.has_value()) {
        return possible_value.value();
    }

    panic<InterpretationException>(expr->name, "runtime error: identifier '{}' not defined", expr->name.str);
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(CallExpression *expr)
{
    ANKH_DEBUG("evaluating call expression");

    const ExprResult callee = evaluate(expr->callee);
    if (callee.type != ExprResultType::RT_CALLABLE) {
        panic<InterpretationException>(expr->marker, "runtime error: only functions and classes are callable");
    }

    Callable *callable = callee.callable;
    const std::string name = callable->name();
    
    if (expr->args.size() != callable->arity()) {
        panic<InterpretationException>(expr->marker, "runtime error: expected {} arguments to function '{}' instead of {}", callable->arity(), name, expr->args.size());
    }

    ANKH_DEBUG("function '{}' with matching arity '{}' found", name, expr->args.size());

    try {
        callable->invoke(expr->args);
        return {};
    } catch (const ReturnException& e) {
        return e.result;
    }

    ANKH_FATAL("callables should always return");
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(LambdaExpression *expr)
{
    const std::string& name = expr->generated_name;
    if (functions_.count(name) > 0) {
        ANKH_FATAL("lambda function generated name '{}' is duplicated", name);
    }

    CallablePtr callable = make_callable<Lambda<ExprResult, Interpreter>>(this, expr, current_env_);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        panic<InterpretationException>(expr->marker, "runtime error: '{}' is already defined", name);
    }

    ANKH_DEBUG("function '{}' added to scope {}", name, current_env_->scope());

    return result;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::CommandExpression *expr)
{
    ANKH_DEBUG("executing {}", expr->cmd.str);

    // TODO: popen() uses the underlying shell to invoke commands
    // This limits the language from being used as a shell itself
    // Come back and explore if that's something we want to consider doing
    std::FILE *fp = popen(expr->cmd.str.c_str(), "r");
    if (fp == nullptr) {
        ANKH_FATAL("popen: unable to launch {}", expr->cmd.str);
    }

    char buf[512];
    std::string output;
    while (std::fread(buf, sizeof(buf[0]), sizeof(buf), fp) > 0) {
        output += buf;
    }
    fclose(fp);

    return output;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ArrayExpression *expr)
{
    Array<ExprResult> array;
    for (const auto& e : expr->elems) {
        array.append(evaluate(e));
    }

    return array;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::IndexExpression *expr)
{
    const ExprResult indexee = evaluate(expr->indexee);
    if (indexee.type != ExprResultType::RT_ARRAY 
        && indexee.type != ExprResultType::RT_DICT 
        && indexee.type != ExprResultType::RT_STRING) {
        panic<InterpretationException>(expr->marker, "runtime error: lookup expects string, array, or dict operand");
    }

    const ExprResult index = evaluate(expr->index);
    if (index.type == ExprResultType::RT_NUMBER) {
        if (!is_integer(index.n)) {
            panic<InterpretationException>(expr->marker, "runtime error: index must be an integral numeric expression");
        }

        if (indexee.type == ExprResultType::RT_ARRAY) {
            if (index.n >= indexee.array.size()) {
                panic<InterpretationException>(expr->marker, "runtime error: index {} must be less than array size {}", index.n, indexee.array.size());
            }

            return indexee.array[index.n];
        }

        if (indexee.type == ExprResultType::RT_STRING) {
            if (index.n >= indexee.str.size()) {
                panic<InterpretationException>(expr->marker, "runtime error: index {} must be less than string length {}", index.n, indexee.str.size());
            }
            
            return std::string{ indexee.str[index.n] };
        }

        panic<InterpretationException>(expr->marker, "runtime error: operand must be an array or string for a numeric index");
    }

    if (index.type == ExprResultType::RT_STRING) {
        if (indexee.type != ExprResultType::RT_DICT) {
            panic<InterpretationException>(expr->marker, "runtime error: operand must be a dict for a string index");
        }

        if (auto possible_value = indexee.dict.value(index.str); possible_value.has_value()) {
            return possible_value->value;
        }

        return {};
    }

    panic<InterpretationException>(expr->marker, "runtime error: '{}' is not a valid lookup expression", index.stringify());
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(SliceExpression *expr)
{
    ExprResult indexee = evaluate(expr->indexee);
    if (indexee.type != ExprResultType::RT_ARRAY) {
        panic<InterpretationException>(expr->marker, "runtime error: slices are only available on arrays, not {}", expr_result_type_str(indexee.type));
    }

    auto assert_is_positive_integer = [&](const ExpressionPtr& e) -> ExprResult {
        ExprResult result = evaluate(e);
        if (result.type != ExprResultType::RT_NUMBER || !is_integer(result.n)) {
            panic<InterpretationException>(expr->marker, "runtime error: slice indexes can only be integers, not {}", expr_result_type_str(result.type));
        }
        if (result.n < 0) {
            panic<InterpretationException>(expr->marker, "runtime error: slice indexes can only be positive, not {}", expr_result_type_str(result.type));
        }

        return result;
    };

    const size_t begin_index = expr->begin ? assert_is_positive_integer(expr->begin).n : 0;
    const size_t end_index   = expr->end   ? assert_is_positive_integer(expr->end).n   : indexee.array.size();
    if (end_index > indexee.array.size()) {
        panic<InterpretationException>(expr->marker, "runtime error: slice index {} out of range", end_index);
    }

    Array<ExprResult> result;
    for (size_t i = begin_index; i < end_index; ++i) {
        result.append(indexee.array[i]);
    }

    return result;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::DictionaryExpression *expr)
{
    Dictionary<ExprResult> dict;
    for (const auto& [key, value] : expr->entries) {
        const ExprResult& key_result = evaluate(key);
        if (key_result.type != ExprResultType::RT_STRING) {
            panic<InterpretationException>(expr->marker, "runtime error: expression key '{}' does not evaluate to a string", key->stringify());
        }
        dict.insert(key_result, evaluate(value));
    }

    return dict;
}

ankh::lang::ExprResult ankh::lang::Interpreter::visit(ankh::lang::StringExpression *expr)
{
    return substitute(expr);
}

void ankh::lang::Interpreter::visit(ExpressionStatement *stmt)
{
    ANKH_DEBUG("executing expression statement");
    const ExprResult result = evaluate(stmt->expr);
    print({ result });
}

void ankh::lang::Interpreter::visit(VariableDeclaration *stmt)
{
    if (current_env_->contains(stmt->name.str)) {
        panic<InterpretationException>(stmt->name, "runtime error: '{}' is already declared in this scope", stmt->name.str);
    }

    const ExprResult result = evaluate(stmt->initializer);

    ANKH_DEBUG("DECLARATION '{}' = '{}'", stmt->name.str, result.stringify());

    if (!current_env_->declare(stmt->name.str, result)) {
        panic<InterpretationException>(stmt->name, "runtime error: '{}' is already defined", stmt->name.str);
    }

    if (stmt->storage_class == StorageClass::EXPORT) {
        const std::string result_str = result.stringify();
        if (setenv(stmt->name.str.c_str(), result_str.c_str(), 1) == -1) {
            const std::string errno_msg(std::strerror(errno));
            panic<InterpretationException>(stmt->name, "runtime error: '{}' could not be exported due to {}", stmt->name.str, errno_msg);
        }
    }
}

void ankh::lang::Interpreter::visit(AssignmentStatement *stmt)
{
    const ExprResult result = evaluate(stmt->initializer);
    if (!current_env_->assign(stmt->name.str, result)) {
        panic<InterpretationException>(stmt->name, "runtime error: '{}' is not defined", stmt->name.str);
    }
}

void ankh::lang::Interpreter::visit(CompoundAssignment* stmt)
{
    auto possible_target = current_env_->value(stmt->target.str);
    if (!possible_target) {
        panic<InterpretationException>(stmt->target, "runtime error: '{}' is not defined", stmt->target.str);
    }

    ExprResult target = possible_target.value();

    ExprResult value;
    if (stmt->op.str == "+=") {
        value = plus(stmt->op, target, evaluate(stmt->value));
    } else if (stmt->op.str == "-=") {
        value = arithmetic(stmt->op, target, evaluate(stmt->value), std::minus<>{});
    } else if (stmt->op.str == "*=") {
        value = arithmetic(stmt->op, target, evaluate(stmt->value), std::multiplies<>{});
    } else if (stmt->op.str == "/=") {
        value = division(stmt->op, target, evaluate(stmt->value));
    } else {
        panic<InterpretationException>(stmt->op, "runtime error: '{}' is not a valid compound assignment operation", stmt->op.str);
    }

    if (!current_env_->assign(stmt->target.str, value)) {
        panic<InterpretationException>(stmt->target, "runtime error: unable to assign the result of the compound assignment");
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::IncOrDecIdentifierStatement* stmt)
{
    ExprResult rhs = evaluate(stmt->expr);
    
    ExprResult value;
    if (stmt->op.str == "++") {
        value = plus(stmt->op, rhs, 1.0);
    }
    else if (stmt->op.str == "--") {
        value = arithmetic(stmt->op, rhs, 1.0, std::minus<>{});
    }
    else {
        // this shouldn't happen since the parser validates the token is one of the above
        // but those are famous last words ;)
        ANKH_FATAL("'{}' is not a valid increment or decrement operation", stmt->op.str);
    }

    IdentifierExpression* expr = static_cast<IdentifierExpression*>(stmt->expr.get());
    if (!current_env_->assign(expr->name.str, value)) {
        ANKH_FATAL("{}:{}, unable to assign '{}'", expr->name.str);
    }
}

void ankh::lang::Interpreter::visit(BlockStatement *stmt)
{
    execute_block(stmt, current_env_);
}

void ankh::lang::Interpreter::execute_block(const BlockStatement *stmt, EnvironmentPtr<ExprResult> environment)
{
    ScopeGuard block_scope(this, environment);
    for (const StatementPtr& statement : stmt->statements) {
        execute(statement);
    }
}

void ankh::lang::Interpreter::visit(IfStatement *stmt)
{
    const ExprResult result = evaluate(stmt->condition);
    if (truthy(stmt->marker, result)) {
        execute(stmt->then_block);
    } else if (stmt->else_block != nullptr) {
        execute(stmt->else_block);
    }
}

void ankh::lang::Interpreter::visit(WhileStatement *stmt)
{
    while (truthy(stmt->marker, evaluate(stmt->condition))) {
        try {
            execute(stmt->body);
        } catch (const BreakException&) {
            return;
        }
    }
}

void ankh::lang::Interpreter::visit(ForStatement *stmt)
{
    ScopeGuard for_scope(this, current_env_);
    
    if (stmt->init) {
        execute(stmt->init);
    }

    while (stmt->condition ? truthy(stmt->marker, evaluate(stmt->condition)) : true) {
        try {
            execute(stmt->body);
        } catch (const BreakException&) {
            return;
        }

        if (stmt->mutator) {
            execute(stmt->mutator);
        }
    }
}

void ankh::lang::Interpreter::visit(ankh::lang::BreakStatement *stmt)
{
    ANKH_UNUSED(stmt);
    
    throw BreakException();
}

void ankh::lang::Interpreter::visit(ankh::lang::FunctionDeclaration *stmt)
{
    declare_function(stmt, current_env_);
}

void ankh::lang::Interpreter::declare_function(FunctionDeclaration *decl, EnvironmentPtr<ExprResult> env)
{
    ANKH_DEBUG("evaluating function declaration of '{}'", decl->name.str);

    const std::string& name = decl->name.str;
    if (functions_.count(name) > 0) {
        panic<InterpretationException>(decl->name, "runtime error: function '{}' is already declared", name);
    }

    CallablePtr callable = make_callable<Function<ExprResult, Interpreter>>(this, decl, env);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!global_->declare(name, result)) {
        panic<InterpretationException>(decl->name, "'{}' is already defined", name);
    }

    ANKH_DEBUG("function '{}' added to scope {}", name, global_->scope());
}

void ankh::lang::Interpreter::visit(ReturnStatement *stmt)
{
    ANKH_DEBUG("evaluating return statement");

    const ExprResult result = stmt->expr ? evaluate(stmt->expr) : ExprResult{};

    throw ReturnException(result);
}

ankh::lang::ExprResult ankh::lang::Interpreter::evaluate(const ExpressionPtr& expr)
{
    return expr->accept(this);
}

std::string ankh::lang::Interpreter::substitute(const StringExpression *expr)
{
    std::vector<size_t> opening_brace_indexes;
    bool is_outer = true;

    std::string result;
    for (size_t i = 0; i < expr->str.str.length(); ++i) {
        auto c = expr->str.str[i];
        if (c == '\\') {
            if (i < expr->str.str.length() - 1) {
                char next = expr->str.str[i + 1];
                if (next == '{' || next == '}') {
                    result += next;
                    ++i;
                    continue;
                }
            }
            // TODO: this should be checked in the parser
            panic<InterpretationException>(expr->str, "runtime error: unterminated \\");
        } else if (c == '{') {
            if (is_outer) {
                opening_brace_indexes.push_back(i);
                is_outer = false;   
            } else {
                panic<InterpretationException>(expr->str, "runtime error: nested brace substitution expressions are not allowed");
            }
        } else if (c == '}') {
            if (opening_brace_indexes.empty()) {
                panic<InterpretationException>(expr->str, "runtime error: mismatched '}'");
            }

            const size_t start_idx = opening_brace_indexes.back();
            opening_brace_indexes.pop_back();

            const size_t expr_length = i - start_idx - 1;
            if (expr_length == 0) {
                panic<InterpretationException>(expr->str, "runtime error: empty expression evaluation");
            }

            const std::string expr_str = expr->str.str.substr(start_idx + 1, expr_length);
            ANKH_DEBUG("{}:{}, parsed expression string '{}' starting @ {}", expr->str.line, expr->str.col, expr_str, start_idx);

            const ExprResult expr_result = evaluate_single_expr(expr->str, expr_str);
            ANKH_DEBUG("'{}' => '{}'", expr_str, expr_result.stringify());

            result += expr_result.stringify();

            is_outer = true;
        } else if (is_outer) {
            result += c;
        }
    }

    if (!opening_brace_indexes.empty()) {
        panic<InterpretationException>(expr->str, "runtime error: mismatched '{'");
    }

    return result;
}

ankh::lang::ExprResult ankh::lang::Interpreter::evaluate_single_expr(const Token& marker, const std::string& str)
{
    auto program = ankh::lang::parse(str);
    if (program.has_errors()) {
        const std::string errors = std::accumulate(program.errors.begin(), program.errors.end(), std::string{""}
        , [](auto& accum, const auto& v) {
            accum += '\n';
            accum += v;

            return accum;
        });

        panic<InterpretationException>(marker, "runtime error: expression '{}' is not valid because:\n{}", str, errors);
    }

    const auto& statements = program.statements;
    if (statements.size() > 1) {
        panic<InterpretationException>(marker, "runtime error: '{}' is a multi return expression", program[0]->stringify());
    }

    if (ExpressionStatement *stmt = instance<ExpressionStatement>(program[0]); stmt == nullptr) {
        panic<InterpretationException>(marker, "runtime error: '{}' is not an expression", program[0]->stringify());
    } else {
        return evaluate(stmt->expr);
    }
}

void ankh::lang::Interpreter::execute(const StatementPtr& stmt)
{
    stmt->accept(this);
}

ankh::lang::Interpreter::ScopeGuard::ScopeGuard(ankh::lang::Interpreter *interpreter, ankh::lang::EnvironmentPtr<ExprResult> enclosing)
    : interpreter_(interpreter)
    , prev_(nullptr)
{
    prev_ = interpreter->current_env_;
    interpreter->current_env_ = make_env<ExprResult>(enclosing);
    ANKH_DEBUG("new scope created from {} to {} through {}", prev_->scope(), interpreter_->current_env_->scope(), enclosing->scope());
}

ankh::lang::Interpreter::ScopeGuard::~ScopeGuard()
{
    ANKH_DEBUG("scope exiting from {} to {}", interpreter_->current_env_->scope(), prev_->scope());
    interpreter_->current_env_ = prev_;
}