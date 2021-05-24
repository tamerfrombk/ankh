#pragma once

#include <string>
#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/token.h>

namespace fk::lang {

// forward declarations
struct ErrorHandler;

class parser 
{
public:
    explicit parser(std::string str, ErrorHandler *error_handler);

    program parse() noexcept;

    bool is_eof() const noexcept;

private:
    statement_ptr  declaration();
    statement_ptr  assignment(expression_ptr target);
    statement_ptr  parse_variable_declaration(expression_ptr target);
    statement_ptr  parse_function_declaration();
    
    statement_ptr  statement();
    statement_ptr  parse_inc_dec();
    
    statement_ptr  block();

    statement_ptr  parse_if();
    statement_ptr  parse_while();
    statement_ptr  parse_for();
    statement_ptr  parse_return();

    expression_ptr expression();
    expression_ptr parse_or();
    expression_ptr parse_and();
    expression_ptr equality();
    expression_ptr comparison();
    expression_ptr term();
    expression_ptr factor();
    expression_ptr unary();
    expression_ptr call();
    expression_ptr primary();

    const Token& prev() const noexcept;
    const Token& curr() const noexcept;
    const Token& advance() noexcept;

    bool match(TokenType type) noexcept;
    bool match(std::initializer_list<TokenType> types) noexcept;
    bool check(TokenType type) const noexcept;
    bool check(std::initializer_list<TokenType> types) const noexcept;
    Token consume(TokenType type, const std::string& msg);

    void synchronize_next_statement() noexcept;

    statement_ptr desugar_for_into_while(statement_ptr init, expression_ptr condition, statement_ptr mutator,statement_ptr body) noexcept;

    statement_ptr desugar_compound_assignment(const Token& lhs, const Token& op, expression_ptr rhs) noexcept;

    statement_ptr desugar_inc_dec(const Token& op, expression_ptr target);

private:
    std::vector<Token> tokens_;
    size_t cursor_;

    ErrorHandler *error_handler_;
};

}
