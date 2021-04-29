#include "fak/lang/expr.h"
#include "fak/lang/statement.h"
#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/log.h>

fk::lang::parser::parser(std::string str, error_handler *error_handler)
    : cursor_(0)
    , error_handler_(error_handler)
{
    lexer lexer(str, error_handler_);
    for (token tok = lexer.next_token(); tok.type != fk::lang::token_type::T_EOF; tok = lexer.next_token()) {
        tokens_.push_back(tok);
    }
    tokens_.emplace_back("", fk::lang::token_type::T_EOF);

    for (const auto& tok : tokens_) {
        fk::log::debug("('%s':'%s')\n", fk::lang::token_type_str(tok.type).c_str(), tok.str.c_str());
    }
}

fk::lang::program fk::lang::parser::parse()
{
    program stmts;
    while (!is_eof()) {
        stmts.emplace_back(declaration());    
    }
    return stmts;
}

fk::lang::statement_ptr fk::lang::parser::declaration()
{
    if (match({ fk::lang::token_type::IDENTIFIER })) {
        return assignment();
    }
    return statement();
}

fk::lang::statement_ptr fk::lang::parser::assignment()
{
    token variable = prev();
    if (match({ fk::lang::token_type::EQ })) {
        return make_statement<assignment_statement>(variable, expression());
    }
    
    error_handler_->report_error({"syntax error: '=' expected after identifier in assignment statement"});
    
    // TODO: what to do here??
    return nullptr;
}

fk::lang::statement_ptr fk::lang::parser::statement()
{
    if (match({ fk::lang::token_type::PRINT })) {
        return make_statement<print_statement>(expression());
    }
    if (check(fk::lang::token_type::LBRACE)) {
        return block();
    }
    if (match({ fk::lang::token_type::IF })) {
        return parse_if();
    }

    return make_statement<expression_statement>(expression());
}

fk::lang::statement_ptr fk::lang::parser::block()
{
    if (!match({ fk::lang::token_type::LBRACE })) {
        error_handler_->report_error({"expect '{' to start block"});
        return nullptr;
    }

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

    return make_statement<block_statement>(std::move(statements));
}

fk::lang::statement_ptr fk::lang::parser::parse_if()
{
    expression_ptr condition = expression();
    statement_ptr then_block = block();

    statement_ptr else_block = nullptr;
    if (match({ fk::lang::token_type::ELSE })) {
        else_block = block();
    }

    return make_statement<if_statement>(std::move(condition), std::move(then_block), std::move(else_block));
}

fk::lang::expression_ptr fk::lang::parser::expression()
{
    return parse_or();
}

fk::lang::expression_ptr fk::lang::parser::parse_or()
{
    fk::lang::expression_ptr left = parse_and();
    while (match({ fk::lang::token_type::OR })) {
        fk::lang::expression_ptr right = parse_and();
        left = make_expression<fk::lang::or_expression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::parse_and()
{
    fk::lang::expression_ptr left = equality();
    while (match({ fk::lang::token_type::AND })) {
        fk::lang::expression_ptr right = equality();
        left = make_expression<fk::lang::and_expression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::equality()
{
    static auto eq_ops = { fk::lang::token_type::EQEQ, fk::lang::token_type::NEQ };

    fk::lang::expression_ptr left = comparison();
    while (match(eq_ops)) {
        token op = prev();
        fk::lang::expression_ptr right = comparison();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::comparison()
{
    static auto comp_ops = { fk::lang::token_type::LT, fk::lang::token_type::LTE, fk::lang::token_type::GT, fk::lang::token_type::GTE };

    fk::lang::expression_ptr left = term();
    while (match(comp_ops)) {
        token op = prev();
        fk::lang::expression_ptr right = term();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::term()
{
    static auto term_ops = { fk::lang::token_type::MINUS, fk::lang::token_type::PLUS };

    fk::lang::expression_ptr left = factor();
    while (match(term_ops)) {
        token op = prev();
        fk::lang::expression_ptr right = factor();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::factor()
{
    static auto factor_ops = { fk::lang::token_type::STAR, fk::lang::token_type::FSLASH };

    fk::lang::expression_ptr left = unary();
    while (match(factor_ops)) {
        token op = prev();
        fk::lang::expression_ptr right = unary();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::unary()
{
    static auto unary_ops = { fk::lang::token_type::BANG, fk::lang::token_type::MINUS };

    if (match(unary_ops)) {
        token op = prev();
        fk::lang::expression_ptr right = unary();
        return make_expression<unary_expression>(op, std::move(right));
    }

    return primary();
}

fk::lang::expression_ptr fk::lang::parser::primary()
{
    if (match({ fk::lang::token_type::NUMBER
            , fk::lang::token_type::STRING
            , fk::lang::token_type::BTRUE
            , fk::lang::token_type::BFALSE
            , fk::lang::token_type::NIL
        })) 
    {
        return make_expression<literal_expression>(prev());
    }

    if (match({ fk::lang::token_type::IDENTIFIER })) {
        return make_expression<identifier_expression>(prev());
    }

    if (match({ fk::lang::token_type::LPAREN })) {
        fk::lang::expression_ptr expr = expression();
        token paren = advance();
        if (paren.type != fk::lang::token_type::RPAREN) {
            error_handler_->report_error({"terminating ')' not found"});
        }
        return make_expression<paren_expression>(std::move(expr));
    }

    // TODO: improve error message
    error_handler_->report_error({"expected expression"});

    // TODO: what to do here?
    return nullptr;
}

const fk::lang::token& fk::lang::parser::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const fk::lang::token& fk::lang::parser::curr() const noexcept
{
    return tokens_[cursor_];
}

const fk::lang::token& fk::lang::parser::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool fk::lang::parser::is_eof() const noexcept
{
    return curr().type == fk::lang::token_type::T_EOF;
}

bool fk::lang::parser::match(std::initializer_list<fk::lang::token_type> types) noexcept
{
    for (fk::lang::token_type type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

bool fk::lang::parser::check(fk::lang::token_type type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}