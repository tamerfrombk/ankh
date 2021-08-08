#include "fak/lang/expr.h"
#include "fak/lang/statement.h"
#include <algorithm>
#include <initializer_list>
#include <random>

#include <fak/lang/parser.h>
#include <fak/lang/lexer.h>
#include <fak/lang/token.h>
#include <fak/lang/exceptions.h>
#include <fak/lang/lambda.h>

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
        fk::lang::panic<fk::lang::ParseException>("invalid {} target", opstr);
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

static bool block_has_return_statement(const fk::lang::BlockStatement *stmt)
{
    for (const auto& st : stmt->statements) {
        if (auto block = fk::lang::instance<fk::lang::BlockStatement>(st); block != nullptr) {
            return block_has_return_statement(block);
        }

        if (fk::lang::instanceof<fk::lang::ReturnStatement>(st)) {
            return true;
        }
    }

    return false;
}

fk::lang::Program fk::lang::parse(const std::string& source)
{
    const std::vector<fk::lang::Token> tokens = fk::lang::scan(source);

    fk::lang::Parser parser(tokens);

    return parser.parse();
}

fk::lang::Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens) 
    , cursor_(0)
{}

fk::lang::Program fk::lang::Parser::parse() noexcept
{
    // PERFORMANCE: see if we can reserve some room up front
    Program program;
    while (!is_eof()) {
        try {
            program.add_statement(declaration());    
        } catch (const fk::lang::ParseException& e) {
            FK_DEBUG("parse exception: {}", e.what());
            program.add_error(e.what());
            synchronize_next_statement();
        }
    }

    return program;
}

fk::lang::StatementPtr fk::lang::Parser::declaration()
{
    if (match(fk::lang::TokenType::FN)) {
        return parse_function_declaration();
    }

    return statement();
}

fk::lang::StatementPtr fk::lang::Parser::parse_variable_declaration()
{
    StorageClass storage_class;
    if (match(TokenType::LET)) {
        storage_class = StorageClass::LOCAL;
    } else if (match(TokenType::EXPORT)) {
        storage_class = StorageClass::EXPORT;
    } else {
        const Token& token = curr();
        panic<ParseException>("{}:{}, '{}' is not a valid storage class specifier.", token.line, token.col, token.str);
    }

    ExpressionPtr target = expression();

    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        panic<ParseException>("invalid variable declaration target");
    }

    consume(TokenType::EQ, "'=' expected in variable declaration");

    ExpressionPtr rhs = expression();

    semicolon();

    return make_statement<VariableDeclaration>(identifier->name, std::move(rhs), storage_class);  
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
    
    // check to see we have a return statement inside the block
    if (BlockStatement* block = static_cast<BlockStatement*>(body.get()); !block_has_return_statement(block)) {
        FK_DEBUG("function '{}' definition doesn't have a return statement so 'return nil' will be injected", name.str);
        
        // TODO: figure out the line and column positions
        // Insert a "return nil" as the last statement in the block
        auto nil = make_expression<LiteralExpression>(Token{"nil", TokenType::NIL, 0, 0});
        block->statements.push_back(make_statement<ReturnStatement>(std::move(nil)));
    }

    return make_statement<FunctionDeclaration>(name, std::move(params), std::move(body));
}

static fk::lang::StatementPtr generate_data_ctor(
    const fk::lang::Token& name
    , const std::vector<fk::lang::Token>& members
)
{
    std::vector<fk::lang::Token> params;
    std::vector<fk::lang::StatementPtr> assignments;
    for (const auto& member : members) {
        fk::lang::Token param = member;
        param.str = "_" + param.str;
        params.push_back(param);

        fk::lang::ExpressionPtr init = fk::lang::make_expression<fk::lang::IdentifierExpression>(param);
        fk::lang::StatementPtr assignment = fk::lang::make_statement<fk::lang::AssignmentStatement>(member, std::move(init));
        assignments.push_back(std::move(assignment));
    }
    fk::lang::StatementPtr body = fk::lang::make_statement<fk::lang::BlockStatement>(std::move(assignments));

    return fk::lang::make_statement<fk::lang::FunctionDeclaration>(name, params, std::move(body));
}

