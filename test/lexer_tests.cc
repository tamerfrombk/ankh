#include "catch.hpp"
#include "clean/lang/token.h"

#include <string>
#include <vector>

#include <clean/lang/lexer.h>
#include <clean/lang/error_handler.h>

#define LEXER_TEST(description) TEST_CASE(description, "[lexer]")

static std::vector<token_t> extract_all_tokens(lexer_t& lexer) 
{
    std::vector<token_t> tokens;
    for (token_t token = lexer.next_token(); token.type != token_type::T_EOF; token = lexer.next_token()) {
        tokens.push_back(token);
    }
    tokens.emplace_back("", token_type::T_EOF);

    return tokens;
}

LEXER_TEST("test all basic language lexemes are lexable")
{
    auto error_handler = std::make_unique<error_handler_t>();

    SECTION("happy path")
    {
        const std::string all_tokens = R"(
            =

            !=
            ==
            >
            >=
            <
            <=

            -
            +
            /
            *

            (
            )

            !
            true
            false
            nil

            ""
            "non-empty string"

            123
        )";

        lexer_t lexer(all_tokens, error_handler.get());

        std::vector<token_t> tokens = extract_all_tokens(lexer);
        
        int i = 0;
        // assignment
        REQUIRE((tokens[i].str == "=" && tokens[i++].type == token_type::EQ));

        // comparison and equality
        REQUIRE((tokens[i].str == "!=" && tokens[i++].type == token_type::NEQ));
        REQUIRE((tokens[i].str == "==" && tokens[i++].type == token_type::EQEQ));
        REQUIRE((tokens[i].str == ">" && tokens[i++].type == token_type::GT));
        REQUIRE((tokens[i].str == ">=" && tokens[i++].type == token_type::GTE));
        REQUIRE((tokens[i].str == "<" && tokens[i++].type == token_type::LT));
        REQUIRE((tokens[i].str == "<=" && tokens[i++].type == token_type::LTE));

        // arithmetic
        REQUIRE((tokens[i].str == "-" && tokens[i++].type == token_type::MINUS));
        REQUIRE((tokens[i].str == "+" && tokens[i++].type == token_type::PLUS));
        REQUIRE((tokens[i].str == "/" && tokens[i++].type == token_type::FSLASH));
        REQUIRE((tokens[i].str == "*" && tokens[i++].type == token_type::STAR));

        // parens
        REQUIRE((tokens[i].str == "(" && tokens[i++].type == token_type::LPAREN));
        REQUIRE((tokens[i].str == ")" && tokens[i++].type == token_type::RPAREN));

        // boolean
        REQUIRE((tokens[i].str == "!" && tokens[i++].type == token_type::BANG));
        REQUIRE((tokens[i].str == "true" && tokens[i++].type == token_type::BTRUE));
        REQUIRE((tokens[i].str == "false" && tokens[i++].type == token_type::BFALSE));

        // nil
        REQUIRE((tokens[i].str == "nil" && tokens[i++].type == token_type::NIL));

        // strings
        REQUIRE((tokens[i].str == "" && tokens[i++].type == token_type::STRING));
        REQUIRE((tokens[i].str == "non-empty string" && tokens[i++].type == token_type::STRING));

        // number
        REQUIRE((tokens[i].str == "123" && tokens[i++].type == token_type::NUMBER));

        // EOF -- make sure this is LAST
        REQUIRE(tokens[i++].type == token_type::T_EOF);

        REQUIRE(error_handler->error_count() == 0);
    }

    SECTION("lex non-terminated string")
    {
        const std::string stream = R"(
            "notice the lack of the terminating double quotes
        )";

        lexer_t lexer(stream, error_handler.get());

        std::vector<token_t> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens[0].type == token_type::UNKNOWN);

        REQUIRE(error_handler->error_count() == 1);
    }
}