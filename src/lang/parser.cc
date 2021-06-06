#include <algorithm>
#include <initializer_list>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/error_handler.h>
#include <fak/lang/parse_exception.h>
#include <fak/log.h>

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
static fk::lang::StatementPtr desugar_for_into_while(
    fk::lang::StatementPtr init
    , fk::lang::ExpressionPtr condition
    , fk::lang::StatementPtr mutator
    , fk::lang::StatementPtr body
) noexcept
{
    std::vector<fk::lang::StatementPtr> while_body_statements;
    while_body_statements.push_back(std::move(body));
    if (mutator != nullptr) {
        while_body_statements.push_back(std::move(mutator));
    }

    auto while_body = fk::lang::make_statement<fk::lang::BlockStatement>(std::move(while_body_statements));
    auto while_stmt = fk::lang::make_statement<fk::lang::WhileStatement>(std::move(condition), std::move(while_body)); 

    std::vector<fk::lang::StatementPtr> statements;
    if (init != nullptr) {
        statements.push_back(std::move(init));
    }
    statements.push_back(std::move(while_stmt));
    
    return fk::lang::make_statement<fk::lang::BlockStatement>(std::move(statements));
}

// this turns a statement like
// a += 2
// into:
// a = a + 2
// for all of the compound assignment operators
static fk::lang::StatementPtr desugar_compound_assignment(
    const fk::lang::Token& lhs
    , const fk::lang::Token& op
    , fk::lang::ExpressionPtr rhs
) noexcept
{
    // TODO: verify the line and column positions
    fk::lang::Token rhs_op{"", fk::lang::TokenType::UNKNOWN, op.line, op.col};
    switch (op.str[0]) {
    case '+':
        rhs_op.str = "+";
        rhs_op.type = fk::lang::TokenType::PLUS;
        break;
    case '-':
        rhs_op.str = "-";
        rhs_op.type = fk::lang::TokenType::MINUS;
        break;
    case '*':
        rhs_op.str = "*";
        rhs_op.type = fk::lang::TokenType::STAR;
        break;
    case '/':
        rhs_op.str = "/";
        rhs_op.type = fk::lang::TokenType::FSLASH;
        break;
    default:
        FK_FATAL("unknown compound assignment operator");
    }

    fk::lang::ExpressionPtr right = fk::lang::make_expression<fk::lang::BinaryExpression>(
        fk::lang::make_expression<fk::lang::IdentifierExpression>(lhs)
        , rhs_op
        , std::move(rhs)
    );

    return fk::lang::make_statement<fk::lang::AssignmentStatement>(lhs, std::move(right));
}

// this turns a statement like
// ++i
// into:
// i = i + 1
static fk::lang::StatementPtr desugar_inc_dec(const fk::lang::Token& op, fk::lang::ExpressionPtr target)
{
    // TODO: verify the line and column positions
    fk::lang::Token rhs_op{"", fk::lang::TokenType::UNKNOWN, op.line, op.col};
    switch (op.str[0]) {
    case '+':
        rhs_op.str = "+";
        rhs_op.type = fk::lang::TokenType::PLUS;
        break;
    case '-':
        rhs_op.str = "-";
        rhs_op.type = fk::lang::TokenType::MINUS;
        break;
    default:
        FK_FATAL("unknown increment or decrement operator");
    }

    // TODO: for now, these operators only work on identifiers
    // Once we add other targets like record members, we need to update this logic here
    // to accept those as well
    fk::lang::IdentifierExpression *identifier = fk::lang::instance<fk::lang::IdentifierExpression>(target);
    if (identifier == nullptr) {
        const std::string opstr = op.type == fk::lang::TokenType::INC
            ? "increment"
            : "decrement";
        throw fk::lang::ParseException("invalid " + opstr + " target");
    }


    // TODO: figure out the line, col positions for these new tokens
    fk::lang::ExpressionPtr one = fk::lang::make_expression<fk::lang::LiteralExpression>(
        fk::lang::Token{"1", fk::lang::TokenType::NUMBER, 0, 0}
    );

    // target + 1
    fk::lang::ExpressionPtr right = fk::lang::make_expression<fk::lang::BinaryExpression>(
        fk::lang::make_expression<fk::lang::IdentifierExpression>(identifier->name)
        , rhs_op
        , std::move(one)
    );

    // target = target + 1
    return fk::lang::make_statement<fk::lang::AssignmentStatement>(identifier->name, std::move(right));
}

