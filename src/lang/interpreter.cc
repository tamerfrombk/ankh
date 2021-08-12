#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <fmt/format.h>

#include <fak/def.h>
#include <fak/log.h>

#include <fak/lang/interpreter.h>
#include <fak/lang/parser.h>

#include <fak/lang/token.h>
#include <fak/lang/expr.h>
#include <fak/lang/exceptions.h>

#include <fak/lang/types/array.h>
#include <fak/lang/types/dictionary.h>
#include <fak/lang/types/data.h>
#include <fak/lang/types/object.h>

#include <unordered_map>
#include <vector>

struct ReturnException
    : public std::runtime_error
{
    explicit ReturnException(fk::lang::ExprResult result)
        : std::runtime_error(""), result(std::move(result)) {}

    fk::lang::ExprResult result;
};

template <class... Args>
FK_NO_RETURN static void panic(const char *fmt, Args&&... args)
{
    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
    fk::lang::panic<fk::lang::InterpretationException>("runtime error: {}", msg);
}

template <class... Args>
FK_NO_RETURN static void panic(const fk::lang::ExprResult& result, const char *fmt, Args&&... args)
{
    const std::string typestr = fk::lang::expr_result_type_str(result.type);

    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);

    fk::lang::panic<fk::lang::InterpretationException>("runtime error: {} instead of '{}'", msg, typestr);
}

template <class... Args>
FK_NO_RETURN static void panic(
    const fk::lang::ExprResult& left
    , const fk::lang::ExprResult& right
    , const char *fmt
    , Args&&... args
)
{
    const std::string ltypestr = fk::lang::expr_result_type_str(left.type);
    const std::string rtypestr = fk::lang::expr_result_type_str(right.type);
    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);

    fk::lang::panic<fk::lang::InterpretationException>("runtime error: {} with LHS '{}', RHS '{}'", msg, ltypestr, rtypestr);
}

static bool operands_are(fk::lang::ExprResultType type, std::initializer_list<fk::lang::ExprResult> elems) noexcept
{
    return std::all_of(elems.begin(), elems.end(), [=](const fk::lang::ExprResult& result) {
        return type == result.type;
    });
}

static fk::lang::Number to_num(const std::string& s)
{
    char *end;

    fk::lang::Number n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    const std::string errno_msg(std::strerror(errno));

    panic("'{}' could not be turned into a number due to {}", s, errno_msg);
}

static bool is_integer(fk::lang::Number n) noexcept
{
    double intpart;
    return modf(n, &intpart) == 0.0;
}

static fk::lang::ExprResult negate(const fk::lang::ExprResult& result)
{
    if (result.type == fk::lang::ExprResultType::RT_NUMBER) {
        return -1 * result.n;
    }

    panic(result, "unary (-) operator expects a number expression");
}

static fk::lang::ExprResult invert(const fk::lang::ExprResult& result)
{
    if (result.type == fk::lang::ExprResultType::RT_BOOL) {
        return !(result.b);
    }

    panic(result, "(!) operator expects a boolean expression");
}

static fk::lang::ExprResult eqeq(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n == right.n;
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str == right.str;
    }

    if (operands_are(fk::lang::ExprResultType::RT_BOOL, {left, right})) {
        return left.b == right.b;
    }

    if (operands_are(fk::lang::ExprResultType::RT_NIL, {left, right})) {
        return true;
    }

    panic(left, right, "unknown overload of (==) operator");
}

template <class BinaryOperation>
static fk::lang::ExprResult 
arithmetic(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, BinaryOperation op)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return op(left.n, right.n);
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

static fk::lang::ExprResult division(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        if (right.n == 0) {
            panic("division by zero");
        }
        return left.n / right.n;
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static fk::lang::ExprResult plus(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return left.n + right.n;
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return left.str + right.str;
    }

    panic(left, right, "unknown overload of (+) operator");
}

