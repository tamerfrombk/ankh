#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <fak/def.h>
#include <fak/log.h>

#include <fak/lang/interpreter.h>
#include <fak/lang/token.h>
#include <fak/lang/expr.h>
#include <fak/lang/interpretation_exception.h>
#include <fak/lang/callable.h>

#include <fak/internal/pretty_printer.h>

struct ReturnException
    : public std::runtime_error
{
    explicit ReturnException(fk::lang::ExprResult result)
        : std::runtime_error(""), result(std::move(result)) {}

    fk::lang::ExprResult result;
};

FK_NO_RETURN static void panic(const std::string& msg)
{
    throw fk::lang::InterpretationException("runtime error: " + msg);
}

FK_NO_RETURN static void panic(const fk::lang::ExprResult& result, const std::string& msg)
{
    const std::string typestr = fk::lang::expr_result_type_str(result.type);

    throw fk::lang::InterpretationException("runtime error: " + msg + " instead of '" + typestr + "'");
}

FK_NO_RETURN static void panic(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, const std::string& msg)
{
    const std::string ltypestr = fk::lang::expr_result_type_str(left.type);
    const std::string rtypestr = fk::lang::expr_result_type_str(right.type);

    throw fk::lang::InterpretationException("runtime error: " + msg 
        + " with LHS '" + ltypestr + "', RHS '" + rtypestr + "'");
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

    panic("'" + s + "' could not be turned into a number due to " + errno_msg);
}

static fk::lang::ExprResult negate(const fk::lang::ExprResult& result)
{
    if (result.type == fk::lang::ExprResultType::RT_NUMBER) {
        return fk::lang::ExprResult::num(-1 * result.n);
    }

    panic(result, "unary (-) operator expects a number expression");
}

static fk::lang::ExprResult invert(const fk::lang::ExprResult& result)
{
    if (result.type == fk::lang::ExprResultType::RT_BOOL) {
        return fk::lang::ExprResult::boolean(!(result.b));
    }

    panic(result, "(!) operator expects a boolean expression");
}

static fk::lang::ExprResult eqeq(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return fk::lang::ExprResult::boolean(left.n == right.n);
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return fk::lang::ExprResult::boolean(left.str == right.str);
    }

    if (operands_are(fk::lang::ExprResultType::RT_BOOL, {left, right})) {
        return fk::lang::ExprResult::boolean(left.b == right.b);
    }

    if (operands_are(fk::lang::ExprResultType::RT_NIL, {left, right})) {
        return fk::lang::ExprResult::boolean(true);
    }

    panic(left, right, "unknown overload of (==) operator");
}

template <class BinaryOperation>
static fk::lang::ExprResult 
arithmetic(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, BinaryOperation op)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return fk::lang::ExprResult::num(op(left.n, right.n));
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static fk::lang::ExprResult plus(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return fk::lang::ExprResult::num(left.n + right.n);
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return fk::lang::ExprResult::string(left.str + right.str);
    }

    panic(left, right, "unknown overload of (+) operator");
}

template <class Compare>
static fk::lang::ExprResult compare(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(fk::lang::ExprResultType::RT_NUMBER, {left, right})) {
        return fk::lang::ExprResult::boolean(cmp(left.n, right.n));
    }

    if (operands_are(fk::lang::ExprResultType::RT_STRING, {left, right})) {
        return fk::lang::ExprResult::boolean(cmp(left.str, right.str));
    }

    panic(left, right, "unknown overload of comparison operator");
}

// TODO: rethink truthiness -- do we really only want boolean expressions?
static bool truthy(const fk::lang::ExprResult& result) noexcept
{
    if (result.type == fk::lang::ExprResultType::RT_BOOL) {
        return result.b;
    }

    return false;
}

static void print(const fk::lang::ExprResult& result)
{
    const std::string stringy = result.stringify();
    std::puts(stringy.c_str());
}

fk::lang::Interpreter::Interpreter()
{
    // initialize the global scope
    enter_new_scope();
}

