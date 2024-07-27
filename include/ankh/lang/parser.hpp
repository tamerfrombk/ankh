#pragma once

#include <string>
#include <vector>

#include <ankh/lang/expr.hpp>
#include <ankh/lang/statement.hpp>
#include <ankh/lang/token.hpp>
#include <ankh/lang/program.hpp>

namespace ankh::lang {

class Parser 
{
public:
    explicit Parser(const std::vector<Token>& tokens);

    Program parse() noexcept;

    bool is_eof() const noexcept;

private:
    StatementPtr  declaration();
    StatementPtr  assignment(ExpressionPtr target);
    StatementPtr  parse_variable_declaration();
    StatementPtr  parse_function_declaration();
    
    StatementPtr  statement();
    StatementPtr  parse_inc_dec();
    
    StatementPtr  block();

    StatementPtr  parse_if();
    StatementPtr  parse_while();
    StatementPtr  parse_for();
    StatementPtr  parse_return();

    ExpressionPtr expression();
    ExpressionPtr parse_or();
    ExpressionPtr parse_and();
    ExpressionPtr equality();
    ExpressionPtr comparison();
    ExpressionPtr term();
    ExpressionPtr factor();
    ExpressionPtr unary();
    ExpressionPtr operable();
    ExpressionPtr call(ExpressionPtr callee);
    ExpressionPtr index(ExpressionPtr indexable);
    ExpressionPtr access(ExpressionPtr accessible);
    ExpressionPtr primary();
    ExpressionPtr lambda();
    ExpressionPtr parse_array();

    ExpressionPtr        dict();
    Entry<ExpressionPtr> entry();
    ExpressionPtr        key();

    void semicolon();

    const Token& prev() const noexcept;
    const Token& curr() const noexcept;
    const Token& advance() noexcept;

    bool match(TokenType type) noexcept;
    bool match(std::initializer_list<TokenType> types) noexcept;

    bool check(TokenType type) const noexcept;
    bool check(std::initializer_list<TokenType> types) const noexcept;
    
    Token consume(TokenType type, const std::string& msg);

    void synchronize_next_statement() noexcept;
private:
    std::vector<Token> tokens_;
    size_t cursor_;
};

template <class ExpectedType, class Ptr>
ExpectedType* instance(const Ptr& ptr) noexcept
{
    return dynamic_cast<ExpectedType*>(ptr.get());
}

template <class ExpectedType, class Ptr>
bool instanceof(const Ptr& ptr) noexcept
{
    return instance<ExpectedType>(ptr) != nullptr;
}

Program parse(const std::string& source);

}