fk::lang::Program fk::lang::parse(const std::string& source, fk::lang::ErrorHandler *error_handler) noexcept
{
    const std::vector<fk::lang::Token> tokens = fk::lang::scan(source, error_handler);

    fk::lang::Parser parser(tokens, error_handler);

    return parser.parse();
}

fk::lang::Parser::Parser(const std::vector<Token>& tokens, ErrorHandler *error_handler)
    : tokens_(tokens) 
    , cursor_(0)
    , error_handler_(error_handler)
{}

fk::lang::Program fk::lang::Parser::parse() noexcept
{
    // PERFORMANCE: see if we can reserve some room up front
    Program stmts;
    while (!is_eof()) {
        try {
            stmts.push_back(declaration());    
        } catch (const fk::lang::ParseException& e) {
            error_handler_->report_error({e.what()});
            synchronize_next_statement();
        }
    }

    return stmts;
}

fk::lang::StatementPtr fk::lang::Parser::declaration()
{
    if (match(fk::lang::TokenType::DEF)) {
        return parse_function_declaration();
    }

    return statement();
}

fk::lang::StatementPtr fk::lang::Parser::parse_variable_declaration(ExpressionPtr target)
{
    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        throw ParseException("invalid variable declaration target");
    }

    consume(TokenType::WALRUS, "':=' expected in variable declaration");

    ExpressionPtr rhs = expression();

    return make_statement<VariableDeclaration>(identifier->name, std::move(rhs));  
} 

fk::lang::StatementPtr fk::lang::Parser::parse_function_declaration()
{
    const Token name = consume(TokenType::IDENTIFIER, "<identifier> expected as function name");

    consume(TokenType::LPAREN, "'(' expected to start function declaration parameters");

    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token param = consume(TokenType::IDENTIFIER, "<identifier> expected in function parameters");
            params.push_back(std::move(param));        
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "')' expected to terminate function declaration parameters");

    StatementPtr body = block();

    return make_statement<FunctionDeclaration>(std::move(name), std::move(params), std::move(body));
}

fk::lang::StatementPtr fk::lang::Parser::assignment(ExpressionPtr target)
{
    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        throw ParseException("invalid assignment target");
    }

    if (match({ 
        TokenType::PLUSEQ
        , TokenType::MINUSEQ
        , TokenType::STAREQ
        , TokenType::FSLASHEQ 
        })
    ) {
        const Token& op = prev();
        ExpressionPtr rhs = expression();
        return desugar_compound_assignment(identifier->name, op, std::move(rhs));
    }

    consume(fk::lang::TokenType::EQ, "'=' expected in assignment");

    ExpressionPtr rhs = expression();

    return make_statement<AssignmentStatement>(identifier->name, std::move(rhs));   
}

fk::lang::StatementPtr fk::lang::Parser::statement()
{
    if (match(fk::lang::TokenType::PRINT)) {
        return make_statement<PrintStatement>(expression());
    } else if (check(fk::lang::TokenType::LBRACE)) {
        // NOTE: we check instead of matching here so we can consume the left brace __in__ block()
        // This allows us to simply call block() whenever we need to parse a block e.g. in while statements
        return block();
    } else if (match(fk::lang::TokenType::IF)) {
        return parse_if();
    } else if (match(fk::lang::TokenType::WHILE)) {
        return parse_while();
    } else if (match(fk::lang::TokenType::FOR)) {
        return parse_for();
    } else if (match(fk::lang::TokenType::FK_RETURN)) {
        return parse_return();
    } else if (check({ TokenType::INC, TokenType::DEC })) {
        return parse_inc_dec();
    } else {
        ExpressionPtr expr = expression();
        if (check(TokenType::WALRUS)) {
            return parse_variable_declaration(std::move(expr));
        } else if (check({ 
            TokenType::EQ
            , TokenType::PLUSEQ
            , TokenType::MINUSEQ
            , TokenType::STAREQ
            , TokenType::FSLASHEQ 
            })
        ) {
            return assignment(std::move(expr));
        } else {
            return make_statement<ExpressionStatement>(std::move(expr));
        }
    }
}

