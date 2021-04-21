#include <clean/lang/expr.h>
#include <clean/lang/token.h>

#include <clean/log.h>

std::string expr_result_type_str(expr_result_type type) noexcept
{
    switch (type) {
    case expr_result_type::RT_EXIT_CODE: return "RT_EXIT_CODE";
    case expr_result_type::RT_STRING: return "RT_STRING";
    case expr_result_type::RT_NUMBER: return "RT_NUMBER";
    case expr_result_type::RT_NIL: return "RT_NIL";
    case expr_result_type::RT_ERROR: return "RT_ERROR";
    default: fatal("expr_result_type_str(): unknown expression result type!\n");
    }
}

expr_result_t expr_result_t::nil() 
{
    expr_result_t e;
    e.type = expr_result_type::RT_NIL;

    return e;
}

expr_result_t expr_result_t::num(number_t n) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_NUMBER;
    e.n  = n;

    return e;
}

expr_result_t expr_result_t::stringe(std::string s) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_STRING;
    e.str  = std::move(s);

    return e;
}

expr_result_t expr_result_t::e(std::string s) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_ERROR;
    e.err  = std::move(s);

    return e;
}