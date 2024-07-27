#include <ankh/lang/callable.hpp>
#include <ankh/lang/expr.hpp>
#include <ankh/lang/token.hpp>

#include <ankh/def.hpp>
#include <ankh/log.hpp>

static std::string stringify(const ankh::lang::Array<ankh::lang::ExprResult> &array) noexcept {
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

static std::string stringify(const ankh::lang::Dictionary<ankh::lang::ExprResult> &dict) noexcept {
    if (dict.empty()) {
        return "{}";
    }

    std::string result = "{";
    for (const auto &[key, value] : dict) {
        result += (key.stringify() + " : " + value.stringify());
        result += ",\n";
    }
    result.pop_back();
    result.pop_back();
    result += "}";

    return result;
}

std::string ankh::lang::ExprResult::stringify() const noexcept {
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
    default:
        ANKH_FATAL("stringify(): unknown expression result type '{}'!", expr_result_type_str(type));
    }
}