fk::lang::StatementPtr fk::lang::Parser::parse_inc_dec()
{
    const Token& op = advance();

    ExpressionPtr target = expression();

    return desugar_inc_dec(op, std::move(target));
}

fk::lang::StatementPtr fk::lang::Parser::block()
{
    consume(fk::lang::TokenType::LBRACE, "'{' expected to start block");

    // PERFORMANCE: reserve some room ahead of time for the statements
    std::vector<fk::lang::StatementPtr> statements;
    while (!check(fk::lang::TokenType::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration()); 
    }

    consume(fk::lang::TokenType::RBRACE, "'}' expected to terminate block");

    return make_statement<BlockStatement>(std::move(statements));
}

fk::lang::StatementPtr fk::lang::Parser::parse_if()
{
    ExpressionPtr condition = expression();
    StatementPtr then_block = block();

    StatementPtr else_block = nullptr;
    if (match(fk::lang::TokenType::ELSE)) {
        else_block = block();
    }

    return make_statement<IfStatement>(std::move(condition), std::move(then_block), std::move(else_block));
}

fk::lang::StatementPtr fk::lang::Parser::parse_while()
{
    ExpressionPtr condition = expression();
    StatementPtr body = block();

    return make_statement<WhileStatement>(std::move(condition), std::move(body));
}

fk::lang::StatementPtr fk::lang::Parser::parse_for()
{
    StatementPtr init;    
    if (match(fk::lang::TokenType::SEMICOLON)) {
        init = nullptr;
    } else {
        ExpressionPtr target = expression();
        init = parse_variable_declaration(std::move(target));
        consume(fk::lang::TokenType::SEMICOLON, "';' expected after initializer statement");
    }

    ExpressionPtr condition;
    if (match(fk::lang::TokenType::SEMICOLON)) {
        const Token& semicolon = prev();
        // if there is no condition, we borrow from C and assume the condition is always true
        condition = make_expression<LiteralExpression>(Token{"true", fk::lang::TokenType::FK_TRUE, semicolon.line, semicolon.col});
    } else {
        condition = expression();
        consume(fk::lang::TokenType::SEMICOLON, "';' expected after condition expression");
    }

    StatementPtr mutator = check(fk::lang::TokenType::LBRACE)
        ? nullptr
        : statement();

    StatementPtr body = block();

    return desugar_for_into_while(std::move(init), std::move(condition), std::move(mutator), std::move(body));
}

fk::lang::StatementPtr fk::lang::Parser::parse_return()
{
    // if there is no expression, we return nil
    ExpressionPtr expr;
    if (check(TokenType::RBRACE)) {
        const Token& current_token = prev();
        expr = make_expression<LiteralExpression>(Token{"nil", TokenType::NIL, current_token.line, current_token.col});
    } else {
        expr = expression();
    }

    return make_statement<ReturnStatement>(std::move(expr));
}

fk::lang::ExpressionPtr fk::lang::Parser::expression()
{
    return parse_or();
}

