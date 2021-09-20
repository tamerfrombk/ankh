#include "ankh/lang/expr.h"
#include "ankh/lang/statement.h"
#include <algorithm>
#include <initializer_list>
#include <random>

#include <ankh/log.h>

#include <ankh/lang/parser.h>
#include <ankh/lang/lexer.h>
#include <ankh/lang/token.h>
#include <ankh/lang/exceptions.h>
#include <ankh/lang/lambda.h>
#include <ankh/lang/static_analyzer.h>


static char generate_random_alpha_char() noexcept
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_int_distribution dist(0, 26);

    return 'A' + dist(mt);
}

static std::string generate_lambda_name() noexcept
{
    std::string name("lambda$");
    for (int i = 0; i < 5; ++i) {
        name += generate_random_alpha_char();
    }

    return name;
}

ankh::lang::Program ankh::lang::parse(const std::string& source)
{
    const std::vector<ankh::lang::Token> tokens = ankh::lang::scan(source);

    ankh::lang::Parser parser(tokens);

    ankh::lang::Program program = parser.parse();

    ankh::lang::StaticAnalyzer analyzer;

    try {
        program.hop_table = analyzer.resolve(program);
    } catch (const ParseException& e) {
        program.errors.push_back(e.what());
    }

    return program;
}

ankh::lang::Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens) 
    , cursor_(0)
{}

ankh::lang::Program ankh::lang::Parser::parse() noexcept
{
    // PERFORMANCE: see if we can reserve some room up front
    Program program;
    while (!is_eof()) {
        try {
            program.statements.push_back(declaration());    
        } catch (const ankh::lang::ParseException& e) {
            ANKH_DEBUG("parse exception: {}", e.what());
            program.errors.push_back(e.what());
            synchronize_next_statement();
        }
    }

    return program;
}

ankh::lang::StatementPtr ankh::lang::Parser::declaration()
{
    if (match(ankh::lang::TokenType::FN)) {
        return parse_function_declaration();
    }

    return statement();
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_variable_declaration()
{
    StorageClass storage_class;
    if (match(TokenType::LET)) {
        storage_class = StorageClass::LOCAL;
    } else if (match(TokenType::EXPORT)) {
        storage_class = StorageClass::EXPORT;
    } else {
        const Token& token = curr();
        panic<ParseException>(token, "syntax error: '{}' is not a valid storage class specifier.", token.str);
    }

    // we get the token here before looking for an expression so we can accurately report line/col
    // positions below
    const Token& current_token = curr();

    ExpressionPtr target = expression();

    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        panic<ParseException>(current_token, "syntax error: invalid variable declaration target");
    }

    consume(TokenType::EQ, "'=' expected in variable declaration");

    ExpressionPtr rhs = expression();

    semicolon();

    return make_statement<VariableDeclaration>(identifier->name, std::move(rhs), storage_class);  
} 

ankh::lang::StatementPtr ankh::lang::Parser::parse_function_declaration()
{
    const Token name = consume(TokenType::IDENTIFIER, "<identifier> expected as function name");

    consume(TokenType::LPAREN, "'(' expected to start function declaration parameters");

    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token param = consume(TokenType::IDENTIFIER, "<identifier> expected in function parameter declaration");
            params.push_back(std::move(param));        
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "')' expected to terminate function declaration parameters");

    StatementPtr body = block();

    return make_statement<FunctionDeclaration>(name, std::move(params), std::move(body));
}

ankh::lang::StatementPtr ankh::lang::Parser::assignment(ExpressionPtr target)
{
    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        panic<ParseException>(curr(), "syntax error: invalid assignment target '{}'", target->stringify());
    }

    // no need to check this since we already know that we have one of these
    match({ TokenType::EQ, TokenType::PLUSEQ, TokenType::MINUSEQ, TokenType::STAREQ, TokenType::FSLASHEQ });
    
    const Token& op = prev();
    
    ExpressionPtr rhs = expression();

    semicolon();

    if (op.type == TokenType::EQ) {
        return make_statement<AssignmentStatement>(identifier->name, std::move(rhs));   
    } else {
        return make_statement<CompoundAssignment>(identifier->name, op, std::move(rhs));
    }
}

