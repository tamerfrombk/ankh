#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/lexer.h>
#include <fak/lang/error_handler.h>

#define LEXER_TEST(description) TEST_CASE(description, "[lexer]")

LEXER_TEST("scan all language tokens")
{
    auto error_handler = std::make_unique<fk::lang::ErrorHandler>();

    SECTION("scan assignment tokens")
    {
        const std::string source =
        R"(
            =
            +=
            -=
            *=
            /=
            :=
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "=", fk::lang::TokenType::EQ, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "+=", fk::lang::TokenType::PLUSEQ, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "-=", fk::lang::TokenType::MINUSEQ, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ "*=", fk::lang::TokenType::STAREQ, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ "/=", fk::lang::TokenType::FSLASHEQ, 6, 13 });
        REQUIRE(tokens[5] == fk::lang::Token{ ":=", fk::lang::TokenType::WALRUS, 7, 13 });
    }

    SECTION("scan comparison tokens")
    {
        const std::string source =
        R"(
            !=
            ==
            >
            >=
            <
            <=
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "!=", fk::lang::TokenType::NEQ, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "==", fk::lang::TokenType::EQEQ, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ ">", fk::lang::TokenType::GT, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ ">=", fk::lang::TokenType::GTE, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ "<", fk::lang::TokenType::LT, 6, 13 });
        REQUIRE(tokens[5] == fk::lang::Token{ "<=", fk::lang::TokenType::LTE, 7, 13 });
    }

    SECTION("scan math tokens")
    {
        const std::string source =
        R"(
            +
            -
            *
            /
            ++
            --
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "+", fk::lang::TokenType::PLUS, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "-", fk::lang::TokenType::MINUS, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "*", fk::lang::TokenType::STAR, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ "/", fk::lang::TokenType::FSLASH, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ "++", fk::lang::TokenType::INC, 6, 13 });
        REQUIRE(tokens[5] == fk::lang::Token{ "--", fk::lang::TokenType::DEC, 7, 13 });
    }

    SECTION("scan grouping tokens")
    {
        const std::string source =
        R"(
            ( 
            )
            {
            }
            ;
            ,
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "(", fk::lang::TokenType::LPAREN, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ ")", fk::lang::TokenType::RPAREN, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "{", fk::lang::TokenType::LBRACE, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ "}", fk::lang::TokenType::RBRACE, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ ";", fk::lang::TokenType::SEMICOLON, 6, 13 });
        REQUIRE(tokens[5] == fk::lang::Token{ ",", fk::lang::TokenType::COMMA, 7, 13 });
    }

    SECTION("scan boolean tokens")
    {
        const std::string source =
        R"(
            ! 
            &&
            ||
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "!", fk::lang::TokenType::BANG, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "&&", fk::lang::TokenType::AND, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "||", fk::lang::TokenType::OR, 4, 13 });
    }

    SECTION("scan keyword tokens")
    {
        const std::string source =
        R"(
            true
            false
            nil
            print
            if
            else
            while
            for
            def
            return
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "true", fk::lang::TokenType::FK_TRUE, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "false", fk::lang::TokenType::FK_FALSE, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "nil", fk::lang::TokenType::NIL, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ "print", fk::lang::TokenType::PRINT, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ "if", fk::lang::TokenType::IF, 6, 13 });
        REQUIRE(tokens[5] == fk::lang::Token{ "else", fk::lang::TokenType::ELSE, 7, 13 });
        REQUIRE(tokens[6] == fk::lang::Token{ "while", fk::lang::TokenType::WHILE, 8, 13 });
        REQUIRE(tokens[7] == fk::lang::Token{ "for", fk::lang::TokenType::FOR, 9, 13 });
        REQUIRE(tokens[8] == fk::lang::Token{ "def", fk::lang::TokenType::DEF, 10, 13 });
        REQUIRE(tokens[9] == fk::lang::Token{ "return", fk::lang::TokenType::FK_RETURN, 11, 13 });
    }

    SECTION("scan string tokens")
    {
        const std::string source =
        R"(
            ""
            "non-empty string"
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "", fk::lang::TokenType::STRING, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "non-empty string", fk::lang::TokenType::STRING, 3, 13 });
    }

    SECTION("scan number tokens")
    {
        const std::string source =
        R"(
            123
            123.45
            123.
            0.1
            1.0
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens[0] == fk::lang::Token{ "123", fk::lang::TokenType::NUMBER, 2, 13 });
        REQUIRE(tokens[1] == fk::lang::Token{ "123.45", fk::lang::TokenType::NUMBER, 3, 13 });
        REQUIRE(tokens[2] == fk::lang::Token{ "123.", fk::lang::TokenType::NUMBER, 4, 13 });
        REQUIRE(tokens[3] == fk::lang::Token{ "0.1", fk::lang::TokenType::NUMBER, 5, 13 });
        REQUIRE(tokens[4] == fk::lang::Token{ "1.0", fk::lang::TokenType::NUMBER, 6, 13 });
    }

    SECTION("lex non-terminated string")
    {
        const std::string source = R"(
            "notice the lack of the terminating double quotes
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());
        
        REQUIRE(tokens[0].type == fk::lang::TokenType::UNKNOWN);
        REQUIRE(error_handler->error_count() == 1);
    }

    SECTION("lex floating point with two decimals")
    {
        const std::string source = R"(
            123.45.67
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());
        
        REQUIRE(tokens[0].type == fk::lang::TokenType::UNKNOWN);
        REQUIRE(error_handler->error_count() > 0);
    }

    SECTION("lex comment")
    {
        const std::string source = R"(
            "string" # this is a comment on the same line as an expression

            # here is a comment preceding the expression
            123.45
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());
        
        REQUIRE(tokens.size() == 3);
        REQUIRE((tokens[0].str == "string" && tokens[0].type == fk::lang::TokenType::STRING));
        REQUIRE((tokens[1].str == "123.45" && tokens[1].type == fk::lang::TokenType::NUMBER));

        REQUIRE(error_handler->error_count() == 0);
    }

    SECTION("lex unary &") 
    {
        const std::string source = R"(
            &
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());
        
        REQUIRE(tokens.size() == 2);
        REQUIRE((tokens[0].str == "&" && tokens[0].type == fk::lang::TokenType::UNKNOWN));

        REQUIRE(error_handler->error_count() == 1);
    }

    SECTION("lex unary |") 
    {
        const std::string source = R"(
            |
        )";

        auto tokens = fk::lang::scan(source, error_handler.get());

        REQUIRE(tokens.size() == 2);
        REQUIRE((tokens[0].str == "|" && tokens[0].type == fk::lang::TokenType::UNKNOWN));

        REQUIRE(error_handler->error_count() == 1);
    }
}