fk::lang::StatementPtr fk::lang::Parser::parse_data_declaration()
{
    // no need for a message since we know we have this token in this context
    consume(TokenType::DATA, "");

    const Token name = consume(TokenType::IDENTIFIER, "<identifier> expected as data declaration name");

    consume(TokenType::LBRACE, "'{' expected to begin data body definition");

    std::vector<Token> members;
    while (!is_eof() && !check(TokenType::RBRACE)) {
        members.push_back(consume(TokenType::IDENTIFIER, "identifier expected as data member declaration"));
    }

    consume(TokenType::RBRACE, "'}' expected to end data body definition");

    if (members.empty()) {
        panic<ParseException>("{}:{}, data declarations cannot be empty", name.line, name.col);
    }

    StatementPtr ctor = generate_data_ctor(name, members);

    return make_statement<DataDeclaration>(name, members, std::move(ctor));
}

fk::lang::StatementPtr fk::lang::Parser::assignment(ExpressionPtr target)
{
    IdentifierExpression *identifier = instance<IdentifierExpression>(target);
    if (identifier == nullptr) {
        panic<ParseException>("invalid assignment target");
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

    semicolon();

    return make_statement<AssignmentStatement>(identifier->name, std::move(rhs));   
}

fk::lang::StatementPtr fk::lang::Parser::statement()
{
    if (match(fk::lang::TokenType::PRINT)) {
        ExpressionPtr expr = expression();

        semicolon();

        return make_statement<PrintStatement>(std::move(expr));
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
    } else if (check({ TokenType::LET, TokenType::EXPORT })) {
        return parse_variable_declaration();
    } else if (check(TokenType::DATA)) {
        return parse_data_declaration();
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
    } else {
        semicolon();

        return make_statement<ExpressionStatement>(std::move(expr));
    }
}

fk::lang::StatementPtr fk::lang::Parser::parse_inc_dec()
{
    const Token& op = advance();

    ExpressionPtr target = expression();

    semicolon();

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
        if (match(TokenType::IF)) {
            else_block = parse_if();
        } else {
            else_block = block();
        }
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
    StatementPtr init = nullptr;    
    if (!match(fk::lang::TokenType::SEMICOLON)) {
        init = parse_variable_declaration();
    }

    ExpressionPtr condition;
    if (match(fk::lang::TokenType::SEMICOLON)) {
        const Token& semicolon = prev();
        // if there is no condition, we borrow from C and assume the condition is always true
        condition = make_expression<LiteralExpression>(Token{"true", fk::lang::TokenType::FK_TRUE, semicolon.line, semicolon.col});
    } else {
        condition = expression();
        semicolon();
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
    if (check(TokenType::SEMICOLON)) {
        const Token& current_token = prev();
        expr = make_expression<LiteralExpression>(Token{"nil", TokenType::NIL, current_token.line, current_token.col});
    } else {
        expr = expression();
    }

    semicolon();

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
        const Token& op = prev();
        fk::lang::ExpressionPtr right = parse_and();
        left = make_expression<fk::lang::BinaryExpression>(std::move(left), op, std::move(right));
    }

    return left;
}

fk::lang::ExpressionPtr fk::lang::Parser::parse_and()
{
    fk::lang::ExpressionPtr left = equality();
    while (match(fk::lang::TokenType::AND)) {
        const Token& op = prev();
        fk::lang::ExpressionPtr right = equality();
        left = make_expression<fk::lang::BinaryExpression>(std::move(left), op, std::move(right));
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

    return operable();
}

fk::lang::ExpressionPtr fk::lang::Parser::operable()
{
    ExpressionPtr expr = primary();
    while (check({ TokenType::LPAREN, TokenType::LBRACKET, TokenType::DOT, TokenType::SEMICOLON })) {
        if (check(TokenType::SEMICOLON)) {
            return expr;
        }

        if (check(TokenType::LPAREN)) {
            expr = call(std::move(expr));
        }

        if (check(TokenType::LBRACKET)) {
            expr = index(std::move(expr));
        }

        if (check(TokenType::DOT)) {
            expr = access(std::move(expr));
        }
    }

    return expr;
}

fk::lang::ExpressionPtr fk::lang::Parser::call(ExpressionPtr callee)
{
    // no message requires since we know we have this token
    consume(TokenType::LPAREN, "");

    std::vector<ExpressionPtr> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(expression());
        } while (match({ TokenType::COMMA }));
    }

    consume(TokenType::RPAREN, "')' expected to terminate callable arguments");

    return make_expression<CallExpression>(std::move(callee), std::move(args));
}

fk::lang::ExpressionPtr fk::lang::Parser::index(ExpressionPtr indexee)
{
    // no message required since we know we have this token
    consume(TokenType::LBRACKET, "");

    ExpressionPtr idx = expression();

    consume(TokenType::RBRACKET, "']' expected to terminate index operation");

    return make_expression<IndexExpression>(std::move(indexee), std::move(idx));
}

