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
#include <fak/lang/token.h>
#include <fak/lang/expr.h>
#include <fak/lang/exceptions.h>
#include <fak/lang/callable.h>

#include <fak/internal/pretty_printer.h>

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

template <class Compare>
static fk::lang::ExprResult logical(const fk::lang::ExprResult& left, const fk::lang::ExprResult& right, Compare cmp)
{
    if (operands_are(fk::lang::ExprResultType::RT_BOOL, {left, right})) {
        return fk::lang::ExprResult::boolean(cmp(left.n, right.n));
    }

    panic(left, right, "unknown overload of comparison operator");
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
    : current_env_(make_env(nullptr)) {}

void fk::lang::Interpreter::interpret(const Program& program)
{
    const auto& statements = program.statements();
    for (const auto& stmt : statements) {
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
    case TokenType::AND:
        return logical(left, right, std::logical_and<>{});
    case TokenType::OR:
        return logical(left, right, std::logical_or<>{});
    case TokenType::FSLASH:
        // TODO: right now, dividing by 0 yields 'inf' revisit this and make sure that's the behavior we want for the language
        return arithmetic(left, right, std::divides<>{});
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
        ::panic("unknown literal expression '{}''", expr->literal.str);
    }
}

fk::lang::ExprResult fk::lang::Interpreter::visit(ParenExpression *expr)
{
    return evaluate(expr->expr);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(IdentifierExpression *expr)
{
    auto possible_value = current_env_->value(expr->name.str);
    if (possible_value.has_value()) {
        return possible_value.value();
    }

    ::panic("identifier '{}' not defined", expr->name.str);
}

fk::lang::ExprResult fk::lang::Interpreter::visit(CallExpression *expr)
{
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

    CallablePtr callable = make_callable<Lambda>(this, expr, current_env_);

    ExprResult result = ExprResult::call(callable.get());
    
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

    return fk::lang::ExprResult::string(output);
}

void fk::lang::Interpreter::visit(PrintStatement *stmt)
{
    const ExprResult result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::Interpreter::visit(ExpressionStatement *stmt)
{
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
}

void fk::lang::Interpreter::visit(AssignmentStatement *stmt)
{
    // TODO: think about adding scope specifiers (export, local, global) to allow the user
    // to mutate global variables with the same name or to add variables to the environment
    const ExprResult result = evaluate(stmt->initializer);
    if (!current_env_->assign(stmt->name.str, result)) {
        ::panic("'{}' is not defined", stmt->name.str);
    }
}

void fk::lang::Interpreter::visit(BlockStatement *stmt)
{
    execute_block(stmt, current_env_.get());
}

void fk::lang::Interpreter::execute_block(const BlockStatement *stmt, Environment *environment)
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
    const std::string& name = stmt->name.str;
    if (functions_.count(name) > 0) {
        ::panic("function '{}' is already declared", name);
    }

    CallablePtr callable = make_callable<Function>(this, stmt, current_env_);

    ExprResult result = ExprResult::call(callable.get());
    
    functions_[name] = std::move(callable);
    
    if (!current_env_->declare(name, result)) {
        ::panic("'{}' is already defined", name);
    }

    FK_DEBUG("function '{}' added to scope {}", name, current_env_->scope());
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

fk::lang::Interpreter::Scope::Scope(fk::lang::Interpreter *interpreter, fk::lang::Environment *enclosing)
    : interpreter_(interpreter)
    , prev_(nullptr)
{
    prev_ = std::move(interpreter->current_env_);
    interpreter->current_env_ = make_env(enclosing);
    FK_DEBUG("new scope created from {} to {} through {}", prev_->scope(), interpreter_->current_env_->scope(), enclosing->scope());
}

fk::lang::Interpreter::Scope::~Scope()
{
    FK_DEBUG("scope exiting from {} to {}", interpreter_->current_env_->scope(), prev_->scope());
    interpreter_->current_env_ = std::move(prev_);
}