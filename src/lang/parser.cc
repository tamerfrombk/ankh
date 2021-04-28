#include "fak/lang/expr.h"
#include "fak/lang/statement.h"
#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/log.h>

parser_t::parser_t(std::string str, error_handler_t *error_handler)
    : cursor_(0)
    , error_handler_(error_handler)
{
    lexer_t lexer(str, error_handler_);
    for (token_t tok = lexer.next_token(); tok.type != token_type::T_EOF; tok = lexer.next_token()) {
        tokens_.push_back(tok);
    }
    tokens_.emplace_back("", token_type::T_EOF);

    for (const auto& tok : tokens_) {
        debug("('%s':'%s')\n", token_type_str(tok.type).c_str(), tok.str.c_str());
    }
}

program_t parser_t::parse()
{
    program_t stmts;
    while (!is_eof()) {
        stmts.emplace_back(declaration());    
    }
    return stmts;
}

statement_ptr parser_t::declaration()
{
    if (match({ token_type::IDENTIFIER })) {
        return assignment();
    }
    return statement();
}

statement_ptr parser_t::assignment()
{
    token_t variable = prev();
    if (match({ token_type::EQ })) {
        return make_statement<assignment_statement_t>(variable, expression());
    }
    
    error_handler_->report_error({"syntax error: '=' expected after identifier in assignment statement"});
    
    // TODO: what to do here??
    return nullptr;
}

statement_ptr parser_t::statement()
{
    if (match({ token_type::PRINT })) {
        return print_statement();
    }
    if (match({ token_type::LBRACE })) {
        return block();
    }

    return expression_statement();
}

statement_ptr parser_t::print_statement()
{
    return make_statement<print_statement_t>(expression());
}

statement_ptr parser_t::expression_statement()
{
    return make_statement<expression_statement_t>(expression());
}

statement_ptr parser_t::block()
{
    // TODO: reserve some room ahead of time for the statements
    std::vector<statement_ptr> statements;
    while (!check(token_type::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration());
    }

    if (!match({ token_type::RBRACE })) {
        error_handler_->report_error({"expect '}' to terminate block"});
        // TODO: handle error
        return nullptr;
    }

    return make_statement<block_statement_t>(std::move(statements));
}

expression_ptr parser_t::expression()
{
//    debug("PARSER: parsing expression: '%s'\n", lexer_.rest().c_str());
    return equality();
}

expression_ptr parser_t::equality()
{
    static auto eq_ops = { token_type::EQEQ, token_type::NEQ };

//    debug("PARSER: parsing equality: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = comparison();
    while (match(eq_ops)) {
        token_t op = prev();
        expression_ptr right = comparison();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::comparison()
{
    static auto comp_ops = { token_type::LT, token_type::LTE, token_type::GT, token_type::GTE };

    //debug("PARSER: parsing comparison: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = term();
    while (match(comp_ops)) {
        token_t op = prev();
        expression_ptr right = term();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::term()
{
    static auto term_ops = { token_type::MINUS, token_type::PLUS };

    //debug("PARSER: parsing term: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = factor();
    while (match(term_ops)) {
        token_t op = prev();
        expression_ptr right = factor();
        //debug("PARSER: right hand side parsed with '%s' remaining\n", lexer_.rest().c_str());
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::factor()
{
    static auto factor_ops = { token_type::STAR, token_type::FSLASH };

    //debug("PARSER: parsing factpr: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = unary();
    while (match(factor_ops)) {
        token_t op = prev();
        expression_ptr right = unary();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::unary()
{
    static auto unary_ops = { token_type::BANG, token_type::MINUS };

//    debug("PARSER: parsing unary: '%s'\n", lexer_.rest().c_str());

    if (match(unary_ops)) {
        token_t op = prev();
        expression_ptr right = unary();
        return make_expression<unary_expression_t>(op, std::move(right));
    }

    return primary();
}

expression_ptr parser_t::primary()
{
    //debug("PARSER: parsing primary: '%s'\n", lexer_.rest().c_str());
    if (match({ token_type::NUMBER
            , token_type::STRING
            , token_type::BTRUE
            , token_type::BFALSE
            , token_type::NIL
        })) 
    {
        return make_expression<literal_expression_t>(prev());
    }

    if (match({ token_type::IDENTIFIER })) {
        return make_expression<identifier_expression_t>(prev());
    }

    if (match({ token_type::LPAREN })) {
        expression_ptr expr = expression();
        token_t paren = advance();
        if (paren.type != token_type::RPAREN) {
            error_handler_->report_error({"terminating ')' not found"});
        }
        return make_expression<paren_expression_t>(std::move(expr));
    }

    // TODO: improve error message
    error_handler_->report_error({"expected expression"});

    // TODO: what to do here?
    return nullptr;
}

const token_t& parser_t::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const token_t& parser_t::curr() const noexcept
{
    return tokens_[cursor_];
}

const token_t& parser_t::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool parser_t::is_eof() const noexcept
{
    return curr().type == token_type::T_EOF;
}

bool parser_t::match(std::initializer_list<token_type> types) noexcept
{
    for (token_type type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

bool parser_t::check(token_type type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}