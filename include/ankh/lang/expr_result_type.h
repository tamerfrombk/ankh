#pragma once

namespace ankh::lang {

enum class ExprResultType {
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_CALLABLE,
    RT_ARRAY,
    RT_DICT,
    RT_NIL
};

}