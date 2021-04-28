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

static bool operands_are(expr_result_type type, std::initializer_list<expr_result_t> elems)
{
    return std::all_of(elems.begin(), elems.end(), [=](const expr_result_t& result) {
        return type == result.type;
    });
}

static number_t to_num(const std::string& s) noexcept
{
    char *end;

    number_t n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    // TODO: properly handle error
    return -1;
}

static expr_result_t negate(const expr_result_t& result) noexcept
{
    if (result.type == expr_result_type::RT_NUMBER) {
        return expr_result_t::num(-1 * result.n);
    }
    
    // TODO: improve the error message
    return expr_result_t::e("- operator expects a number expression");
}

static expr_result_t invert(const expr_result_t& result) noexcept
{
    if (result.type == expr_result_type::RT_BOOL) {
        return expr_result_t::boolean(!(result.b));
    }

    // TODO: improve the error message
    return expr_result_t::e("! operator expects a boolean expression");
}

static expr_result_t eqeq(const expr_result_t& left, const expr_result_t& right) noexcept
{
    if (operands_are(expr_result_type::RT_NUMBER, {left, right})) {
        return expr_result_t::boolean(left.n == right.n);
    }

    if (operands_are(expr_result_type::RT_STRING, {left, right})) {
        return expr_result_t::boolean(left.str == right.str);
    }

    if (operands_are(expr_result_type::RT_BOOL, {left, right})) {
        return expr_result_t::boolean(left.b == right.b);
    }

    if (operands_are(expr_result_type::RT_NIL, {left, right})) {
        return expr_result_t::boolean(true);
    }

    // TODO: improve error message
    return expr_result_t::e("unknown overload for (!=) operator");
}

template <class BinaryOperation>
static expr_result_t arithmetic(const expr_result_t& left, const expr_result_t& right, BinaryOperation op) noexcept
{
    if (operands_are(expr_result_type::RT_NUMBER, {left, right})) {
        return expr_result_t::num(op(left.n, right.n));
    }

    // TODO: improve error message
    return expr_result_t::e("unknown overload for arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static expr_result_t plus(const expr_result_t& left, const expr_result_t& right)
{
    if (operands_are(expr_result_type::RT_NUMBER, {left, right})) {
        return expr_result_t::num(left.n + right.n);
    }

    if (operands_are(expr_result_type::RT_STRING, {left, right})) {
        return expr_result_t::stringe(left.str + right.str);
    }

    // TODO: improve error message
    return expr_result_t::e("unknown overload for (+) operator");
}

template <class Compare>
static expr_result_t compare(const expr_result_t& left, const expr_result_t& right, Compare cmp) noexcept
{
    if (operands_are(expr_result_type::RT_NUMBER, {left, right})) {
        return expr_result_t::boolean(cmp(left.n, right.n));
    }

    if (operands_are(expr_result_type::RT_STRING, {left, right})) {
        return expr_result_t::boolean(cmp(left.str, right.str));
    }

    // TODO: improve error message
    return expr_result_t::e("unknown overload for comparison operator");
}

interpreter_t::interpreter_t()
{
    // initialize the global scope
    enter_new_scope();
}

void interpreter_t::interpret(const program_t& program)
{
    for (const auto& stmt : program) {
        execute(stmt);
    }
}

expr_result_t interpreter_t::visit(binary_expression_t *expr)
{
    const expr_result_t left  = evaluate(expr->left);
    const expr_result_t right = evaluate(expr->right);

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
        fatal("unimplemented binary operator '%s'\n", expr->op.str.c_str());
    }
}

expr_result_t interpreter_t::visit(unary_expression_t *expr)
{
    const expr_result_t result = evaluate(expr->right);
    switch (expr->op.type) {
    case token_type::MINUS:
        return negate(result);
    case token_type::BANG:
        return invert(result);
    default:
        return expr_result_t::e("unknown unary operator " + expr->op.str);
    }
}

expr_result_t interpreter_t::visit(literal_expression_t *expr)
{
    switch (expr->literal.type) {
    case token_type::NUMBER:
        return expr_result_t::num(to_num(expr->literal.str));
    case token_type::STRING:
        return expr_result_t::stringe(expr->literal.str);
    case token_type::BTRUE:
        return expr_result_t::boolean(true);
    case token_type::BFALSE:
        return expr_result_t::boolean(false);
    case token_type::NIL:
        return expr_result_t::nil();
    default:
        // TODO: handle error better than this
        return expr_result_t::e("unknown literal type");
    }
}

expr_result_t interpreter_t::visit(paren_expression_t *expr)
{
    return evaluate(expr->expr);
}

expr_result_t interpreter_t::visit(identifier_expression_t *expr)
{
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        auto possible_value = it->value(expr->name.str); 
        if (possible_value.has_value()) {
            return possible_value.value();
        }
    }

    return expr_result_t::e("identifier " + expr->name.str + " is not defined in the current scope");
}

void interpreter_t::visit(print_statement_t *stmt)
{
    const expr_result_t result = evaluate(stmt->expr);
    switch (result.type) {
    case expr_result_type::RT_ERROR:
        error("error evaluating expression: '%s'\n", result.err.c_str());
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
        error("unhandled expression result type");
        break;
    }
}

void interpreter_t::visit(expression_statement_t *stmt)
{
    evaluate(stmt->expr);
}

void interpreter_t::visit(assignment_statement_t *stmt)
{
    const expr_result_t result = evaluate(stmt->initializer);
    if (result.type == expr_result_type::RT_ERROR) {
        error("error evaluating expression: '%s'\n", result.err.c_str());
        return;
    }

    current_scope().assign(stmt->name.str, result);
}

void interpreter_t::visit(block_statement_t *stmt)
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

expr_result_t interpreter_t::evaluate(expression_ptr& expr) noexcept
{
    return expr->accept(this);
}

void interpreter_t::execute(const statement_ptr& stmt) noexcept
{
    stmt->accept(this);
}

void interpreter_t::enter_new_scope()
{
    env_.emplace_back();
}

void interpreter_t::leave_current_scope()
{
    env_.pop_back();
}

environment_t& interpreter_t::current_scope()
{
    return env_.back();
}
