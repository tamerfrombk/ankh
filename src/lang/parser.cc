#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/lang/parse_exception.h>
#include <fak/log.h>

template <class ExpectedType>
ExpectedType* instanceof(const fk::lang::expression_ptr& expr) noexcept
{
    return dynamic_cast<ExpectedType*>(expr.get());
}

fk::lang::parser::parser(std::string str, ErrorHandler *error_handler)
    : cursor_(0)
    , error_handler_(error_handler)
{
    Lexer lexer(str, error_handler_);
    for (Token tok = lexer.next(); tok.type != fk::lang::TokenType::FK_EOF; tok = lexer.next()) {
        tokens_.push_back(tok);
    }
    tokens_.push_back(lexer.next());

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
    if (match({ fk::lang::TokenType::DEF })) {
        return parse_function_declaration();
    }

    return statement();
}

fk::lang::statement_ptr fk::lang::parser::parse_variable_declaration(expression_ptr target)
{
    identifier_expression *identifier = instanceof<identifier_expression>(target);
    if (identifier == nullptr) {
        throw parse_exception("invalid variable declaration target");
    }

    consume(TokenType::WALRUS, "':=' expected in variable declaration");

    expression_ptr rhs = expression();

    return make_statement<variable_declaration>(identifier->name, std::move(rhs));  
} 

fk::lang::statement_ptr fk::lang::parser::parse_function_declaration()
{
    const Token name = consume(TokenType::IDENTIFIER, "<identifier> expected as function name");

    consume(TokenType::LPAREN, "'(' expected to start function declaration parameters");

    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token param = consume(TokenType::IDENTIFIER, "<identifier> expected in function parameters");
            params.push_back(std::move(param));        
        } while (match({ TokenType::COMMA }));
    }

    consume(TokenType::RPAREN, "')' expected to terminate function declaration parameters");

    statement_ptr body = block();

    return make_statement<function_declaration>(std::move(name), std::move(params), std::move(body));
}

fk::lang::statement_ptr fk::lang::parser::assignment(expression_ptr target)
{
    identifier_expression *identifier = instanceof<identifier_expression>(target);
    if (identifier == nullptr) {
        throw parse_exception("invalid assignment target");
    }

    if (match({ TokenType::PLUSEQ, TokenType::MINUSEQ, TokenType::STAREQ, TokenType::FSLASHEQ })) {
        const Token& op = prev();
        expression_ptr rhs = expression();
        return desugar_compound_assignment(identifier->name, op, std::move(rhs));
    }

    consume(fk::lang::TokenType::EQ, "'=' expected in assignment");

    expression_ptr rhs = expression();

    return make_statement<assignment_statement>(identifier->name, std::move(rhs));   
}

fk::lang::statement_ptr fk::lang::parser::statement()
{
    if (match({ fk::lang::TokenType::PRINT })) {
        return make_statement<print_statement>(expression());
    }

    // NOTE: we check instead of matching here so we can consume the left brace __in__ block()
    // This allows us to simply call block() whenever we need to parse a block e.g. in while statements
    if (check(fk::lang::TokenType::LBRACE)) {
        return block();
    }

    if (match({ fk::lang::TokenType::IF })) {
        return parse_if();
    }
    if (match({ fk::lang::TokenType::WHILE })) {
        return parse_while();
    }
    if (match({ fk::lang::TokenType::FOR })) {
        return parse_for();
    }
    if (match({ fk::lang::TokenType::FK_RETURN })) {
        return parse_return();
    }

    if (check({ TokenType::INC, TokenType::DEC })) {
        return parse_inc_dec();
    }

    expression_ptr expr = expression();
    if (check(TokenType::WALRUS)) {
        return parse_variable_declaration(std::move(expr));
    }
    if (check({ TokenType::EQ, TokenType::PLUSEQ, TokenType::MINUSEQ, TokenType::STAREQ, TokenType::FSLASHEQ })) {
        return assignment(std::move(expr));
    }

    return make_statement<expression_statement>(std::move(expr));
}

fk::lang::statement_ptr fk::lang::parser::parse_inc_dec()
{
    const Token& op = advance();

    expression_ptr target = expression();

    return desugar_inc_dec(op, std::move(target));
}

fk::lang::statement_ptr fk::lang::parser::block()
{
    consume(fk::lang::TokenType::LBRACE, "'{' expected to start block");

    // TODO: reserve some room ahead of time for the statements
    std::vector<fk::lang::statement_ptr> statements;
    while (!check(fk::lang::TokenType::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration()); 
    }

    consume(fk::lang::TokenType::RBRACE, "'}' expected to terminate block");

    return make_statement<block_statement>(std::move(statements));
}

