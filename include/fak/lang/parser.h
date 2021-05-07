#pragma once

#include <string>
#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/token.h>

namespace fk::lang {

// forward declarations
struct error_handler;

class parser 
{
public:
    explicit parser(std::string str, error_handler *error_handler);

    program parse() noexcept;

    bool is_eof() const noexcept;

private:
    statement_ptr  declaration();
    statement_ptr  assignment();
    statement_ptr  parse_variable_declaration();
    statement_ptr  parse_function_declaration();
    
    statement_ptr  statement();
    
    statement_ptr  block();

    statement_ptr  parse_if();
    statement_ptr  parse_while();
    statement_ptr  parse_for();

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

    const token& prev() const noexcept;
    const token& curr() const noexcept;
    const token& advance() noexcept;

    bool match(std::initializer_list<token_type> types) noexcept;
    bool check(token_type type) const noexcept;
    token consume(token_type type, const std::string& msg);

    void synchronize_next_statement() noexcept;

    statement_ptr desugar_for_into_while(statement_ptr init, expression_ptr condition, statement_ptr mutator,statement_ptr body) noexcept;

private:
    std::vector<token> tokens_;
    size_t cursor_;

    error_handler *error_handler_;
};

}
