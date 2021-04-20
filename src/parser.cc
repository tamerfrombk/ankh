#include <algorithm>
#include <initializer_list>

#include <clean/parser.h>
#include <clean/lexer.h>
#include <clean/token.h>
#include <clean/log.h>
#include <clean/error_handler.h>

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

expression_ptr parser_t::parse_expression()
{
//    debug("PARSER: parsing expression: '%s'\n", lexer_.rest().c_str());
    return parse_equality();
}

expression_ptr parser_t::parse_equality()
{
    static auto eq_ops = { token_type::EQ, token_type::NEQ };

//    debug("PARSER: parsing equality: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = parse_comparison();
    while (match(eq_ops)) {
        token_t op = prev();
        expression_ptr right = parse_comparison();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::parse_comparison()
{
    static auto comp_ops = { token_type::LT, token_type::LTE, token_type::GT, token_type::GTE };

    //debug("PARSER: parsing comparison: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = parse_term();
    while (match(comp_ops)) {
        token_t op = prev();
        expression_ptr right = parse_term();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::parse_term()
{
    static auto term_ops = { token_type::MINUS, token_type::PLUS };

    //debug("PARSER: parsing term: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = parse_factor();
    while (match(term_ops)) {
        token_t op = prev();
        expression_ptr right = parse_factor();
        //debug("PARSER: right hand side parsed with '%s' remaining\n", lexer_.rest().c_str());
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::parse_factor()
{
    static auto factor_ops = { token_type::STAR, token_type::FSLASH };

    //debug("PARSER: parsing factpr: '%s'\n", lexer_.rest().c_str());

    expression_ptr left = parse_unary();
    while (match(factor_ops)) {
        token_t op = prev();
        expression_ptr right = parse_unary();
        left = make_expression<binary_expression_t>(std::move(left), op, std::move(right));
    }

    return left;
}

expression_ptr parser_t::parse_unary()
{
    static auto unary_ops = { token_type::BANG, token_type::MINUS };

//    debug("PARSER: parsing unary: '%s'\n", lexer_.rest().c_str());

    if (match(unary_ops)) {
        token_t op = prev();
        expression_ptr right = parse_unary();
        return make_expression<unary_expression_t>(op, std::move(right));
    }

    return parse_primary();
}

expression_ptr parser_t::parse_primary()
{
    //debug("PARSER: parsing primary: '%s'\n", lexer_.rest().c_str());

    const token_t& tok = curr();
    if (tok.str == "false" || tok.str == "true" || tok.str == "nil") {
        advance();
        return make_expression<literal_expression_t>(tok);
    }

    if (match({token_type::NUMBER, token_type::STRING})) {
        return make_expression<literal_expression_t>(prev());
    } 

    if (match({ token_type::LPAREN })) {
        expression_ptr expr = parse_expression();
        token_t paren = advance();
        if (paren.type != token_type::RPAREN) {
            error_handler_->report_error({"terminating ')' not found"});
        }
        return make_expression<paren_expression_t>(std::move(expr));
    }

    error_handler_->report_error({"expected expression but got " + tok.str});
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