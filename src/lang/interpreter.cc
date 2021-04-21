#include <cstdlib>

#include <clean/lang/interpreter.h>
#include <clean/lang/token.h>
#include <clean/lang/expr.h>
#include <clean/log.h>

expr_result_t interpreter_t::visit(binary_expression_t *expr)
{
    const expr_result_t left  = expr->left->accept(this);
    const expr_result_t right = expr->right->accept(this);

    // TODO: implement other binary operators
    switch (expr->op.type) {
    case token_type::PLUS:
        if (left.type == expr_result_type::RT_NUMBER && right.type == expr_result_type::RT_NUMBER) {
            return expr_result_t::num(left.n + right.n);
        }
        if (left.type == expr_result_type::RT_STRING && right.type == expr_result_type::RT_STRING) {
            return expr_result_t::stringe(left.str + right.str);
        }
        return expr_result_t::e("unknown overload for (+) operator ");
    default:
        fatal("unimplemented binary operator '%s'\n", expr->op.str.c_str());
    }
}

expr_result_t interpreter_t::visit(unary_expression_t *expr)
{
    const expr_result_t result = expr->right->accept(this);
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
    return expr->expr->accept(this);
}

number_t interpreter_t::to_num(const std::string& s) const noexcept
{
    char *end;

    number_t n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    // TODO: properly handle error
    return -1;
}

expr_result_t interpreter_t::negate(const expr_result_t& result) const noexcept
{
    if (result.type == expr_result_type::RT_NUMBER) {
        return expr_result_t::num(-1 * result.n);
    }
    
    // TODO: improve the error message
    return expr_result_t::e("- operator expects a number expression");
}

expr_result_t interpreter_t::invert(const expr_result_t& result) const noexcept
{
    if (result.type == expr_result_type::RT_BOOL) {
        return expr_result_t::boolean(!(result.b));
    }

    // TODO: improve the error message
    return expr_result_t::e("! operator expects a boolean expression");
}