ankh::lang::StatementPtr ankh::lang::Parser::statement()
{
    if (check(ankh::lang::TokenType::LBRACE)) {
        // NOTE: we check instead of matching here so we can consume the left brace __in__ block()
        // This allows us to simply call block() whenever we need to parse a block e.g. in while statements
        return block();
    } else if (match(ankh::lang::TokenType::IF)) {
        return parse_if();
    } else if (match(ankh::lang::TokenType::WHILE)) {
        return parse_while();
    } else if (match(ankh::lang::TokenType::FOR)) {
        return parse_for();
    } else if (match(ankh::lang::TokenType::ANKH_RETURN)) {
        return parse_return();
    } else if (match(ankh::lang::TokenType::BREAK)) {
        return make_statement<BreakStatement>(prev());
    } else if (check({ TokenType::INC, TokenType::DEC })) {
        return parse_inc_dec();
    } else if (check({ TokenType::LET, TokenType::EXPORT })) {
        return parse_variable_declaration();
    }

    ExpressionPtr expr = expression();
    if (check({ 
        TokenType::EQ
        , TokenType::PLUSEQ
        , TokenType::MINUSEQ
        , TokenType::STAREQ
        , TokenType::FSLASHEQ 
        })
    ) {
        return assignment(std::move(expr));
    }

    semicolon();

    return make_statement<ExpressionStatement>(std::move(expr));
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_inc_dec()
{
    const Token& op = advance();

    ExpressionPtr target = expression();
    
    semicolon();
    
    if (instanceof<IdentifierExpression>(target)) {
        return make_statement<IncOrDecIdentifierStatement>(op, std::move(target));
    }

    panic<ParseException>(op, "syntax error: only identifiers are valid increment/decrement targets");
}

ankh::lang::StatementPtr ankh::lang::Parser::block()
{
    consume(ankh::lang::TokenType::LBRACE, "'{' expected to start block");

    // PERFORMANCE: reserve some room ahead of time for the statements
    std::vector<ankh::lang::StatementPtr> statements;
    while (!check(ankh::lang::TokenType::RBRACE) && !is_eof()) {
        statements.emplace_back(declaration()); 
    }

    consume(ankh::lang::TokenType::RBRACE, "'}' expected to terminate block");

    return make_statement<BlockStatement>(std::move(statements));
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_if()
{
    const Token& if_token = prev();

    ExpressionPtr condition = expression();
    
    StatementPtr then_block = block();

    StatementPtr else_block = nullptr;
    if (match(ankh::lang::TokenType::ELSE)) {
        if (match(TokenType::IF)) {
            else_block = parse_if();
        } else {
            else_block = block();
        }
    }

    return make_statement<IfStatement>(if_token, std::move(condition), std::move(then_block), std::move(else_block));
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_while()
{
    const Token& while_token = prev();

    ExpressionPtr condition = expression();
    StatementPtr body = block();

    return make_statement<WhileStatement>(while_token, std::move(condition), std::move(body));
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_for()
{
    const Token& for_token = prev();

    // If we hit a brace, we know it is an infinite loop
    if (check(TokenType::LBRACE)) {
        StatementPtr body = block();

        return make_statement<ForStatement>(for_token, nullptr, nullptr, nullptr, std::move(body));
    }

    StatementPtr init = nullptr;
    if (check(TokenType::LET)) {
        init = parse_variable_declaration();
    } else {
        consume(TokenType::SEMICOLON, "';' expected after for-loop init statement");
    }

    ExpressionPtr condition = nullptr;
    if (!match(TokenType::SEMICOLON)) {
        condition = expression();
        semicolon();
    }

    StatementPtr mutator = check(ankh::lang::TokenType::LBRACE)
        ? nullptr
        : statement();

    StatementPtr body = block();

    return make_statement<ForStatement>(for_token, std::move(init), std::move(condition), std::move(mutator), std::move(body));
}

ankh::lang::StatementPtr ankh::lang::Parser::parse_return()
{
    // get the "return" token for error handling purposes
    const Token& return_token = prev();
    
    if (check(TokenType::RBRACE) || match(TokenType::SEMICOLON)) {
        return make_statement<ReturnStatement>(return_token, nullptr);
    }
    
    ExpressionPtr expr = expression();

    semicolon();

    return make_statement<ReturnStatement>(return_token, std::move(expr));
}

ankh::lang::ExpressionPtr ankh::lang::Parser::expression()
{
    return parse_or();
}

ankh::lang::ExpressionPtr ankh::lang::Parser::parse_or()
{
    ankh::lang::ExpressionPtr left = parse_and();
    while (match(ankh::lang::TokenType::OR)) {
        const Token& op = prev();
        ankh::lang::ExpressionPtr right = parse_and();
        left = make_expression<ankh::lang::BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::parse_and()
{
    ankh::lang::ExpressionPtr left = equality();
    while (match(ankh::lang::TokenType::AND)) {
        const Token& op = prev();
        ankh::lang::ExpressionPtr right = equality();
        left = make_expression<ankh::lang::BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::equality()
{
    ankh::lang::ExpressionPtr left = comparison();
    while (match({ ankh::lang::TokenType::EQEQ, ankh::lang::TokenType::NEQ })) {
        Token op = prev();
        ankh::lang::ExpressionPtr right = comparison();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::comparison()
{
    ankh::lang::ExpressionPtr left = term();
    while (match({ 
        ankh::lang::TokenType::LT
        , ankh::lang::TokenType::LTE
        , ankh::lang::TokenType::GT
        , ankh::lang::TokenType::GTE
        })
    ) {
        Token op = prev();
        ankh::lang::ExpressionPtr right = term();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::term()
{
    ankh::lang::ExpressionPtr left = factor();
    while (match({ ankh::lang::TokenType::MINUS, ankh::lang::TokenType::PLUS })) {
        Token op = prev();
        ankh::lang::ExpressionPtr right = factor();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::factor()
{
    ankh::lang::ExpressionPtr left = unary();
    while (match({ ankh::lang::TokenType::STAR, ankh::lang::TokenType::FSLASH })) {
        Token op = prev();
        ankh::lang::ExpressionPtr right = unary();
        left = make_expression<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::unary()
{
    if (match({ ankh::lang::TokenType::BANG, ankh::lang::TokenType::MINUS })) {
        Token op = prev();
        ankh::lang::ExpressionPtr right = unary();
        return make_expression<UnaryExpression>(op, std::move(right));
    }

    return operable();
}

ankh::lang::ExpressionPtr ankh::lang::Parser::operable()
{
    ExpressionPtr expr = primary();
    while (check({ TokenType::LPAREN, TokenType::LBRACKET, TokenType::SEMICOLON })) {
        if (check(TokenType::SEMICOLON)) {
            return expr;
        }

        if (check(TokenType::LPAREN)) {
            expr = call(std::move(expr));
        }

        if (check(TokenType::LBRACKET)) {
            expr = index(std::move(expr));
        }
    }

    return expr;
}

ankh::lang::ExpressionPtr ankh::lang::Parser::call(ExpressionPtr callee)
{
    // no message requires since we know we have this token
    const Token& lparen = consume(TokenType::LPAREN, "");

    std::vector<ExpressionPtr> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(expression());
        } while (match({ TokenType::COMMA }));
    }

    consume(TokenType::RPAREN, "')' expected to terminate callable arguments");

    return make_expression<CallExpression>(lparen, std::move(callee), std::move(args));
}

ankh::lang::ExpressionPtr ankh::lang::Parser::index(ExpressionPtr indexee)
{
    // no message required since we know we have this token
    const Token& lbracket = consume(TokenType::LBRACKET, "");

    ExpressionPtr begin = nullptr;
    if (match(TokenType::COLON)) {
        ExpressionPtr end = nullptr;
        if (!match(TokenType::RBRACKET)) {
            end = expression();
            consume(TokenType::RBRACKET, "']' expected to terminate slice operation");
        }
        return make_expression<SliceExpression>(lbracket, std::move(indexee), std::move(begin), std::move(end));
    }

    begin = expression();
    if (match(TokenType::COLON)) {
        ExpressionPtr end = nullptr;
        if (!match(TokenType::RBRACKET)) {
            end = expression();
            consume(TokenType::RBRACKET, "']' expected to terminate slice operation");
        }
        return make_expression<SliceExpression>(lbracket, std::move(indexee), std::move(begin), std::move(end));
    }

    consume(TokenType::RBRACKET, "']' expected to terminate index operation");

    return make_expression<IndexExpression>(lbracket, std::move(indexee), std::move(begin));
}

ankh::lang::ExpressionPtr ankh::lang::Parser::primary()
{
    if (match(TokenType::STRING)) {
        return make_expression<StringExpression>(prev());
    }

    if (match({ 
        TokenType::NUMBER
        , TokenType::ANKH_TRUE
        , TokenType::ANKH_FALSE
        , TokenType::NIL
        })
    ) {
        return make_expression<LiteralExpression>(prev());
    }

    if (match(TokenType::IDENTIFIER)) {
        return make_expression<IdentifierExpression>(prev());
    }

    if (match(TokenType::LPAREN)) {
        ExpressionPtr expr = expression();
        
        consume(TokenType::RPAREN, "')' expected to terminate parenthetic expression");

        return make_expression<ParenExpression>(std::move(expr));
    }

    if (match(TokenType::FN)) {
        return lambda();
    }

    if (match(TokenType::COMMAND)) {
        const Token& cmd = prev();
        if (cmd.str.empty()) {
            panic<ParseException>(cmd, "syntax error: command cannot be empty", cmd.line, cmd.col);
        }

        return make_expression<CommandExpression>(cmd);
    }

    if (check(TokenType::LBRACKET)) {
        return parse_array();
    }

    if (check(TokenType::LBRACE)) {
        return dict();
    }

    panic<ParseException>(curr(), "syntax error: primary expression expected, found '{}' instead", curr().str);
}

ankh::lang::ExpressionPtr ankh::lang::Parser::lambda()
{
    const Token& fn_token = prev();

    consume(TokenType::LPAREN, "'(' expected to start lambda expression");

    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token token = consume(TokenType::IDENTIFIER, "<identifier> expected in lambda parameter declaration");
            params.push_back(token);
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "')' expected to terminate lambda expression");

    StatementPtr body = block();
    
    const std::string name = generate_lambda_name();

    return make_expression<LambdaExpression>(fn_token, name, std::move(params), std::move(body));
}

ankh::lang::ExpressionPtr ankh::lang::Parser::parse_array()
{
    consume(TokenType::LBRACKET, "'[' expected to begin array expression");

    std::vector<ExpressionPtr> elems;
    if (!check(TokenType::RBRACKET)) {
        do {
            elems.push_back(expression());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RBRACKET, "']' expected to terminate array expression");

    return make_expression<ArrayExpression>(std::move(elems));
}

ankh::lang::ExpressionPtr ankh::lang::Parser::dict()
{
    const Token& lbrace = consume(TokenType::LBRACE, "'{' expected to begin dictionary expression");

    std::vector<Entry<ExpressionPtr>> entries;
    if (!check(TokenType::RBRACE)) {
        entries.push_back(entry());
        while (match(TokenType::COMMA)) {
            entries.push_back(entry());
        }
    }

    consume(TokenType::RBRACE, "'}' expected to terminate dictionary expression");

    return make_expression<DictionaryExpression>(lbrace, std::move(entries));
}

ankh::lang::Entry<ankh::lang::ExpressionPtr> ankh::lang::Parser::entry()
{
    ExpressionPtr keyv = key();

    consume(TokenType::COLON, "':' expected after dictionary key");

    ExpressionPtr value = expression();

    return { std::move(keyv), std::move(value) };
}

ankh::lang::ExpressionPtr ankh::lang::Parser::key()
{
    if (match(TokenType::IDENTIFIER)) {
        Token str = prev();
        str.type = ankh::lang::TokenType::STRING;
        return make_expression<StringExpression>(str);
    }

    consume(TokenType::LBRACKET, "'[' expected to start expression key");

    ExpressionPtr expr = expression();

    consume(TokenType::RBRACKET, "']' expected to terminate expression key");

    return expr;
}

const ankh::lang::Token& ankh::lang::Parser::prev() const noexcept
{
    return tokens_[cursor_ - 1];
}

const ankh::lang::Token& ankh::lang::Parser::curr() const noexcept
{
    return tokens_[cursor_];
}

const ankh::lang::Token& ankh::lang::Parser::advance() noexcept
{
    if (!is_eof()) {
        ++cursor_;
    }

    return prev();
}

bool ankh::lang::Parser::is_eof() const noexcept
{
    return curr().type == ankh::lang::TokenType::ANKH_EOF;
}

bool ankh::lang::Parser::match(ankh::lang::TokenType type) noexcept
{
    if (check(type)) {
        advance();
        return true;
    }

    return false;
}

bool ankh::lang::Parser::match(std::initializer_list<ankh::lang::TokenType> types) noexcept
{
    return std::any_of(types.begin(), types.end(), [&](TokenType type) {
        return match(type);
    });
}

bool ankh::lang::Parser::check(ankh::lang::TokenType type) const noexcept
{
    if (is_eof()) {
        return false;
    }

    return curr().type == type;
}

bool ankh::lang::Parser::check(std::initializer_list<TokenType> types) const noexcept
{
    return std::any_of(types.begin(), types.end(), [&](TokenType type) {
        return check(type);
    });
}

ankh::lang::Token ankh::lang::Parser::consume(TokenType type, const std::string& msg)
{
    if (!match(type)) {
        const Token& current = curr();
        panic<ParseException>(current, "syntax error: {}, found '{}' instead", msg, current.str);
    }

    return prev();
}

void ankh::lang::Parser::semicolon()
{
    // semicolons are optional so eat it if one is present
    match(TokenType::SEMICOLON);
}

void ankh::lang::Parser::synchronize_next_statement() noexcept
{
    const auto statement_initializer_tokens = {
        TokenType::LBRACE,
        TokenType::IF,
        TokenType::WHILE,
        TokenType::FOR,
        TokenType::ANKH_RETURN,
        TokenType::INC,
        TokenType::DEC,
        TokenType::FN,
        TokenType::LET,
        TokenType::EXPORT,
        TokenType::DATA,
        TokenType::BREAK,
    };

    while (!is_eof() && !check(statement_initializer_tokens)) {
        advance();
    }
}