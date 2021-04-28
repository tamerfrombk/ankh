#pragma once

#include <string>
#include <vector>

#include <clean/lang/expr.h>
#include <clean/lang/statement.h>
#include <clean/lang/token.h>

// forward declarations
struct error_handler_t;

class parser_t 
{
public:
    explicit parser_t(std::string str, error_handler_t *error_handler);

    program_t parse();

    bool is_eof() const noexcept;

private:
    statement_ptr  declaration();
    statement_ptr  assignment();
    
    statement_ptr  statement();
    statement_ptr  expression_statement();
    statement_ptr  print_statement();
    statement_ptr  block();
    
    expression_ptr expression();
    expression_ptr equality();
    expression_ptr comparison();
    expression_ptr term();
    expression_ptr factor();
    expression_ptr unary();
    expression_ptr primary();

    const token_t& prev() const noexcept;
    const token_t& curr() const noexcept;
    const token_t& advance() noexcept;

    bool match(std::initializer_list<token_type> types) noexcept;
    bool check(token_type type) const noexcept;

private:
    std::vector<token_t> tokens_;
    size_t cursor_;

    error_handler_t *error_handler_;
};