fk::lang::statement_ptr fk::lang::parser::parse_if()
{
    expression_ptr condition = expression();
    statement_ptr then_block = block();

    statement_ptr else_block = nullptr;
    if (match({ fk::lang::TokenType::ELSE })) {
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
    if (match({ fk::lang::TokenType::SEMICOLON })) {
        init = nullptr;
    } else {
        expression_ptr target = expression();
        init = parse_variable_declaration(std::move(target));
        consume(fk::lang::TokenType::SEMICOLON, "';' expected after initializer statement");
    }

    expression_ptr condition;
    if (match({ fk::lang::TokenType::SEMICOLON })) {
        const Token& semicolon = prev();
        // if there is no condition, we borrow from C and assume the condition is always true
        condition = make_expression<literal_expression>(Token{"true", fk::lang::TokenType::FK_TRUE, semicolon.line, semicolon.col});
    } else {
        condition = expression();
        consume(fk::lang::TokenType::SEMICOLON, "';' expected after condition expression");
    }

    statement_ptr mutator = check(fk::lang::TokenType::LBRACE)
        ? nullptr
        : statement();

    statement_ptr body = block();

    return desugar_for_into_while(std::move(init), std::move(condition), std::move(mutator), std::move(body));
}

fk::lang::statement_ptr fk::lang::parser::parse_return()
{
    // if there is no expression, we return nil
    expression_ptr expr;
    if (check(TokenType::RBRACE)) {
        const Token& current_token = prev();
        expr = make_expression<literal_expression>(Token{"nil", TokenType::NIL, current_token.line, current_token.col});
    } else {
        expr = expression();
    }

    return make_statement<return_statement>(std::move(expr));
}

fk::lang::expression_ptr fk::lang::parser::expression()
{
    return parse_or();
}

fk::lang::expression_ptr fk::lang::parser::parse_or()
{
    fk::lang::expression_ptr left = parse_and();
    while (match({ fk::lang::TokenType::OR })) {
        fk::lang::expression_ptr right = parse_and();
        left = make_expression<fk::lang::or_expression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::parse_and()
{
    fk::lang::expression_ptr left = equality();
    while (match({ fk::lang::TokenType::AND })) {
        fk::lang::expression_ptr right = equality();
        left = make_expression<fk::lang::and_expression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::equality()
{
    static auto eq_ops = { fk::lang::TokenType::EQEQ, fk::lang::TokenType::NEQ };

    fk::lang::expression_ptr left = comparison();
    while (match(eq_ops)) {
        Token op = prev();
        fk::lang::expression_ptr right = comparison();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::comparison()
{
    static auto comp_ops = { fk::lang::TokenType::LT, fk::lang::TokenType::LTE, fk::lang::TokenType::GT, fk::lang::TokenType::GTE };

    fk::lang::expression_ptr left = term();
    while (match(comp_ops)) {
        Token op = prev();
        fk::lang::expression_ptr right = term();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::term()
{
    static auto term_ops = { fk::lang::TokenType::MINUS, fk::lang::TokenType::PLUS };

    fk::lang::expression_ptr left = factor();
    while (match(term_ops)) {
        Token op = prev();
        fk::lang::expression_ptr right = factor();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::factor()
{
    static auto factor_ops = { fk::lang::TokenType::STAR, fk::lang::TokenType::FSLASH };

    fk::lang::expression_ptr left = unary();
    while (match(factor_ops)) {
        Token op = prev();
        fk::lang::expression_ptr right = unary();
        left = make_expression<binary_expression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::expression_ptr fk::lang::parser::unary()
{
    static auto unary_ops = { fk::lang::TokenType::BANG, fk::lang::TokenType::MINUS };

    if (match(unary_ops)) {
        Token op = prev();
        fk::lang::expression_ptr right = unary();
        return make_expression<unary_expression>(op, std::move(right));
    }

    return call();
}

fk::lang::expression_ptr fk::lang::parser::call()
{
    expression_ptr expr = primary();

    if (!check(TokenType::LPAREN)) {
        return expr;
    }

    // TODO: find a better way to check this
    if (!dynamic_cast<identifier_expression*>(expr.get())) {
        throw parse_exception("<identifier> expected as function name");
    }

    consume(TokenType::LPAREN, "'(' expected to start function call arguments");
    
    std::vector<expression_ptr> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(expression());
        } while (match({ TokenType::COMMA }));
    }

    consume(TokenType::RPAREN, "')' expected to terminate function call arguments");

    return make_expression<call_expression>(std::move(expr), std::move(args));
}

fk::lang::expression_ptr fk::lang::parser::primary()
{
    if (match({ fk::lang::TokenType::NUMBER
            , fk::lang::TokenType::STRING
            , fk::lang::TokenType::FK_TRUE
            , fk::lang::TokenType::FK_FALSE
            , fk::lang::TokenType::NIL
        })) 
    {
        return make_expression<literal_expression>(prev());
    }

    if (match({ fk::lang::TokenType::IDENTIFIER })) {
        return make_expression<identifier_expression>(prev());
    }

    if (match({ fk::lang::TokenType::LPAREN })) {
        fk::lang::expression_ptr expr = expression();
        
        consume(fk::lang::TokenType::RPAREN, "terminating ')' in parenthetic expression expected");

        return make_expression<paren_expression>(std::move(expr));
    }

    throw parse_exception("primary expression expected");
}

const fk::lang::Token& fk::lang::parser::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const fk::lang::Token& fk::lang::parser::curr() const noexcept
{
    return tokens_[cursor_];
}

const fk::lang::Token& fk::lang::parser::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool fk::lang::parser::is_eof() const noexcept
{
    return curr().type == fk::lang::TokenType::FK_EOF;
}

bool fk::lang::parser::match(fk::lang::TokenType type) noexcept
{
    return match({ type });
}

bool fk::lang::parser::match(std::initializer_list<fk::lang::TokenType> types) noexcept
{
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }

    return false;
}

bool fk::lang::parser::check(fk::lang::TokenType type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}

bool fk::lang::parser::check(std::initializer_list<TokenType> types) const noexcept
{
    return std::any_of(types.begin(), types.end(), [&](TokenType type) {
        return check(type);
    });
}

fk::lang::Token fk::lang::parser::consume(TokenType type, const std::string& msg)
{
    if (!match({ type })) {
        const Token& current = curr();
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

// this turns a statement like
// a += 2
// into:
// a = a + 2
// for all of the compound assignment operators
fk::lang::statement_ptr fk::lang::parser::desugar_compound_assignment(
    const fk::lang::Token& lhs
    , const fk::lang::Token& op
    , fk::lang::expression_ptr rhs
) noexcept
{
    // TODO: verify the line and column positions
    Token rhs_op{"", TokenType::UNKNOWN, op.line, op.col};
    switch (op.str[0]) {
    case '+':
        rhs_op.str = "+";
        rhs_op.type = TokenType::PLUS;
        break;
    case '-':
        rhs_op.str = "-";
        rhs_op.type = TokenType::MINUS;
        break;
    case '*':
        rhs_op.str = "*";
        rhs_op.type = TokenType::STAR;
        break;
    case '/':
        rhs_op.str = "/";
        rhs_op.type = TokenType::FSLASH;
        break;
    default:
        // we should never hit this case
        fk::log::fatal("unknown compound assignment operator");
    }

    expression_ptr right = make_expression<binary_expression>(
        make_expression<identifier_expression>(lhs)
        , rhs_op
        , std::move(rhs)
    );

    return make_statement<assignment_statement>(lhs, std::move(right));
}

// this turns a statement like
// ++i
// into:
// i = i + 1
fk::lang::statement_ptr fk::lang::parser::desugar_inc_dec(const Token& op, expression_ptr target)
{
    // TODO: verify the line and column positions
    Token rhs_op{"", TokenType::UNKNOWN, op.line, op.col};
    switch (op.str[0]) {
    case '+':
        rhs_op.str = "+";
        rhs_op.type = TokenType::PLUS;
        break;
    case '-':
        rhs_op.str = "-";
        rhs_op.type = TokenType::MINUS;
        break;
    default:
        // we should never hit this case
        fk::log::fatal("unknown increment or decrement operator");
    }

    // TODO: for now, these operators only work on identifiers
    // Once we add other targets like record members, we need to update this logic here
    // to accept those as well
    identifier_expression *identifier = instanceof<identifier_expression>(target);
    if (identifier == nullptr) {
        const std::string opstr = op.type == TokenType::INC
            ? "increment"
            : "decrement";
        throw parse_exception("invalid " + opstr + " target");
    }


    // TODO: figure out the line, col positions for these new tokens
    expression_ptr one = make_expression<literal_expression>(Token{"1", TokenType::NUMBER, 0, 0});

    // target + 1
    expression_ptr right = make_expression<binary_expression>(
        make_expression<identifier_expression>(identifier->name)
        , rhs_op
        , std::move(one)
    );

    // target = target + 1
    return make_statement<assignment_statement>(identifier->name, std::move(right));

}