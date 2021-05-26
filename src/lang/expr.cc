#include <fak/lang/expr.h>
#include <fak/lang/token.h>

#include <fak/log.h>

std::string fk::lang::expr_result_type_str(fk::lang::ExprResultType type) noexcept
{
    switch (type) {
    case fk::lang::ExprResultType::RT_EXIT_CODE: return "EXIT_CODE";
    case fk::lang::ExprResultType::RT_STRING:    return "STRING";
    case fk::lang::ExprResultType::RT_NUMBER:    return "NUMBER";
    case fk::lang::ExprResultType::RT_BOOL:      return "BOOL";
    case fk::lang::ExprResultType::RT_NIL:       return "NIL";
    default:                                       
        fk::log::fatal("expr_result_type_str(): unknown expression result type!\n");
    }
}

fk::lang::ExprResult fk::lang::ExprResult::nil() 
{
    ExprResult e;
    e.type = ExprResultType::RT_NIL;

    return e;
}

fk::lang::ExprResult fk::lang::ExprResult::num(Number n) 
{
    ExprResult e;
    e.type = ExprResultType::RT_NUMBER;
    e.n  = n;

    return e;
}

fk::lang::ExprResult fk::lang::ExprResult::string(std::string s) 
{
    ExprResult e;
    e.type = ExprResultType::RT_STRING;
    e.str  = std::move(s);

    return e;
}

fk::lang::ExprResult fk::lang::ExprResult::boolean(bool b) 
{
    ExprResult e;
    e.type = ExprResultType::RT_BOOL;
    e.b    = b;

    return e;
}

std::string fk::lang::ExprResult::stringify() const noexcept
{
    switch (type) {
    case fk::lang::ExprResultType::RT_STRING:
        return str;
    case fk::lang::ExprResultType::RT_NUMBER:
        return std::to_string(n);
    case fk::lang::ExprResultType::RT_BOOL:
        return b ? "true" : "false";
    case fk::lang::ExprResultType::RT_NIL:
        return "nil";
    default:
        fk::log::fatal("stringify(): unknown expression result type!\n");
    }
}