#include <clean/token.h>
#include <clean/log.h>

std::string token_type_str(token_type type)
{
    switch (type) {
    case token_type::KEYWORD:
        return "KEYWORD";
    case token_type::IDENTIFIER:
        return "IDENTIFIER";
    case token_type::EQ:
        return "EQ";
    case token_type::STRING:
        return "STRING";
    case token_type::T_EOF:
        return "EOF";
    case token_type::UNKNOWN:
        return "UNKNOWN";
    default:
        fatal("shouldn't happen");
    }
}

token_t::token_t(std::string str, token_type type)
    : str(std::move(str))
    , type(type)
    {}