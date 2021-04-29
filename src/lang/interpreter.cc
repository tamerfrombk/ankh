#include "fak/lang/env.h"
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>

#include <fak/lang/interpreter.h>
#include <fak/lang/token.h>
#include <fak/lang/expr.h>
#include <fak/log.h>

static bool operands_are(fk::lang::expr_result_type type, std::initializer_list<fk::lang::expr_result> elems)
{
    return std::all_of(elems.begin(), elems.end(), [=](const fk::lang::expr_result& result) {
        return type == result.type;
    });
}

static fk::lang::number to_num(const std::string& s) noexcept
{
    char *end;

    fk::lang::number n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    // TODO: properly handle error
    return -1;
}

static fk::lang::expr_result negate(const fk::lang::expr_result& result) noexcept
{
    if (result.type == fk::lang::expr_result_type::RT_NUMBER) {
        return fk::lang::expr_result::num(-1 * result.n);
    }
    
    // TODO: improve the error message
    return fk::lang::expr_result::error("- operator expects a number expression");
}

static fk::lang::expr_result invert(const fk::lang::expr_result& result) noexcept
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return fk::lang::expr_result::boolean(!(result.b));
    }

    // TODO: improve the error message
    return fk::lang::expr_result::error("! operator expects a boolean expression");
}

static fk::lang::expr_result eqeq(const fk::lang::expr_result& left, const fk::lang::expr_result& right) noexcept
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

    // TODO: improve error message
    return fk::lang::expr_result::error("unknown overload for (!=) operator");
}

template <class BinaryOperation>
static fk::lang::expr_result 
arithmetic(const fk::lang::expr_result& left, const fk::lang::expr_result& right, BinaryOperation op) noexcept
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::num(op(left.n, right.n));
    }

    // TODO: improve error message
    return fk::lang::expr_result::error("unknown overload for arithmetic operator");
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

    // TODO: improve error message
    return fk::lang::expr_result::error("unknown overload for (+) operator");
}

template <class Compare>
static fk::lang::expr_result 
compare(const fk::lang::expr_result& left, const fk::lang::expr_result& right, Compare cmp) noexcept
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.n, right.n));
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.str, right.str));
    }

    // TODO: improve error message
    return fk::lang::expr_result::error("unknown overload for comparison operator");
}

// TODO: rethink truthiness -- do we really only want boolean expressions?
static bool truthy(const fk::lang::expr_result& result) noexcept
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return result.b;
    }

    return false;
}

fk::lang::interpreter::interpreter()
{
    // initialize the global scope
    enter_new_scope();
}

void fk::lang::interpreter::interpret(const program& program)
{
    for (const auto& stmt : program) {
        execute(stmt);
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(binary_expression *expr)
{
    const expr_result left  = evaluate(expr->left);
    const expr_result right = evaluate(expr->right);

    switch (expr->op.type) {
    case token_type::EQEQ:
        return eqeq(left, right);
    case token_type::NEQ:
        return invert(eqeq(left, right));
    case token_type::GT:
        return compare(left, right, std::greater<>{});
    case token_type::GTE:
        return compare(left, right, std::greater_equal<>{});
    case token_type::LT:
        return compare(left, right, std::less<>{});
    case token_type::LTE:
        return compare(left, right, std::less_equal<>{});
    case token_type::MINUS:
        return arithmetic(left, right, std::minus<>{});
    case token_type::PLUS:
        return plus(left, right);
    case token_type::STAR:
        return arithmetic(left, right, std::multiplies<>{});
    case token_type::FSLASH:
        // TODO: right now, dividing by 0 yields 'inf' revisit this and make sure that's the behavior we want for the language
        return arithmetic(left, right, std::divides<>{});
    default:
        fk::log::fatal("unimplemented binary operator '%s'\n", expr->op.str.c_str());
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(unary_expression *expr)
{
    const expr_result result = evaluate(expr->right);
    switch (expr->op.type) {
    case token_type::MINUS:
        return negate(result);
    case token_type::BANG:
        return invert(result);
    default:
        return expr_result::error("unknown unary operator " + expr->op.str);
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(literal_expression *expr)
{
    switch (expr->literal.type) {
    case token_type::NUMBER:
        return expr_result::num(to_num(expr->literal.str));
    case token_type::STRING:
        return expr_result::string(expr->literal.str);
    case token_type::BTRUE:
        return expr_result::boolean(true);
    case token_type::BFALSE:
        return expr_result::boolean(false);
    case token_type::NIL:
        return expr_result::nil();
    default:
        // TODO: handle error better than this
        return expr_result::error("unknown literal type");
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
            return possible_value.value();
        }
    }

    return expr_result::error("identifier " + expr->name.str + " is not defined in the current scope");
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

void fk::lang::interpreter::visit(print_statement *stmt)
{
    const expr_result result = evaluate(stmt->expr);
    switch (result.type) {
    case expr_result_type::RT_ERROR:
        fk::log::error("error evaluating expression: '%s'\n", result.err.c_str());
        break;
    case expr_result_type::RT_STRING:
        std::puts(result.str.c_str());
        break;
    case expr_result_type::RT_NUMBER:
        std::printf("%f\n", result.n);
        break;
    case expr_result_type::RT_BOOL:
        std::puts(result.b ? "true" : "false");
        break;
    case expr_result_type::RT_NIL:
        std::puts("nil");
        break;
    default:
        fk::log::error("unhandled expression result type");
        break;
    }
}

void fk::lang::interpreter::visit(expression_statement *stmt)
{
    evaluate(stmt->expr);
}

void fk::lang::interpreter::visit(assignment_statement *stmt)
{
    const expr_result result = evaluate(stmt->initializer);
    if (result.type == expr_result_type::RT_ERROR) {
        fk::log::error("error evaluating expression: '%s'\n", result.err.c_str());
        return;
    }

    // TODO: think about adding scope specifiers (export, local, global) to allow the user
    // to mutate global variables with the same name or to add variables to the environment
    current_scope().assign(stmt->name.str, result);
}

void fk::lang::interpreter::visit(block_statement *stmt)
{
    try {
        enter_new_scope();
        for (const statement_ptr& statement : stmt->statements) {
            execute(statement);
        }
    } catch (...) {
        leave_current_scope();
    }
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

fk::lang::expr_result fk::lang::interpreter::evaluate(expression_ptr& expr) noexcept
{
    return expr->accept(this);
}

void fk::lang::interpreter::execute(const statement_ptr& stmt) noexcept
{
    stmt->accept(this);
}

void fk::lang::interpreter::enter_new_scope()
{
    env_.emplace_back();
}

void fk::lang::interpreter::leave_current_scope()
{
    env_.pop_back();
}

fk::lang::environment& fk::lang::interpreter::current_scope()
{
    return env_.back();
}
