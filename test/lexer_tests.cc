#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/lexer.h>
#include <fak/lang/error_handler.h>

#define LEXER_TEST(description) TEST_CASE(description, "[lexer]")

static std::vector<fk::lang::token> extract_all_tokens(fk::lang::lexer& lexer) noexcept
{
    std::vector<fk::lang::token> tokens;
    for (fk::lang::token token = lexer.next_token(); token.type != fk::lang::token_type::T_EOF; token = lexer.next_token()) {
        tokens.push_back(token);
    }
    tokens.emplace_back("", fk::lang::token_type::T_EOF);

    return tokens;
}

LEXER_TEST("scan all basic language lexemes")
{
    auto error_handler = std::make_unique<fk::lang::error_handler>();

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

            { 
            }

            !
            true
            false
            nil
            print

            ""
            "non-empty string"

            123
            123.45
            123.
            0.1
            1.0
        )";

        fk::lang::lexer lexer(all_tokens, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        int i = 0;
        // assignment
        REQUIRE((tokens[i].str == "=" && tokens[i++].type == fk::lang::token_type::EQ));

        // comparison and equality
        REQUIRE((tokens[i].str == "!=" && tokens[i++].type == fk::lang::token_type::NEQ));
        REQUIRE((tokens[i].str == "==" && tokens[i++].type == fk::lang::token_type::EQEQ));
        REQUIRE((tokens[i].str == ">" && tokens[i++].type == fk::lang::token_type::GT));
        REQUIRE((tokens[i].str == ">=" && tokens[i++].type == fk::lang::token_type::GTE));
        REQUIRE((tokens[i].str == "<" && tokens[i++].type == fk::lang::token_type::LT));
        REQUIRE((tokens[i].str == "<=" && tokens[i++].type == fk::lang::token_type::LTE));

        // arithmetic
        REQUIRE((tokens[i].str == "-" && tokens[i++].type == fk::lang::token_type::MINUS));
        REQUIRE((tokens[i].str == "+" && tokens[i++].type == fk::lang::token_type::PLUS));
        REQUIRE((tokens[i].str == "/" && tokens[i++].type == fk::lang::token_type::FSLASH));
        REQUIRE((tokens[i].str == "*" && tokens[i++].type == fk::lang::token_type::STAR));

        // parens
        REQUIRE((tokens[i].str == "(" && tokens[i++].type == fk::lang::token_type::LPAREN));
        REQUIRE((tokens[i].str == ")" && tokens[i++].type == fk::lang::token_type::RPAREN));

        // braces
        REQUIRE((tokens[i].str == "{" && tokens[i++].type == fk::lang::token_type::LBRACE));
        REQUIRE((tokens[i].str == "}" && tokens[i++].type == fk::lang::token_type::RBRACE));

        // boolean
        REQUIRE((tokens[i].str == "!" && tokens[i++].type == fk::lang::token_type::BANG));
        REQUIRE((tokens[i].str == "true" && tokens[i++].type == fk::lang::token_type::BTRUE));
        REQUIRE((tokens[i].str == "false" && tokens[i++].type == fk::lang::token_type::BFALSE));

        // nil
        REQUIRE((tokens[i].str == "nil" && tokens[i++].type == fk::lang::token_type::NIL));

        // print
        REQUIRE((tokens[i].str == "print" && tokens[i++].type == fk::lang::token_type::PRINT));

        // strings
        REQUIRE((tokens[i].str == "" && tokens[i++].type == fk::lang::token_type::STRING));
        REQUIRE((tokens[i].str == "non-empty string" && tokens[i++].type == fk::lang::token_type::STRING));

        // number
        REQUIRE((tokens[i].str == "123" && tokens[i++].type == fk::lang::token_type::NUMBER));
        REQUIRE((tokens[i].str == "123.45" && tokens[i++].type == fk::lang::token_type::NUMBER));
        REQUIRE((tokens[i].str == "123." && tokens[i++].type == fk::lang::token_type::NUMBER));
        REQUIRE((tokens[i].str == "0.1" && tokens[i++].type == fk::lang::token_type::NUMBER));
        REQUIRE((tokens[i].str == "1.0" && tokens[i++].type == fk::lang::token_type::NUMBER));

        // EOF -- make sure this is LAST
        REQUIRE(tokens[i++].type == fk::lang::token_type::T_EOF);

        REQUIRE(error_handler->error_count() == 0);
    }

    SECTION("lex non-terminated string")
    {
        const std::string stream = R"(
            "notice the lack of the terminating double quotes
        )";

        fk::lang::lexer lexer(stream, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens[0].type == fk::lang::token_type::UNKNOWN);

        REQUIRE(error_handler->error_count() == 1);
    }

    SECTION("lex floating point with two decimals")
    {
        const std::string stream = R"(
            123.45.67
        )";

        fk::lang::lexer lexer(stream, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens[0].type == fk::lang::token_type::UNKNOWN);
        REQUIRE(error_handler->error_count() > 0);
    }

    SECTION("lex comment")
    {
        const std::string stream = R"(
            "string" # this is a comment on the same line as an expression

            # here is a comment preceding the expression
            123.45
        )";

        fk::lang::lexer lexer(stream, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens.size() == 3);
        REQUIRE((tokens[0].str == "string" && tokens[0].type == fk::lang::token_type::STRING));
        REQUIRE((tokens[1].str == "123.45" && tokens[1].type == fk::lang::token_type::NUMBER));

        REQUIRE(error_handler->error_count() == 0);
    }
}