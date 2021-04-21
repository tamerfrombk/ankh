#include <cstdlib>
#include <algorithm>
#include <initializer_list>
#include <functional>

#include <clean/lang/interpreter.h>
#include <clean/lang/token.h>
#include <clean/lang/expr.h>
#include <clean/log.h>

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

static expr_result_t plus(const expr_result_t& left, const expr_result_t& right) noexcept
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
    case token_type::PLUS:
        return plus(left, right);
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

expr_result_t interpreter_t::evaluate(expression_ptr& expr) noexcept
{
    return expr->accept(this);
}
