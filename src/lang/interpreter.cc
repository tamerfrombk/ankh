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

#include <fak/internal/pretty_printer.h>

struct return_exception
    : public std::runtime_error
{
    explicit return_exception(fk::lang::expr_result result)
        : std::runtime_error(""), result(std::move(result)) {}

    fk::lang::expr_result result;
};

FK_NO_RETURN static void panic(const std::string& msg)
{
    throw fk::lang::interpretation_exception("runtime error: " + msg);
}

FK_NO_RETURN static void panic(const fk::lang::expr_result& result, const std::string& msg)
{
    const std::string typestr = fk::lang::expr_result_type_str(result.type);

    throw fk::lang::interpretation_exception("runtime error: " + msg + " instead of '" + typestr + "'");
}

FK_NO_RETURN static void panic(const fk::lang::expr_result& left, const fk::lang::expr_result& right, const std::string& msg)
{
    const std::string ltypestr = fk::lang::expr_result_type_str(left.type);
    const std::string rtypestr = fk::lang::expr_result_type_str(right.type);

    throw fk::lang::interpretation_exception("runtime error: " + msg 
        + " with LHS '" + ltypestr + "', RHS '" + rtypestr + "'");
}

static bool operands_are(fk::lang::expr_result_type type, std::initializer_list<fk::lang::expr_result> elems) noexcept
{
    return std::all_of(elems.begin(), elems.end(), [=](const fk::lang::expr_result& result) {
        return type == result.type;
    });
}

static fk::lang::number to_num(const std::string& s)
{
    char *end;

    fk::lang::number n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    const std::string errno_msg(std::strerror(errno));

    panic("'" + s + "' could not be turned into a number due to " + errno_msg);
}

static fk::lang::expr_result negate(const fk::lang::expr_result& result)
{
    if (result.type == fk::lang::expr_result_type::RT_NUMBER) {
        return fk::lang::expr_result::num(-1 * result.n);
    }

    panic(result, "unary (-) operator expects a number expression");
}

static fk::lang::expr_result invert(const fk::lang::expr_result& result)
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return fk::lang::expr_result::boolean(!(result.b));
    }

    panic(result, "(!) operator expects a boolean expression");
}

static fk::lang::expr_result eqeq(const fk::lang::expr_result& left, const fk::lang::expr_result& right)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::boolean(left.n == right.n);
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::boolean(left.str == right.str);
    }

    if (operands_are(fk::lang::expr_result_type::RT_BOOL, {left, right})) {
        return fk::lang::expr_result::boolean(left.b == right.b);
    }

    if (operands_are(fk::lang::expr_result_type::RT_NIL, {left, right})) {
        return fk::lang::expr_result::boolean(true);
    }

    panic(left, right, "unknown overload of (==) operator");
}

template <class BinaryOperation>
static fk::lang::expr_result 
arithmetic(const fk::lang::expr_result& left, const fk::lang::expr_result& right, BinaryOperation op)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::num(op(left.n, right.n));
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static fk::lang::expr_result plus(const fk::lang::expr_result& left, const fk::lang::expr_result& right)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::num(left.n + right.n);
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::string(left.str + right.str);
    }

    panic(left, right, "unknown overload of (+) operator");
}

template <class Compare>
static fk::lang::expr_result compare(const fk::lang::expr_result& left, const fk::lang::expr_result& right, Compare cmp)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.n, right.n));
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.str, right.str));
    }

    panic(left, right, "unknown overload of comparison operator");
}

// TODO: rethink truthiness -- do we really only want boolean expressions?
static bool truthy(const fk::lang::expr_result& result) noexcept
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return result.b;
    }

    return false;
}

static void print(const fk::lang::expr_result& result)
{
    const std::string stringy = result.stringify();
    std::puts(stringy.c_str());
}

fk::lang::interpreter::interpreter()
{
    // initialize the global scope
    enter_new_scope();
}

