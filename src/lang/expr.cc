#include <fak/lang/expr.h>
#include <fak/lang/token.h>

#include <fak/log.h>

std::string expr_result_type_str(fk::lang::expr_result_type type) noexcept
{
    switch (type) {
    case fk::lang::expr_result_type::RT_EXIT_CODE: return "RT_EXIT_CODE";
    case fk::lang::expr_result_type::RT_STRING:    return "RT_STRING";
    case fk::lang::expr_result_type::RT_NUMBER:    return "RT_NUMBER";
    case fk::lang::expr_result_type::RT_BOOL:      return "RT_BOOL";
    case fk::lang::expr_result_type::RT_NIL:       return "RT_NIL";
    case fk::lang::expr_result_type::RT_ERROR:     return "RT_ERROR";
    default:                             fk::log::fatal("expr_result_type_str(): unknown expression result type!\n");
    }
}

fk::lang::expr_result_t fk::lang::expr_result_t::nil() 
{
    expr_result_t e;
    e.type = expr_result_type::RT_NIL;

    return e;
}

fk::lang::expr_result_t fk::lang::expr_result_t::num(number_t n) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_NUMBER;
    e.n  = n;

    return e;
}

fk::lang::expr_result_t fk::lang::expr_result_t::stringe(std::string s) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_STRING;
    e.str  = std::move(s);

    return e;
}

fk::lang::expr_result_t fk::lang::expr_result_t::e(std::string s) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_ERROR;
    e.err  = std::move(s);

    return e;
}

fk::lang::expr_result_t fk::lang::expr_result_t::boolean(bool b) 
{
    expr_result_t e;
    e.type = expr_result_type::RT_BOOL;
    e.b    = b;

    return e;
}