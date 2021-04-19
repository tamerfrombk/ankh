#pragma once

#include "clean/lexer.h"
#include <string>
#include <vector>

#include <clean/expr.h>
#include <clean/token.h>


class parser_t 
{
public:
    explicit parser_t(std::string str);

    expression_ptr parse_expression();

    bool is_eof() const noexcept;
    
private:
    expression_ptr parse_equality();
    expression_ptr parse_comparison();
    expression_ptr parse_term();
    expression_ptr parse_factor();
    expression_ptr parse_unary();
    expression_ptr parse_primary();

    const token_t& prev() const noexcept;
    const token_t& curr() const noexcept;
    const token_t& advance() noexcept;

    bool match(std::initializer_list<token_type> types) noexcept;
    bool check(token_type type) const noexcept;

private:
    std::vector<token_t> tokens_;
    size_t cursor_;
};