void fk::lang::interpreter::interpret(const program& program)
{
    for (const auto& stmt : program) {
#ifndef NDEBUG
        fk::internal::pretty_printer printer;
        const std::string pretty = stmt->accept(&printer);
        fk::log::debug("%s\n", pretty.c_str());
#endif
        execute(stmt);
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(binary_expression *expr)
{
    const expr_result left  = evaluate(expr->left);
    const expr_result right = evaluate(expr->right);

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

fk::lang::expr_result fk::lang::interpreter::visit(unary_expression *expr)
{
    const expr_result result = evaluate(expr->right);
    switch (expr->op.type) {
    case TokenType::MINUS:
        return negate(result);
    case TokenType::BANG:
        return invert(result);
    default:
        panic(result, "unknown unary (" + expr->op.str + ") operator");
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(literal_expression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return expr_result::num(to_num(expr->literal.str));
    case TokenType::STRING:
        return expr_result::string(expr->literal.str);
    case TokenType::FK_TRUE:
        return expr_result::boolean(true);
    case TokenType::FK_FALSE:
        return expr_result::boolean(false);
    case TokenType::NIL:
        return expr_result::nil();
    default:
        panic("unkown literal expression: '" + expr->literal.str + "'");
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(paren_expression *expr)
{
    return evaluate(expr->expr);
}

fk::lang::expr_result fk::lang::interpreter::visit(identifier_expression *expr)
{
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        auto possible_value = it->value(expr->name.str); 
        if (possible_value.has_value()) {
            fk::log::debug("IDENTIFIER '%s' = '%s' @ scope '%d'\n", expr->name.str.c_str(), possible_value.value().stringify().c_str(), scope());
            return possible_value.value();
        }
    }

    panic("identifier " + expr->name.str + " is not defined in the current scope");
}

fk::lang::expr_result fk::lang::interpreter::visit(and_expression *expr)
{
    const expr_result left = evaluate(expr->left);
    if (!truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

fk::lang::expr_result fk::lang::interpreter::visit(or_expression *expr)
{
    const expr_result left = evaluate(expr->left);
    if (truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

fk::lang::expr_result fk::lang::interpreter::visit(call_expression *expr)
{
    // TODO: ugh, fix this again
    if (!dynamic_cast<identifier_expression*>(expr->name.get())) {
        panic("<identifier> expected as function call name");
    }

    const std::string name = static_cast<identifier_expression*>(expr->name.get())->name.str;
    if (functions_.count(name) == 0) {
        panic(name + " is not a function defined in this scope");
    }

    function_declaration *declaration = functions_[name];
    if (expr->args.size() != declaration->params.size()) {
        panic("expected " + std::to_string(declaration->params.size()) + " arguments to function " + name + " instead of " + std::to_string(expr->args.size()));
    }

    fk::log::debug("function '%s' with matching arity '%d' found in the current scope\n", name.c_str(), expr->args.size());

    size_t entered_scope;
    try {
        enter_new_scope();
        entered_scope = scope();
        
        for (size_t i = 0; i < expr->args.size(); ++i) {
            const expr_result arg = evaluate(expr->args[i]);
            current_scope().assign(declaration->params[i].str, arg);
        }

        // Here, we avoid calling execute(declaration->body) directly because visiting a block statement will
        // create a new additional environment we don't want. When we implement function calls, we will be controlling
        // the environment of the child block so we execute its statements directly here.
        block_statement *block = static_cast<block_statement*>(declaration->body.get());
        for (const auto& stmt : block->statements) {
            execute(stmt);
        }
        
        leave_current_scope();
    } catch (const interpretation_exception& e) {
        leave_current_scope();
        throw e;
    } catch (const return_exception& e) {
        size_t return_scope = scope();
        fk::log::debug("return scope: %d entered scope %d\n", return_scope, entered_scope);
        // TODO: create an assertion that return_scope is always > entered_scope here
        
        // unwind the stack
        while (return_scope >= entered_scope) {
            leave_current_scope();
            --return_scope;
        }

        return e.result;
    }

    // this shouldn't be reached but it's here to satisfy the compiler
    return expr_result::nil();
}

void fk::lang::interpreter::visit(print_statement *stmt)
{
    const expr_result result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::interpreter::visit(expression_statement *stmt)
{
    evaluate(stmt->expr);
}

void fk::lang::interpreter::visit(variable_declaration *stmt)
{
    if (current_scope().contains(stmt->name.str)) {
        panic(stmt->name.str + " is already declared in this scope");
    }

    const expr_result result = evaluate(stmt->initializer);
    fk::log::debug("DECLARATION '%s' = '%s'\n", stmt->name.str.c_str(), result.stringify().c_str());

    current_scope().assign(stmt->name.str, result);
}

void fk::lang::interpreter::visit(assignment_statement *stmt)
{
    // TODO: think about adding scope specifiers (export, local, global) to allow the user
    // to mutate global variables with the same name or to add variables to the environment
    size_t this_scope = scope();
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        if (it->contains(stmt->name.str)) {
            const expr_result result = evaluate(stmt->initializer);
            it->assign(stmt->name.str, result);
            fk::log::debug("ASSIGNMENT '%s' = '%s' @ scope '%d'\n", stmt->name.str.c_str(), result.stringify().c_str(), this_scope);
            return;
        }
        --this_scope;
    }
    
    panic(stmt->name.str + " is not defined");
}

void fk::lang::interpreter::visit(block_statement *stmt)
{
    enter_new_scope();
    for (const statement_ptr& statement : stmt->statements) {
        execute(statement);
    }
    leave_current_scope();
}

void fk::lang::interpreter::visit(if_statement *stmt)
{
    const expr_result result = evaluate(stmt->condition);
    if (truthy(result)) {
        execute(stmt->then_block);
    } else if (stmt->else_block != nullptr) {
        execute(stmt->else_block);
    }
}

void fk::lang::interpreter::visit(while_statement *stmt)
{
    expr_result result = evaluate(stmt->condition);
    while (truthy(result)) {
        execute(stmt->body);
        result = evaluate(stmt->condition);
    }
}

void fk::lang::interpreter::visit(fk::lang::function_declaration *stmt)
{
    if (functions_.count(stmt->name.str) > 0) {
        panic("function " + stmt->name.str + " is already declared in this scope");
    }

    functions_[stmt->name.str] = stmt;

    fk::log::debug("function '%s' added to current scope\n", stmt->name.str.c_str());
}

void fk::lang::interpreter::visit(return_statement *stmt)
{
    const expr_result result = evaluate(stmt->expr);

    throw return_exception(result);
}

fk::lang::expr_result fk::lang::interpreter::evaluate(const expression_ptr& expr)
{
    return expr->accept(this);
}

void fk::lang::interpreter::execute(const statement_ptr& stmt)
{
    stmt->accept(this);
}

void fk::lang::interpreter::enter_new_scope() noexcept
{
    env_.emplace_back();
    fk::log::debug("entered new scope %d\n", scope());
}

void fk::lang::interpreter::leave_current_scope() noexcept
{
    fk::log::debug("leaving scope %d\n", scope());
    env_.pop_back();
}

fk::lang::environment& fk::lang::interpreter::current_scope() noexcept
{
    return env_.back();
}

size_t fk::lang::interpreter::scope() const noexcept
{
    return env_.size() - 1;
}
