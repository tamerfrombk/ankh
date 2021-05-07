#include "fak/lang/interpreter.h"
#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/lang/parse_exception.h>
#include <fak/log.h>

fk::lang::parser::parser(std::string str, error_handler *error_handler)
    : cursor_(0)
    , error_handler_(error_handler)
{
    lexer lexer(str, error_handler_);
    for (token tok = lexer.next_token(); tok.type != fk::lang::token_type::T_EOF; tok = lexer.next_token()) {
        tokens_.push_back(tok);
    }
    tokens_.push_back(lexer.next_token());

    for (const auto& tok : tokens_) {
        fk::log::debug("('%s':'%s')\n", fk::lang::token_type_str(tok.type).c_str(), tok.str.c_str());
    }
}

fk::lang::program fk::lang::parser::parse() noexcept
{
    program stmts;
    while (!is_eof()) {
        try {
            stmts.emplace_back(declaration());    
        } catch (const fk::lang::parse_exception& e) {
            error_handler_->report_error({e.what()});
            synchronize_next_statement();
        }
    }

    return stmts;
}

fk::lang::statement_ptr fk::lang::parser::declaration()
{
    if (match({ fk::lang::token_type::DEF })) {
        return parse_function_declaration();
    }

    return statement();
}

fk::lang::statement_ptr fk::lang::parser::parse_variable_declaration()
{
    consume(token_type::LET, "'let' expected");
    consume(token_type::IDENTIFIER, "identifier expected");

    token identifier = prev();

    consume(token_type::EQ, "'=' expected");

    return make_statement<variable_declaration>(identifier, expression());  
} 

fk::lang::statement_ptr fk::lang::parser::parse_function_declaration()
{
    const token name = consume(token_type::IDENTIFIER, "<identifier> expected as function name");

    consume(token_type::LPAREN, "'(' expected to start function declaration parameters");

    std::vector<token> params;
    if (!check(token_type::RPAREN)) {
        do {
            token param = consume(token_type::IDENTIFIER, "<identifier> expected in function parameters");
            params.push_back(std::move(param));        
        } while (match({ token_type::COMMA }));
    }

    consume(token_type::RPAREN, "')' expected to terminate function declaration parameters");

    statement_ptr body = block();

    return make_statement<function_declaration>(std::move(name), std::move(params), std::move(body));
}

fk::lang::statement_ptr fk::lang::parser::assignment()
{
    consume(fk::lang::token_type::IDENTIFIER, "identifier expected");

    token identifier = prev();

    consume(fk::lang::token_type::EQ, "'=' expected");

    return make_statement<assignment_statement>(identifier, expression());   
}

fk::lang::statement_ptr fk::lang::parser::statement()
{
    if (match({ fk::lang::token_type::PRINT })) {
        return make_statement<print_statement>(expression());
    }

    // NOTE: we check instead of matching here so we can consume the left brace __in__ block()
    // This allows us to simply call block() whenever we need to parse a block e.g. in while statements
    if (check(fk::lang::token_type::LBRACE)) {
        return block();
    }

    if (match({ fk::lang::token_type::IF })) {
        return parse_if();
    }
    if (match({ fk::lang::token_type::WHILE })) {
        return parse_while();
    }
    if (match({ fk::lang::token_type::FOR })) {
        return parse_for();
    }

    if (check(fk::lang::token_type::LET)) {
        return parse_variable_declaration();
    }

    if (check(fk::lang::token_type::IDENTIFIER)) {
        return assignment();
    }

    return make_statement<expression_statement>(expression());
}

fk::lang::statement_ptr fk::lang::parser::block()
{
    consume(fk::lang::token_type::LBRACE, "'{' expected to start block");

    // TODO: reserve some room ahead of time for the statements
    std::vector<fk::lang::statement_ptr> statements;
    while (!check(fk::lang::token_type::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration()); 
    }

    consume(fk::lang::token_type::RBRACE, "'}' expected to terminate block");

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

fk::lang::statement_ptr fk::lang::parser::parse_while()
{
    expression_ptr condition = expression();
    statement_ptr body = block();

    return make_statement<while_statement>(std::move(condition), std::move(body));
}

fk::lang::statement_ptr fk::lang::parser::parse_for()
{
    statement_ptr init;    
    if (match({ fk::lang::token_type::SEMICOLON })) {
        init = nullptr;
    } else {
        init = parse_variable_declaration();
        consume(fk::lang::token_type::SEMICOLON, "';' expected after initializer statement");
    }

    expression_ptr condition;
    if (match({ fk::lang::token_type::SEMICOLON })) {
        const token& semicolon = prev();
        // if there is no condition, we borrow from C and assume the condition is always true
        condition = make_expression<literal_expression>(token{"true", fk::lang::token_type::BTRUE, semicolon.line, semicolon.inline_pos});
    } else {
        condition = expression();
        consume(fk::lang::token_type::SEMICOLON, "';' expected after condition expression");
    }

    statement_ptr mutator = check(fk::lang::token_type::LBRACE)
        ? nullptr
        : assignment();

    statement_ptr body = block();

    return desugar_for_into_while(std::move(init), std::move(condition), std::move(mutator), std::move(body));
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
        
        consume(fk::lang::token_type::RPAREN, "terminating ')' in parenthetic expression expected");

        return make_expression<paren_expression>(std::move(expr));
    }

    throw parse_exception("primary expression expected");
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

fk::lang::token fk::lang::parser::consume(token_type type, const std::string& msg)
{
    if (!match({ type })) {
        const token& current = curr();
        std::string error_message("syntax error: " + msg + " instead of '" + current.str + "'");
        throw fk::lang::parse_exception(error_message);
    }

    return prev();
}

void fk::lang::parser::synchronize_next_statement() noexcept
{
    // TODO: actually synchronize to the next statement
    advance();
}

// desugar for loop into while loop
// basically, this turns something like:
// for i = 0; i < 10; i = i + 1 { 
// /* for-loop-body */
// }
// into:
// {
//      i = 0
//      while i < 10 {
//      {
//       /* for-loop-body */
//      }
//      i = i + 1
//      }
// }
fk::lang::statement_ptr fk::lang::parser::desugar_for_into_while(
    fk::lang::statement_ptr init
    , fk::lang::expression_ptr condition
    , fk::lang::statement_ptr mutator
    , fk::lang::statement_ptr body
    ) noexcept
{
    std::vector<statement_ptr> while_body_statements;
    while_body_statements.push_back(std::move(body));
    if (mutator != nullptr) {
        while_body_statements.push_back(std::move(mutator));
    }

    auto while_body = make_statement<block_statement>(std::move(while_body_statements));
    auto while_stmt = make_statement<while_statement>(std::move(condition), std::move(while_body)); 

    std::vector<statement_ptr> statements;
    if (init != nullptr) {
        statements.push_back(std::move(init));
    }
    statements.push_back(std::move(while_stmt));
    
    return make_statement<block_statement>(std::move(statements));
}