fk::lang::ExpressionPtr fk::lang::Parser::parse_or()
{
    fk::lang::ExpressionPtr left = parse_and();
    while (match(fk::lang::TokenType::OR)) {
        fk::lang::ExpressionPtr right = parse_and();
        left = make_expression<fk::lang::OrExpression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::parse_and()
{
    fk::lang::ExpressionPtr left = equality();
    while (match(fk::lang::TokenType::AND)) {
        fk::lang::ExpressionPtr right = equality();
        left = make_expression<fk::lang::AndExpression>(std::move(left), std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::equality()
{
    fk::lang::ExpressionPtr left = comparison();
    while (match({ fk::lang::TokenType::EQEQ, fk::lang::TokenType::NEQ })) {
        Token op = prev();
        fk::lang::ExpressionPtr right = comparison();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::comparison()
{
    fk::lang::ExpressionPtr left = term();
    while (match({ 
        fk::lang::TokenType::LT
        , fk::lang::TokenType::LTE
        , fk::lang::TokenType::GT
        , fk::lang::TokenType::GTE
        })
    ) {
        Token op = prev();
        fk::lang::ExpressionPtr right = term();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::term()
{
    fk::lang::ExpressionPtr left = factor();
    while (match({ fk::lang::TokenType::MINUS, fk::lang::TokenType::PLUS })) {
        Token op = prev();
        fk::lang::ExpressionPtr right = factor();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::factor()
{
    fk::lang::ExpressionPtr left = unary();
    while (match({ fk::lang::TokenType::STAR, fk::lang::TokenType::FSLASH })) {
        Token op = prev();
        fk::lang::ExpressionPtr right = unary();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::unary()
{
    if (match({ fk::lang::TokenType::BANG, fk::lang::TokenType::MINUS })) {
        Token op = prev();
        fk::lang::ExpressionPtr right = unary();
        return make_expression<UnaryExpression>(op, std::move(right));
    }

    return call();
}

fk::lang::ExpressionPtr fk::lang::Parser::call()
{
    ExpressionPtr expr = primary();
    
    if (!check(TokenType::LPAREN)) {
        return expr;
    }

    if (!instanceof<IdentifierExpression>(expr)) {
        throw ParseException("<identifier> expected as function name");
    }

    ExpressionPtr callable = std::move(expr);
    while (match(TokenType::LPAREN)) {
        std::vector<ExpressionPtr> args;
        
        if (!check(TokenType::RPAREN)) {
            do {
                args.push_back(expression());
            } while (match({ TokenType::COMMA }));
        }

        consume(TokenType::RPAREN, "')' expected to terminate function call arguments");

        callable = make_expression<CallExpression>(std::move(callable), std::move(args)); 
    }

    return callable;
}

fk::lang::ExpressionPtr fk::lang::Parser::primary()
{
    if (match({ 
        fk::lang::TokenType::NUMBER
        , fk::lang::TokenType::STRING
        , fk::lang::TokenType::FK_TRUE
        , fk::lang::TokenType::FK_FALSE
        , fk::lang::TokenType::NIL
        })
    ) {
        return make_expression<LiteralExpression>(prev());
    }

    if (match(fk::lang::TokenType::IDENTIFIER)) {
        return make_expression<IdentifierExpression>(prev());
    }

    if (match(fk::lang::TokenType::LPAREN)) {
        fk::lang::ExpressionPtr expr = expression();
        
        consume(fk::lang::TokenType::RPAREN, "terminating ')' in parenthetic expression expected");

        return make_expression<ParenExpression>(std::move(expr));
    }

    throw ParseException("primary expression expected");
}

const fk::lang::Token& fk::lang::Parser::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const fk::lang::Token& fk::lang::Parser::curr() const noexcept
{
    return tokens_[cursor_];
}

const fk::lang::Token& fk::lang::Parser::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool fk::lang::Parser::is_eof() const noexcept
{
    return curr().type == fk::lang::TokenType::FK_EOF;
}

bool fk::lang::Parser::match(fk::lang::TokenType type) noexcept
{
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

bool fk::lang::Parser::match(std::initializer_list<fk::lang::TokenType> types) noexcept
{
    return std::any_of(types.begin(), types.end(), [&](TokenType type) {
        return match(type);
    });
}

bool fk::lang::Parser::check(fk::lang::TokenType type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}

bool fk::lang::Parser::check(std::initializer_list<TokenType> types) const noexcept
{
    return std::any_of(types.begin(), types.end(), [&](TokenType type) {
        return check(type);
    });
}

fk::lang::Token fk::lang::Parser::consume(TokenType type, const std::string& msg)
{
    if (!match(type)) {
        const Token& current = curr();
        std::string error_message("syntax error: " + msg + " instead of '" + current.str + "'");
        throw fk::lang::ParseException(error_message);
    }

    return prev();
}

void fk::lang::Parser::synchronize_next_statement() noexcept
{
    const auto statement_initializer_tokens = {
        TokenType::PRINT,
        TokenType::LBRACE,
        TokenType::IF,
        TokenType::WHILE,
        TokenType::FOR,
        TokenType::FK_RETURN,
        TokenType::INC,
        TokenType::DEC
    };

    while (!check(statement_initializer_tokens)) {
        advance();
    }
}