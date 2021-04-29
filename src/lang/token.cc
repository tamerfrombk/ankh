#include <fak/lang/token.h>
#include <fak/log.h>

std::string fk::lang::token_type_str(fk::lang::token_type type)
{
    switch (type) {
    case fk::lang::token_type::IDENTIFIER:
        return "IDENTIFIER";
    case fk::lang::token_type::EQ:
        return "EQ";
    case fk::lang::token_type::EQEQ:
        return "EQEQ";
    case fk::lang::token_type::NEQ:
        return "NEQ";
    case fk::lang::token_type::LT:
        return "LT";
    case fk::lang::token_type::LTE:
        return "LTE";
    case fk::lang::token_type::GT:
        return "GT";
    case fk::lang::token_type::GTE:
        return "GTE";
    case fk::lang::token_type::MINUS:
        return "MINUS";
    case fk::lang::token_type::PLUS:
        return "PLUS";
    case fk::lang::token_type::FSLASH:
        return "FSLASH";
    case fk::lang::token_type::STAR:
        return "STAR";
    case fk::lang::token_type::BANG:
        return "BANG";
    case fk::lang::token_type::LPAREN:
        return "LPAREN";
    case fk::lang::token_type::RPAREN:
        return "RPAREN";
    case fk::lang::token_type::LBRACE:
        return "LBRACE";
    case fk::lang::token_type::RBRACE:
        return "RBRACE";
    case fk::lang::token_type::BTRUE:
        return "BTRUE";
    case fk::lang::token_type::BFALSE:
        return "BFALSE";
    case fk::lang::token_type::NIL:
        return "NIL";
    case fk::lang::token_type::PRINT:
        return "PRINT";
    case fk::lang::token_type::IF:
        return "IF";
    case fk::lang::token_type::ELSE:
        return "ELSE";
    case fk::lang::token_type::AND:
        return "AND";
    case fk::lang::token_type::OR:
        return "OR";
    case fk::lang::token_type::WHILE:
        return "WHILE";
    case fk::lang::token_type::NUMBER:
        return "NUMBER";
    case fk::lang::token_type::STRING:
        return "STRING";
    case fk::lang::token_type::T_EOF:
        return "EOF";
    case fk::lang::token_type::UNKNOWN:
        return "UNKNOWN";
    default:
        fk::log::fatal("shouldn't happen");
    }
}

fk::lang::token::token(std::string str, fk::lang::token_type type)
    : str(std::move(str)), type(type) {}