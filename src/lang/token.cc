#include <clean/lang/token.h>
#include <clean/log.h>

std::string token_type_str(token_type type)
{
    switch (type) {
    case token_type::IDENTIFIER:
        return "IDENTIFIER";
    case token_type::EQ:
        return "EQ";
    case token_type::EQEQ:
        return "EQEQ";
    case token_type::NEQ:
        return "NEQ";
    case token_type::LT:
        return "LT";
    case token_type::LTE:
        return "LTE";
    case token_type::GT:
        return "GT";
    case token_type::GTE:
        return "GTE";
    case token_type::MINUS:
        return "MINUS";
    case token_type::PLUS:
        return "PLUS";
    case token_type::FSLASH:
        return "FSLASH";
    case token_type::STAR:
        return "STAR";
    case token_type::BANG:
        return "BANG";
    case token_type::LPAREN:
        return "LPAREN";
    case token_type::RPAREN:
        return "RPAREN";
    case token_type::LBRACE:
        return "LBRACE";
    case token_type::RBRACE:
        return "RBRACE";
    case token_type::BTRUE:
        return "BTRUE";
    case token_type::BFALSE:
        return "BFALSE";
    case token_type::NIL:
        return "NIL";
    case token_type::PRINT:
        return "PRINT";
    case token_type::NUMBER:
        return "NUMBER";
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
    : str(std::move(str)), type(type) {}