fk::lang::ExpressionPtr fk::lang::Parser::access(ExpressionPtr accessible)
{
    // no message required here since we know we have a DOT
    consume(TokenType::DOT, "");

    Token identifier = consume(TokenType::IDENTIFIER, "identifier required after '.'");
    
    return make_expression<AccessExpression>(std::move(accessible), identifier);
}

fk::lang::ExpressionPtr fk::lang::Parser::primary()
{
    if (match(TokenType::STRING)) {
        return make_expression<StringExpression>(prev());
    }

    if (match({ 
        TokenType::NUMBER
        , TokenType::FK_TRUE
        , TokenType::FK_FALSE
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
        
        consume(TokenType::RPAREN, "terminating ')' in parenthetic expression expected");

        return make_expression<ParenExpression>(std::move(expr));
    }

    if (match(TokenType::FN)) {
        return lambda();
    }

    if (match(TokenType::COMMAND)) {
        const Token& cmd = prev();
        if (cmd.str.empty()) {
            panic<ParseException>("{}:{} command is empty", cmd.line, cmd.col);
        }

        return make_expression<CommandExpression>(cmd);
    }

    if (check(TokenType::LBRACKET)) {
        return parse_array();
    }

    if (check(TokenType::LBRACE)) {
        return dict();
    }

    panic<ParseException>("primary expression expected");
}

fk::lang::ExpressionPtr fk::lang::Parser::lambda()
{
    consume(TokenType::LPAREN, "starting '(' expected in lambda expression");

    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            Token token = consume(TokenType::IDENTIFIER, "<identifier> expected in lambda parameter declaration");
            params.push_back(token);
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RPAREN, "terminating ')' expected in lambda expression");

    StatementPtr body = block();
    
    const std::string name = generate_lambda_name();
    
    if (BlockStatement* block = static_cast<BlockStatement*>(body.get()); !block_has_return_statement(block)) {
        FK_DEBUG("lambda '{}' definition doesn't have a return statement so 'return nil' will be injected", name);
        
        // TODO: figure out the line and column positions
        // Insert a "return nil" as the last statement in the block
        auto nil = make_expression<LiteralExpression>(Token{"nil", TokenType::NIL, 0, 0});
        block->statements.push_back(make_statement<ReturnStatement>(std::move(nil)));
    }

    return make_expression<LambdaExpression>(name, std::move(params), std::move(body));
}

fk::lang::ExpressionPtr fk::lang::Parser::parse_array()
{
    consume(TokenType::LBRACKET, "starting '[' expected in array expression");

    std::vector<ExpressionPtr> elems;
    if (!check(TokenType::RBRACKET)) {
        do {
            elems.push_back(expression());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RBRACKET, "terminating ']' expected in array expression");

    return make_expression<ArrayExpression>(std::move(elems));
}

fk::lang::ExpressionPtr fk::lang::Parser::dict()
{
    consume(TokenType::LBRACE, "'{' expected to begin dictionary expression");

    std::vector<Entry<ExpressionPtr>> entries;
    if (!check(TokenType::RBRACE)) {
        entries.push_back(entry());
        while (match(TokenType::COMMA)) {
            entries.push_back(entry());
        }
    }

    consume(TokenType::RBRACE, "'}' expected to terminate dictionary expression");

    return make_expression<DictionaryExpression>(std::move(entries));
}

fk::lang::Entry<fk::lang::ExpressionPtr> fk::lang::Parser::entry()
{
    ExpressionPtr keyv = key();

    consume(TokenType::COLON, "':' expected after dictionary key");

    ExpressionPtr value = expression();

    return { std::move(keyv), std::move(value) };
}

fk::lang::ExpressionPtr fk::lang::Parser::key()
{
    if (match(TokenType::IDENTIFIER)) {
        Token str = prev();
        str.type = fk::lang::TokenType::STRING;
        return make_expression<StringExpression>(str);
    }

    consume(TokenType::LBRACKET, "'[' expected to start expression key");

    ExpressionPtr expr = expression();

    consume(TokenType::RBRACKET, "']' expected to terminate expression key");

    return expr;
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
        panic<ParseException>("syntax error: '{}' instead of '{}'", msg, current.str);
    }

    return prev();
}

void fk::lang::Parser::semicolon()
{
    // semicolons are optional so eat it if one is present
    match(TokenType::SEMICOLON);
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
        TokenType::DEC,
        TokenType::FN,
        TokenType::LET,
        TokenType::EXPORT
    };

    while (!is_eof() && !check(statement_initializer_tokens)) {
        advance();
    }
}