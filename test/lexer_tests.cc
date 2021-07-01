#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/lexer.h>
#include <fak/lang/exceptions.h>

#define LEXER_TEST(description) TEST_CASE(description, "[lexer]")

LEXER_TEST("scan assignment tokens")
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

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "=", fk::lang::TokenType::EQ, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "+=", fk::lang::TokenType::PLUSEQ, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "-=", fk::lang::TokenType::MINUSEQ, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "*=", fk::lang::TokenType::STAREQ, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "/=", fk::lang::TokenType::FSLASHEQ, 6, 9 });
    REQUIRE(tokens[5] == fk::lang::Token{ ":=", fk::lang::TokenType::WALRUS, 7, 9 });
}

LEXER_TEST("scan comparison tokens")
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

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "!=", fk::lang::TokenType::NEQ, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "==", fk::lang::TokenType::EQEQ, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ ">", fk::lang::TokenType::GT, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ ">=", fk::lang::TokenType::GTE, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "<", fk::lang::TokenType::LT, 6, 9 });
    REQUIRE(tokens[5] == fk::lang::Token{ "<=", fk::lang::TokenType::LTE, 7, 9 });
}

LEXER_TEST("scan math tokens")
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

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "+", fk::lang::TokenType::PLUS, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "-", fk::lang::TokenType::MINUS, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "*", fk::lang::TokenType::STAR, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "/", fk::lang::TokenType::FSLASH, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "++", fk::lang::TokenType::INC, 6, 9 });
    REQUIRE(tokens[5] == fk::lang::Token{ "--", fk::lang::TokenType::DEC, 7, 9 });
}

LEXER_TEST("scan grouping tokens")
{
    const std::string source =
    R"(
        ( 
        )
        {
        }
        ;
        ,
        [
        ]
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "(", fk::lang::TokenType::LPAREN, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ ")", fk::lang::TokenType::RPAREN, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "{", fk::lang::TokenType::LBRACE, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "}", fk::lang::TokenType::RBRACE, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ ";", fk::lang::TokenType::SEMICOLON, 6, 9 });
    REQUIRE(tokens[5] == fk::lang::Token{ ",", fk::lang::TokenType::COMMA, 7, 9 });
    REQUIRE(tokens[6] == fk::lang::Token{ "[", fk::lang::TokenType::LBRACKET, 8, 9 });
    REQUIRE(tokens[7] == fk::lang::Token{ "]", fk::lang::TokenType::RBRACKET, 9, 9 });
}

LEXER_TEST("scan boolean tokens")
{
    const std::string source =
    R"(
        ! 
        &&
        ||
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "!", fk::lang::TokenType::BANG, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "&&", fk::lang::TokenType::AND, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "||", fk::lang::TokenType::OR, 4, 9 });
}

LEXER_TEST("scan keyword tokens")
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
        fn
        return
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "true", fk::lang::TokenType::FK_TRUE, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "false", fk::lang::TokenType::FK_FALSE, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "nil", fk::lang::TokenType::NIL, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "print", fk::lang::TokenType::PRINT, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "if", fk::lang::TokenType::IF, 6, 9 });
    REQUIRE(tokens[5] == fk::lang::Token{ "else", fk::lang::TokenType::ELSE, 7, 9 });
    REQUIRE(tokens[6] == fk::lang::Token{ "while", fk::lang::TokenType::WHILE, 8, 9 });
    REQUIRE(tokens[7] == fk::lang::Token{ "for", fk::lang::TokenType::FOR, 9, 9 });
    REQUIRE(tokens[8] == fk::lang::Token{ "fn", fk::lang::TokenType::FN, 10, 9 });
    REQUIRE(tokens[9] == fk::lang::Token{ "return", fk::lang::TokenType::FK_RETURN, 11, 9 });

    for (const fk::lang::Token& token : tokens) {
        if (token.type != fk::lang::TokenType::FK_EOF){
            REQUIRE(fk::lang::is_keyword(token.str));
        }
    }
}

LEXER_TEST("scan string tokens")
{
    const std::string source =
    R"(
        ""
        "non-empty string"
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "", fk::lang::TokenType::STRING, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "non-empty string", fk::lang::TokenType::STRING, 3, 9 });
}

LEXER_TEST("scan number tokens")
{
    const std::string source =
    R"(
        123
        123.45
        123.
        0.1
        1.0
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "123", fk::lang::TokenType::NUMBER, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "123.45", fk::lang::TokenType::NUMBER, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "123.", fk::lang::TokenType::NUMBER, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "0.1", fk::lang::TokenType::NUMBER, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "1.0", fk::lang::TokenType::NUMBER, 6, 9 });
}

LEXER_TEST("scan identifier tokens")
{
    const std::string source =
    R"(
        _foo
        foo_bar
        hello
        hello2
        zfh_3_2a
    )";

    auto tokens = fk::lang::scan(source);

    REQUIRE(tokens[0] == fk::lang::Token{ "_foo", fk::lang::TokenType::IDENTIFIER, 2, 9 });
    REQUIRE(tokens[1] == fk::lang::Token{ "foo_bar", fk::lang::TokenType::IDENTIFIER, 3, 9 });
    REQUIRE(tokens[2] == fk::lang::Token{ "hello", fk::lang::TokenType::IDENTIFIER, 4, 9 });
    REQUIRE(tokens[3] == fk::lang::Token{ "hello2", fk::lang::TokenType::IDENTIFIER, 5, 9 });
    REQUIRE(tokens[4] == fk::lang::Token{ "zfh_3_2a", fk::lang::TokenType::IDENTIFIER, 6, 9 });
}

LEXER_TEST("lex non-terminated string")
{
    const std::string source = R"(
        "notice the lack of the terminating double quotes
    )";

    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}

LEXER_TEST("lex floating point with two decimals")
{
    const std::string source = R"(
        123.45.67
    )";

    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}

LEXER_TEST("lex comment")
{
    const std::string source = R"(
        "string" # this is a comment on the same line as an expression

        # here is a comment preceding the expression
        123.45
    )";

    auto tokens = fk::lang::scan(source);
    
    REQUIRE(tokens.size() == 3);
    REQUIRE((tokens[0].str == "string" && tokens[0].type == fk::lang::TokenType::STRING));
    REQUIRE((tokens[1].str == "123.45" && tokens[1].type == fk::lang::TokenType::NUMBER));
}

LEXER_TEST("lex unary &") 
{
    const std::string source = R"(
        &
    )";

    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}

LEXER_TEST("lex unary |") 
{
    const std::string source = R"(
        |
    )";
    
    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}

LEXER_TEST("scan command operator") 
{
    const std::string source = R"(
        $(echo hello)
    )";
    
    auto tokens = fk::lang::scan(source);

    REQUIRE((tokens[0].str == "echo hello" && tokens[0].type == fk::lang::TokenType::COMMAND));
}

LEXER_TEST("scan command operator missing initial (") 
{
    const std::string source = R"(
        $echo hello)
    )";
    
    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}

LEXER_TEST("scan command operator missing terminal (") 
{
    const std::string source = R"(
        $(echo hello
    )";
    
    REQUIRE_THROWS_AS(fk::lang::scan(source), fk::lang::ScanException);
}