template <class Compare>
static fk::lang::ExprResult compare(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return cmp(left.n, right.n);
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return cmp(left.str, right.str);
    }

    panic(left, right, "unknown overload of comparison operator");
}

template <class Compare>
static fk::lang::ExprResult logical(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(fk::lang::ExprResultType::RT_BOOL, {left, right})) {
        return cmp(left.b, right.b);
    }

    panic(left, right, "unknown overload of logical operator");
}

static bool truthy(const fk::lang::ExprResult& result) noexcept
{
    if (result.type == fk::lang::ExprResultType::RT_BOOL) {
        return result.b;
    }

    fk::lang::panic<fk::lang::InterpretationException>("'{}' is not a boolean expression", result.stringify());
}

static void print(const fk::lang::ExprResult& result)
{
    const std::string stringy = result.stringify();
    std::puts(stringy.c_str());
}

fk::lang::Interpreter::Interpreter()
    : current_env_(make_env<ExprResult>()) {}

void fk::lang::Interpreter::interpret(const Program& program)
{
    const auto& statements = program.statements();
    for (const auto& stmt : statements) {
#ifndef NDEBUG
        FK_DEBUG("{}", stmt->stringify());
#endif
        execute(stmt);
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(BinaryExpression *expr)
{
    const ExprResult left  = evaluate(expr->left);
    const ExprResult right = evaluate(expr->right);

    switch (expr->op.type) {
    case TokenType::EQEQ:
        return eqeq(left, right);
    case TokenType::NEQ:
        return invert(eqeq(left, right));
    case TokenType::GT:
        return compare(left, right, std::greater<>{});
    case TokenType::GTE:
        return compare(left, right, std::greater_equal<>{});
    case TokenType::LT:
        return compare(left, right, std::less<>{});
    case TokenType::LTE:
        return compare(left, right, std::less_equal<>{});
    case TokenType::MINUS:
        return arithmetic(left, right, std::minus<>{});
    case TokenType::PLUS:
        return plus(left, right);
    case TokenType::STAR:
        return arithmetic(left, right, std::multiplies<>{});
    case TokenType::AND:
        return logical(left, right, std::logical_and<>{});
    case TokenType::OR:
        return logical(left, right, std::logical_or<>{});
    case TokenType::FSLASH:
        return division(left, right);
    default:
        ::panic(left, right, "unknown binary operator '{}'", expr->op.str);
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(UnaryExpression *expr)
{
    const ExprResult result = evaluate(expr->right);
    switch (expr->op.type) {
    case TokenType::MINUS:
        return negate(result);
    case TokenType::BANG:
        return invert(result);
    default:
        ::panic(result, "unknown unary operator '{}'", expr->op.str);
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(LiteralExpression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return to_num(expr->literal.str);
    case TokenType::STRING:
        return expr->literal.str;
    case TokenType::FK_TRUE:
        return true;
    case TokenType::FK_FALSE:
        return false;
    case TokenType::NIL:
        return {};
    default:
        ::panic("unknown literal expression '{}''", expr->literal.str);
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(ParenExpression *expr)
{
    return evaluate(expr->expr);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(IdentifierExpression *expr)
{
    FK_DEBUG("evaluating identifier expression '{}'", expr->name.str);

    auto possible_value = current_env_->value(expr->name.str);
    if (possible_value.has_value()) {
        return possible_value.value();
    }

    ::panic("identifier '{}' not defined", expr->name.str);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(CallExpression *expr)
{
    FK_DEBUG("evaluating call expression");

    const ExprResult callee = evaluate(expr->callee);
    if (callee.type != ExprResultType::RT_CALLABLE) {
        ::panic("only functions and classes are callable");
    }

    Callable *callable = callee.callable;
    const std::string name = callable->name();
    if (expr->args.size() != callable->arity()) {
        ::panic("expected {} arguments to function '{}' instead of {}", callable->arity(), name, expr->args.size());
    }

    FK_DEBUG("function '{}' with matching arity '{}' found", name, expr->args.size());

    try {
        callable->invoke(expr->args);
        
        // TODO: I'm not totally happy with this but this is a solution for constructors
        // Constructors do not return anything (yet)
        // Every other function has a return statement.
        // We take advantage of this here to implement object generation.
        FK_VERIFY(data_declarations_[name]);
        
        auto data = static_cast<Data<ExprResult, Interpreter>*>(functions_[name].get());

        return make_object<ExprResult>(make_env<ExprResult>(data->env()));

    } catch (const ReturnException& e) {
        return e.result;
    }

    FK_FATAL("callables should always return");
}

fk::lang::ExprResult fk::lang::Interpreter::visit(LambdaExpression *expr)
{
    const std::string& name = expr->generated_name;
    if (functions_.count(name) > 0) {
        FK_FATAL("lambda function generated name '{}' is duplicated", name);
    }

    CallablePtr callable = make_callable<Lambda<ExprResult, Interpreter>>(this, expr->clone(), current_env_);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        ::panic("'{}' is already defined", name);
    }

    FK_DEBUG("function '{}' added to scope {}", name, current_env_->scope());

    return result;
}

fk::lang::ExprResult fk::lang::Interpreter::visit(fk::lang::CommandExpression *expr)
{
    FK_DEBUG("executing {}", expr->cmd.str);

    // TODO: popen() uses the underlying shell to invoke commands
    // This limits the language from being used as a shell itself
    // Come back and explore if that's something we want to consider doing
    std::FILE *fp = popen(expr->cmd.str.c_str(), "r");
    if (fp == nullptr) {
        FK_FATAL("popen: unable to launch {}", expr->cmd.str);
    }

    char buf[512];
    std::string output;
    while (std::fread(buf, sizeof(buf[0]), sizeof(buf), fp) > 0) {
        output += buf;
    }
    fclose(fp);

    return output;
}

fk::lang::ExprResult fk::lang::Interpreter::visit(ArrayExpression *expr)
{
    Array<ExprResult> array;
    for (const auto& e : expr->elems) {
        array.append(evaluate(e));
    }

    return array;
}

fk::lang::ExprResult fk::lang::Interpreter::visit(fk::lang::IndexExpression *expr)
{
    const ExprResult indexee = evaluate(expr->indexee);
    if (indexee.type != ExprResultType::RT_ARRAY && indexee.type != ExprResultType::RT_DICT) {
        ::panic("lookup expects array or dict operand");
    }

    const ExprResult index = evaluate(expr->index);
    if (index.type == ExprResultType::RT_NUMBER) {
        if (!is_integer(index.n)) {
            ::panic("index must be an integral numeric expression");
        }

        if (indexee.type != ExprResultType::RT_ARRAY) {
            ::panic("operand must be an array for a numeric index");
        }

        if (index.n >= indexee.array.size()) {
            ::panic("index {} must be less than array size {}", index.n, indexee.array.size());
        }

        return indexee.array[index.n];
    }

    if (index.type == ExprResultType::RT_STRING) {
        if (indexee.type != ExprResultType::RT_DICT) {
            ::panic("operand must be a dict for a string index");
        }

        if (auto possible_value = indexee.dict.value(index.str); possible_value.has_value()) {
            return possible_value->value;
        }

        return {};
    }

    ::panic("'{}' is not a valid lookup expression", index.stringify());
}

fk::lang::ExprResult fk::lang::Interpreter::visit(fk::lang::DictionaryExpression *expr)
{
    Dictionary<ExprResult> dict;
    for (const auto& [key, value] : expr->entries) {
        const ExprResult& key_result = evaluate(key);
        if (key_result.type != ExprResultType::RT_STRING) {
            ::panic("expression key does not evaluate to a string");
        }
        dict.insert(key_result, evaluate(value));
    }

    return dict;
}

fk::lang::ExprResult fk::lang::Interpreter::visit(fk::lang::StringExpression *expr)
{
    return substitute(expr);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(fk::lang::AccessExpression *expr)
{
    ExprResult result = evaluate(expr->accessible);
    if (result.type == ExprResultType::RT_OBJECT) {
        if (auto possible_value = result.obj->env->value(expr->accessor.str); possible_value) {
            return possible_value.value();
        }
        ::panic("'{}' is not a member of '{}'", expr->accessor.str, expr->accessible->stringify());
    }

    ::panic("'{}' is not accessible", expr->accessible->stringify());
}

void fk::lang::Interpreter::visit(PrintStatement *stmt)
{
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::Interpreter::visit(ExpressionStatement *stmt)
{
    FK_DEBUG("executing expression statement");
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::Interpreter::visit(VariableDeclaration *stmt)
{
    if (current_env_->contains(stmt->name.str)) {
        ::panic("'{}' is already declared in this scope", stmt->name.str);
    }

    const ExprResult result = evaluate(stmt->initializer);

    FK_DEBUG("DECLARATION '{}' = '{}'", stmt->name.str, result.stringify());

    if (!current_env_->declare(stmt->name.str, result)) {
        ::panic("'{}' is already defined", stmt->name.str);
    }

    if (stmt->storage_class == StorageClass::EXPORT) {
        const std::string result_str = result.stringify();
        if (setenv(stmt->name.str.c_str(), result_str.c_str(), 1) == -1) {
            const std::string errno_msg(std::strerror(errno));
            ::panic("'{}' could not be exported due to {}", stmt->name.str, errno_msg);
        }
    }
}

void fk::lang::Interpreter::visit(AssignmentStatement *stmt)
{
    const ExprResult result = evaluate(stmt->initializer);
    if (!current_env_->assign(stmt->name.str, result)) {
        ::panic("'{}' is not defined", stmt->name.str);
    }
}

void fk::lang::Interpreter::visit(BlockStatement *stmt)
{
    execute_block(stmt, current_env_);
}

void fk::lang::Interpreter::execute_block(const BlockStatement *stmt, EnvironmentPtr<ExprResult> environment)
{
    Scope block_scope(this, environment);
    for (const StatementPtr& statement : stmt->statements) {
        execute(statement);
    }
}

void fk::lang::Interpreter::visit(IfStatement *stmt)
{
    const ExprResult result = evaluate(stmt->condition);
    if (truthy(result)) {
        execute(stmt->then_block);
    } else if (stmt->else_block != nullptr) {
        execute(stmt->else_block);
    }
}

void fk::lang::Interpreter::visit(WhileStatement *stmt)
{
    ExprResult result = evaluate(stmt->condition);
    while (truthy(result)) {
        execute(stmt->body);
        result = evaluate(stmt->condition);
    }
}

void fk::lang::Interpreter::visit(fk::lang::FunctionDeclaration *stmt)
{
    declare_function(stmt, current_env_);
}

void fk::lang::Interpreter::declare_function(FunctionDeclaration *decl, EnvironmentPtr<ExprResult> env)
{
    FK_DEBUG("evaluating function declaration of '{}'", decl->name.str);

    const std::string& name = decl->name.str;
    if (functions_.count(name) > 0) {
        ::panic("function '{}' is already declared", name);
    }

    CallablePtr callable = make_callable<Function<ExprResult, Interpreter>>(this, decl->clone(), env);

    ExprResult result { callable.get() };
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        ::panic("'{}' is already defined", name);
    }

    FK_DEBUG("function '{}' added to scope {}", name, current_env_->scope());
}

void fk::lang::Interpreter::visit(ReturnStatement *stmt)
{
    FK_DEBUG("evaluating return statement");

    const ExprResult result = evaluate(stmt->expr);

    throw ReturnException(result);
}

void fk::lang::Interpreter::visit(DataDeclaration *stmt)
{
    FK_DEBUG("evaluating data '{}' declaration", stmt->name.str);

    if (data_declarations_[stmt->name.str]) {
        ::panic("{}:{}, '{}' is already a data declaration", stmt->name.line, stmt->name.col, stmt->name.str);
    }

    EnvironmentPtr<ExprResult> env = make_env<ExprResult>(current_env_);
    std::vector<std::string> members;
    for (const auto& member : stmt->members) {
        if (!env->declare(member.str, ExprResult{})) {
            FK_FATAL("unable to declare data member '{}'", member.str);
        }
        members.push_back(member.str);
    }

    functions_[stmt->name.str] = make_callable<Data<ExprResult, Interpreter>>(this, stmt->name.str, env, members);

    if (!current_env_->declare(stmt->name.str, functions_[stmt->name.str].get())) {
        FK_FATAL("unable to declare constructor for data declaration '{}'", stmt->name.str);
    }

    data_declarations_[stmt->name.str] = true;

    FK_DEBUG("data '{}' declared", stmt->name.str);
}

fk::lang::ExprResult fk::lang::Interpreter::evaluate(const ExpressionPtr& expr)
{
    return expr->accept(this);
}

std::string fk::lang::Interpreter::substitute(const StringExpression *expr)
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
            ::panic("unterminated \\");
        } else if (c == '{') {
            if (is_outer) {
                opening_brace_indexes.push_back(i);
                is_outer = false;   
            } else {
                ::panic("{}:{}, nested brace substitution expressions are not allowed", expr->str.line, expr->str.col + i);
            }
        } else if (c == '}') {
            if (opening_brace_indexes.empty()) {
                ::panic("{}:{}, mismatched '}'", expr->str.line, expr->str.col);
            }

            const size_t start_idx = opening_brace_indexes.back();
            opening_brace_indexes.pop_back();

            const size_t expr_length = i - start_idx - 1;
            if (expr_length == 0) {
                ::panic("{}:{}, empty expression evaluation", expr->str.line, expr->str.col);
            }

            const std::string expr_str = expr->str.str.substr(start_idx + 1, expr_length);
            FK_DEBUG("{}:{}, parsed expression string '{}' starting @ {}", expr->str.line, expr->str.col, expr_str, start_idx);

            const ExprResult expr_result = evaluate_single_expr(expr_str);
            FK_DEBUG("'{}' => '{}'", expr_str, expr_result.stringify());

            result += expr_result.stringify();

            is_outer = true;
        } else if (is_outer) {
            result += c;
        }
    }

    if (!opening_brace_indexes.empty()) {
        ::panic("{}:{}, mismatched '{'", expr->str.line, expr->str.col);
    }

    return result;
}

fk::lang::ExprResult fk::lang::Interpreter::evaluate_single_expr(const std::string& str)
{
    auto program = fk::lang::parse(str);
    if (program.has_errors()) {
        // TODO: print out why
        ::panic("expression '{}' is not valid", str);
    }

    const auto& statements = program.statements();
    if (statements.size() > 1) {
        ::panic("'{}' is a multi return expression");
    }

    if (ExpressionStatement *stmt = instance<ExpressionStatement>(program[0]); stmt == nullptr) {
        ::panic("'{}' is not an expression", str);
    } else {
        return evaluate(stmt->expr);
    }
}

void fk::lang::Interpreter::execute(const StatementPtr& stmt)
{
    stmt->accept(this);
}

fk::lang::Interpreter::Scope::Scope(fk::lang::Interpreter *interpreter, fk::lang::EnvironmentPtr<ExprResult> enclosing)
    : interpreter_(interpreter)
    , prev_(nullptr)
{
    prev_ = interpreter->current_env_;
    interpreter->current_env_ = make_env<ExprResult>(enclosing);
    FK_DEBUG("new scope created from {} to {} through {}", prev_->scope(), interpreter_->current_env_->scope(), enclosing->scope());
}

fk::lang::Interpreter::Scope::~Scope()
{
    FK_DEBUG("scope exiting from {} to {}", interpreter_->current_env_->scope(), prev_->scope());
    interpreter_->current_env_ = prev_;
}