void fk::lang::Interpreter::interpret(const Program& program)
{
    for (const auto& stmt : program) {
#ifndef NDEBUG
        fk::internal::PrettyPrinter printer;
        const std::string pretty = stmt->accept(&printer);
        FK_DEBUG("{}", pretty);
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
    case TokenType::FSLASH:
        // TODO: right now, dividing by 0 yields 'inf' revisit this and make sure that's the behavior we want for the language
        return arithmetic(left, right, std::divides<>{});
    default:
        panic(left, right, "unknown binary operator (" + expr->op.str + ")");
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
        panic(result, "unknown unary (" + expr->op.str + ") operator");
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(LiteralExpression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return ExprResult::num(to_num(expr->literal.str));
    case TokenType::STRING:
        return ExprResult::string(expr->literal.str);
    case TokenType::FK_TRUE:
        return ExprResult::boolean(true);
    case TokenType::FK_FALSE:
        return ExprResult::boolean(false);
    case TokenType::NIL:
        return ExprResult::nil();
    default:
        panic("unkown literal expression: '" + expr->literal.str + "'");
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(ParenExpression *expr)
{
    return evaluate(expr->expr);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(IdentifierExpression *expr)
{
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        auto possible_value = it->value(expr->name.str); 
        if (possible_value.has_value()) {
            FK_DEBUG("IDENTIFIER '{}' = '{}' @ scope '{}'", expr->name.str, possible_value.value().stringify(), scope());
            return possible_value.value();
        }
    }

    panic("identifier " + expr->name.str + " is not defined in the current scope");
}

fk::lang::ExprResult fk::lang::Interpreter::visit(AndExpression *expr)
{
    const ExprResult left = evaluate(expr->left);
    if (!truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(OrExpression *expr)
{
    const ExprResult left = evaluate(expr->left);
    if (truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(CallExpression *expr)
{
    // TODO: ugh, fix this again
    const ExprResult callee = evaluate(expr->callee);
    if (callee.type != ExprResultType::RT_CALLABLE) {
        panic("only functions and classes are callable");
    }

    Callable *callable = callee.callable;
    const std::string name = callable->name();
    if (expr->args.size() != callable->arity()) {
        panic("expected " + std::to_string(callable->arity()) + " arguments to function " + name + " instead of " + std::to_string(expr->args.size()));
    }

    FK_DEBUG("function '{}' with matching arity '{}' found in the current scope", name, expr->args.size());

    size_t entered_scope;
    try {
        enter_new_scope();
        entered_scope = scope();
        
        callable->invoke(expr->args);
        
        leave_current_scope();
    } catch (const InterpretationException& e) {
        leave_current_scope();
        throw e;
    } catch (const ReturnException& e) {
        size_t return_scope = scope();
        FK_DEBUG("return scope: '{}' entered scope '{}'", return_scope, entered_scope);
        
        FK_VERIFY(return_scope >= entered_scope);
        
        // unwind the stack
        while (return_scope >= entered_scope) {
            leave_current_scope();
            --return_scope;
        }

        return e.result;
    }

    // this shouldn't be reached but it's here to satisfy the compiler
    return ExprResult::nil();
}

void fk::lang::Interpreter::visit(PrintStatement *stmt)
{
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::Interpreter::visit(ExpressionStatement *stmt)
{
    evaluate(stmt->expr);
}

void fk::lang::Interpreter::visit(VariableDeclaration *stmt)
{
    if (current_scope().contains(stmt->name.str)) {
        panic(stmt->name.str + " is already declared in this scope");
    }

    const ExprResult result = evaluate(stmt->initializer);
    FK_DEBUG("DECLARATION '{}' = '{}'", stmt->name.str, result.stringify());

    current_scope().assign(stmt->name.str, result);
}

void fk::lang::Interpreter::visit(AssignmentStatement *stmt)
{
    // TODO: think about adding scope specifiers (export, local, global) to allow the user
    // to mutate global variables with the same name or to add variables to the environment
    size_t this_scope = scope();
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        if (it->contains(stmt->name.str)) {
            const ExprResult result = evaluate(stmt->initializer);
            it->assign(stmt->name.str, result);
            FK_DEBUG("ASSIGNMENT '{}' = '{}' @ scope '{}'", stmt->name.str, result.stringify(), this_scope);
            return;
        }
        --this_scope;
    }
    
    panic(stmt->name.str + " is not defined");
}

void fk::lang::Interpreter::visit(BlockStatement *stmt)
{
    enter_new_scope();
    for (const StatementPtr& statement : stmt->statements) {
        execute(statement);
    }
    leave_current_scope();
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
    const std::string& name = stmt->name.str;
    if (functions_.count(name) > 0) {
        panic("function " + stmt->name.str + " is already declared in this scope");
    }

    CallablePtr callable = make_callable<Function>(this, stmt);

    ExprResult result = ExprResult::call(callable.get());

    functions_[name] = std::move(callable);

    global_scope().assign(name, result);

    FK_DEBUG("function '{}' added to global scope", name);
}

void fk::lang::Interpreter::visit(ReturnStatement *stmt)
{
    const ExprResult result = evaluate(stmt->expr);

    throw ReturnException(result);
}

fk::lang::ExprResult fk::lang::Interpreter::evaluate(const ExpressionPtr& expr)
{
    return expr->accept(this);
}

void fk::lang::Interpreter::execute(const StatementPtr& stmt)
{
    stmt->accept(this);
}

void fk::lang::Interpreter::enter_new_scope() noexcept
{
    env_.emplace_back();
    FK_DEBUG("entered new scope '{}'", scope());
}

void fk::lang::Interpreter::leave_current_scope() noexcept
{
    FK_DEBUG("leaving scope '{}'", scope());
    env_.pop_back();
}

fk::lang::Environment& fk::lang::Interpreter::current_scope() noexcept
{
    return env_.back();
}

fk::lang::Environment& fk::lang::Interpreter::global_scope() noexcept
{
    return env_.front();
}

size_t fk::lang::Interpreter::scope() const noexcept
{
    return env_.size() - 1;
}
