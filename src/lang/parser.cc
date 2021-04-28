#include "fak/lang/expr.h"
#include "fak/lang/statement.h"
#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/log.h>

fk::lang::parser_t::parser_t(std::string str, error_handler_t *error_handler)
    : cursor_(0)
    , error_handler_(error_handler)
{
    lexer_t lexer(str, error_handler_);
    for (token_t tok = lexer.next_token(); tok.type != fk::lang::token_type::T_EOF; tok = lexer.next_token()) {
        tokens_.push_back(tok);
    }
    tokens_.emplace_back("", fk::lang::token_type::T_EOF);

    for (const auto& tok : tokens_) {
        fk::log::debug("('%s':'%s')\n", fk::lang::token_type_str(tok.type).c_str(), tok.str.c_str());
    }
}

fk::lang::program_t fk::lang::parser_t::parse()
{
    program_t stmts;
    while (!is_eof()) {
        stmts.emplace_back(declaration());    
    }
    return stmts;
}

fk::lang::statement_ptr fk::lang::parser_t::declaration()
{
    if (match({ fk::lang::token_type::IDENTIFIER })) {
        return assignment();
    }
    return statement();
}

fk::lang::statement_ptr fk::lang::parser_t::assignment()
{
    token_t variable = prev();
    if (match({ fk::lang::token_type::EQ })) {
        return make_statement<assignment_statement_t>(variable, expression());
    }
    
    error_handler_->report_error({"syntax error: '=' expected after identifier in assignment statement"});
    
    // TODO: what to do here??
    return nullptr;
}

fk::lang::statement_ptr fk::lang::parser_t::statement()
{
    if (match({ fk::lang::token_type::PRINT })) {
        return print_statement();
    }
    if (match({ fk::lang::token_type::LBRACE })) {
        return block();
    }

    return expression_statement();
}

fk::lang::statement_ptr fk::lang::parser_t::print_statement()
{
    return make_statement<print_statement_t>(expression());
}

fk::lang::statement_ptr fk::lang::parser_t::expression_statement()
{
    return make_statement<expression_statement_t>(expression());
}

fk::lang::statement_ptr fk::lang::parser_t::block()
{
    // TODO: reserve some room ahead of time for the statements
    std::vector<fk::lang::statement_ptr> statements;
    while (!check(fk::lang::token_type::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration());
    }

    if (!match({ fk::lang::token_type::RBRACE })) {
        error_handler_->report_error({"expect '}' to terminate block"});
        // TODO: handle error
        return nullptr;
    }

    return make_statement<block_statement_t>(std::move(statements));
}

fk::lang::expression_ptr fk::lang::parser_t::expression()
{
    return equality();
}

fk::lang::expression_ptr fk::lang::parser_t::equality()
{
    static auto eq_ops = { fk::lang::token_type::EQEQ, fk::lang::token_type::NEQ };

    fk::lang::expression_ptr left = comparison();
    while (match(eq_ops)) {
        token_t op = prev();
        fk::lang::expression_ptr right = comparison();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser_t::comparison()
{
    static auto comp_ops = { fk::lang::token_type::LT, fk::lang::token_type::LTE, fk::lang::token_type::GT, fk::lang::token_type::GTE };

    fk::lang::expression_ptr left = term();
    while (match(comp_ops)) {
        token_t op = prev();
        fk::lang::expression_ptr right = term();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser_t::term()
{
    static auto term_ops = { fk::lang::token_type::MINUS, fk::lang::token_type::PLUS };

    fk::lang::expression_ptr left = factor();
    while (match(term_ops)) {
        token_t op = prev();
        fk::lang::expression_ptr right = factor();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser_t::factor()
{
    static auto factor_ops = { fk::lang::token_type::STAR, fk::lang::token_type::FSLASH };

    fk::lang::expression_ptr left = unary();
    while (match(factor_ops)) {
        token_t op = prev();
        fk::lang::expression_ptr right = unary();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser_t::unary()
{
    static auto unary_ops = { fk::lang::token_type::BANG, fk::lang::token_type::MINUS };

    if (match(unary_ops)) {
        token_t op = prev();
        fk::lang::expression_ptr right = unary();
        return make_expression<unary_expression_t>(op, std::move(right));
    }

    return primary();
}

fk::lang::expression_ptr fk::lang::parser_t::primary()
{
    if (match({ fk::lang::token_type::NUMBER
            , fk::lang::token_type::STRING
            , fk::lang::token_type::BTRUE
            , fk::lang::token_type::BFALSE
            , fk::lang::token_type::NIL
        })) 
    {
        return make_expression<literal_expression_t>(prev());
    }

    if (match({ fk::lang::token_type::IDENTIFIER })) {
        return make_expression<identifier_expression_t>(prev());
    }

    if (match({ fk::lang::token_type::LPAREN })) {
        fk::lang::expression_ptr expr = expression();
        token_t paren = advance();
        if (paren.type != fk::lang::token_type::RPAREN) {
            error_handler_->report_error({"terminating ')' not found"});
        }
        return make_expression<paren_expression_t>(std::move(expr));
    }

    // TODO: improve error message
    error_handler_->report_error({"expected expression"});

    // TODO: what to do here?
    return nullptr;
}

const fk::lang::token_t& fk::lang::parser_t::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const fk::lang::token_t& fk::lang::parser_t::curr() const noexcept
{
    return tokens_[cursor_];
}

const fk::lang::token_t& fk::lang::parser_t::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool fk::lang::parser_t::is_eof() const noexcept
{
    return curr().type == fk::lang::token_type::T_EOF;
}

bool fk::lang::parser_t::match(std::initializer_list<fk::lang::token_type> types) noexcept
{
    for (fk::lang::token_type type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

bool fk::lang::parser_t::check(fk::lang::token_type type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}