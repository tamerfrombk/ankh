#include <ankh/lang/expr.h>
#include <ankh/lang/token.h>
#include <ankh/lang/callable.h>
#include <ankh/lang/types/object.h>

#include <ankh/log.h>
#include <ankh/def.h>

static std::string stringify(const ankh::lang::Array<ankh::lang::ExprResult>& array) noexcept
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

static std::string stringify(const ankh::lang::Dictionary<ankh::lang::ExprResult>& dict) noexcept
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

static std::string stringify(const ankh::lang::ObjectPtr<ankh::lang::ExprResult> & obj) noexcept
{
    ankh_UNUSED(obj);
    
    return "{ object }";
}

std::string ankh::lang::ExprResult::stringify() const noexcept
{
    switch (type) {
    case ankh::lang::ExprResultType::RT_STRING:
        return str;
    case ankh::lang::ExprResultType::RT_NUMBER:
        return std::to_string(n);
    case ankh::lang::ExprResultType::RT_BOOL:
        return b ? "true" : "false";
    case ankh::lang::ExprResultType::RT_CALLABLE:
        return callable->name();
    case ankh::lang::ExprResultType::RT_ARRAY:
        return ::stringify(array);
    case ankh::lang::ExprResultType::RT_DICT:
        return ::stringify(dict);
    case ankh::lang::ExprResultType::RT_NIL:
        return "nil";
    case ankh::lang::ExprResultType::RT_OBJECT:
        return ::stringify(obj);
    default:
        ankh_FATAL("stringify(): unknown expression result type '{}'!", expr_result_type_str(type));
    }
}