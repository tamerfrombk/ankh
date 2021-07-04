#pragma once

#include <string>

#include <fak/lang/types/dictionary.h>
#include <fak/lang/types/array.h>

#include <fak/log.h>

namespace fk::lang {

using Number = double;

struct Callable;

enum class ExprResultType {
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_CALLABLE,
    RT_ARRAY,
    RT_DICT,
    RT_NIL
};

inline std::string expr_result_type_str(fk::lang::ExprResultType type) noexcept
{
    switch (type) {
    case fk::lang::ExprResultType::RT_STRING:    return "STRING";
    case fk::lang::ExprResultType::RT_NUMBER:    return "NUMBER";
    case fk::lang::ExprResultType::RT_BOOL:      return "BOOL";
    case fk::lang::ExprResultType::RT_CALLABLE:  return "RT_CALLABLE";
    case fk::lang::ExprResultType::RT_ARRAY:     return "RT_ARRAY";
    case fk::lang::ExprResultType::RT_DICT:      return "RT_DICT";
    case fk::lang::ExprResultType::RT_NIL:       return "NIL";
    default:                                       
        FK_FATAL("expr_result_type_str(): unknown expression result type!");
    }
}

struct ExprResult {

    std::string str;
    union {
        Number      n;
        bool        b;
        Callable    *callable;
    };
    
    Array<ExprResult> array;
    Dictionary<ExprResult> dict;
    ExprResultType type;

    ExprResult()                   :                      type(ExprResultType::RT_NIL) {}
    ExprResult(std::string str)    : str(std::move(str)), type(ExprResultType::RT_STRING) {}
    ExprResult(Number n)           : n(n)               , type(ExprResultType::RT_NUMBER) {}
    ExprResult(bool b)             : b(b)               , type(ExprResultType::RT_BOOL) {}
    ExprResult(Callable *callable) : callable(callable) , type(ExprResultType::RT_CALLABLE) {}
    ExprResult(Array<ExprResult> array)        : array(array)       , type(ExprResultType::RT_ARRAY) {}
    ExprResult(Dictionary<ExprResult> dict)    : dict(dict)         , type(ExprResultType::RT_DICT) {}

    std::string stringify() const noexcept;

    friend bool operator==(const ExprResult& lhs, const ExprResult& rhs) noexcept
    {
        if (lhs.type != rhs.type) {
            return false;
        }

        switch (lhs.type)
        {
        case ExprResultType::RT_NIL:      return true;
        case ExprResultType::RT_STRING:   return lhs.str == rhs.str;
        case ExprResultType::RT_NUMBER:   return lhs.n == rhs.n;
        case ExprResultType::RT_BOOL:     return lhs.b == rhs.b;
        case ExprResultType::RT_CALLABLE: return lhs.callable == rhs.callable;
        case ExprResultType::RT_ARRAY:    return lhs.array == rhs.array;
        case ExprResultType::RT_DICT:     return lhs.dict  == rhs.dict;
        default: FK_FATAL("unknown expression result type");
        }
    }

    friend bool operator!=(const ExprResult& lhs, const ExprResult& rhs) noexcept
    {
        return !(operator==(lhs, rhs));
    }
};

}
