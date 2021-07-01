#include <fak/lang/expr.h>
#include <fak/lang/token.h>
#include <fak/lang/callable.h>

#include <fak/log.h>

std::string fk::lang::expr_result_type_str(fk::lang::ExprResultType type) noexcept
{
    switch (type) {
    case fk::lang::ExprResultType::RT_STRING:    return "STRING";
    case fk::lang::ExprResultType::RT_NUMBER:    return "NUMBER";
    case fk::lang::ExprResultType::RT_BOOL:      return "BOOL";
    case fk::lang::ExprResultType::RT_CALLABLE:  return "RT_CALLABLE";
    case fk::lang::ExprResultType::RT_ARRAY:     return "RT_ARRAY";
    case fk::lang::ExprResultType::RT_NIL:       return "NIL";
    default:                                       
        FK_FATAL("expr_result_type_str(): unknown expression result type!");
    }
}

static std::string stringify(const fk::lang::Array& array) noexcept
{
    if (array.empty()) {
        return "[]";
    }

    std::string result = "[" + array[0].stringify();
    for (size_t i = 1; i < array.size(); ++i) {
        result += ", ";
        result += array[i].stringify();
    }
    result += "]";

    return result;
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
    case fk::lang::ExprResultType::RT_CALLABLE:
        return callable->name();
    case fk::lang::ExprResultType::RT_ARRAY:
        return ::stringify(array);
    case fk::lang::ExprResultType::RT_NIL:
        return "nil";
    default:
        FK_FATAL("stringify(): unknown expression result type '{}'!", expr_result_type_str(type));
    }
}