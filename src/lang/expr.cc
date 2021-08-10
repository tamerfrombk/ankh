#include <fak/lang/expr.h>
#include <fak/lang/token.h>
#include <fak/lang/callable.h>
#include <fak/lang/types/object.h>

#include <fak/log.h>
#include <fak/def.h>

static std::string stringify(const fk::lang::Array<fk::lang::ExprResult>& array) noexcept
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

static std::string stringify(const fk::lang::Dictionary<fk::lang::ExprResult>& dict) noexcept
{
    if (dict.empty()) {
        return "{}";
    }

    std::string result = "{";
    for (const auto& [key, value] : dict) {
        result += (key.stringify() + " : " + value.stringify());
        result += ",\n";
    }
    result.pop_back();
    result.pop_back();
    result += "}";

    return result;
}

static std::string stringify(const fk::lang::ObjectPtr<fk::lang::ExprResult> & obj) noexcept
{
    FK_UNUSED(obj);
    
    return "{ object }";
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
    case fk::lang::ExprResultType::RT_DICT:
        return ::stringify(dict);
    case fk::lang::ExprResultType::RT_NIL:
        return "nil";
    case fk::lang::ExprResultType::RT_OBJECT:
        return ::stringify(obj);
    default:
        FK_FATAL("stringify(): unknown expression result type '{}'!", expr_result_type_str(type));
    }
}