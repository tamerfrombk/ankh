#include <fak/lang/expr.h>
#include <fak/lang/token.h>

#include <fak/log.h>

std::string fk::lang::expr_result_type_str(fk::lang::expr_result_type type) noexcept
{
    switch (type) {
    case fk::lang::expr_result_type::RT_EXIT_CODE: return "EXIT_CODE";
    case fk::lang::expr_result_type::RT_STRING:    return "STRING";
    case fk::lang::expr_result_type::RT_NUMBER:    return "NUMBER";
    case fk::lang::expr_result_type::RT_BOOL:      return "BOOL";
    case fk::lang::expr_result_type::RT_NIL:       return "NIL";
    default:                                       
        fk::log::fatal("expr_result_type_str(): unknown expression result type!\n");
    }
}

fk::lang::expr_result fk::lang::expr_result::nil() 
{
    expr_result e;
    e.type = expr_result_type::RT_NIL;

    return e;
}

fk::lang::expr_result fk::lang::expr_result::num(number n) 
{
    expr_result e;
    e.type = expr_result_type::RT_NUMBER;
    e.n  = n;

    return e;
}

fk::lang::expr_result fk::lang::expr_result::string(std::string s) 
{
    expr_result e;
    e.type = expr_result_type::RT_STRING;
    e.str  = std::move(s);

    return e;
}

fk::lang::expr_result fk::lang::expr_result::boolean(bool b) 
{
    expr_result e;
    e.type = expr_result_type::RT_BOOL;
    e.b    = b;

    return e;
}

std::string fk::lang::expr_result::stringify() const noexcept
{
    switch (type) {
    case fk::lang::expr_result_type::RT_STRING:
        return str;
    case fk::lang::expr_result_type::RT_NUMBER:
        return std::to_string(n);
    case fk::lang::expr_result_type::RT_BOOL:
        return b ? "true" : "false";
    case fk::lang::expr_result_type::RT_NIL:
        return "nil";
    default:
        fk::log::fatal("stringify(): unknown expression result type!\n");
    }
}