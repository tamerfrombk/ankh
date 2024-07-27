#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include <ankh/lang/exceptions.hpp>
#include <ankh/lang/lexer.hpp>
#include <ankh/lang/token.hpp>

TEST_CASE("scan assignment tokens", "[lexer]") {
    const std::string source =
        R"(
        =
        +=
        -=
        *=
        /=
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"=", ankh::lang::TokenType::EQ, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"+=", ankh::lang::TokenType::PLUSEQ, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"-=", ankh::lang::TokenType::MINUSEQ, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"*=", ankh::lang::TokenType::STAREQ, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"/=", ankh::lang::TokenType::FSLASHEQ, 6, 9});
}

TEST_CASE("scan comparison tokens", "[lexer]") {
    const std::string source =
        R"(
        !=
        ==
        >
        >=
        <
        <=
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"!=", ankh::lang::TokenType::NEQ, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"==", ankh::lang::TokenType::EQEQ, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{">", ankh::lang::TokenType::GT, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{">=", ankh::lang::TokenType::GTE, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"<", ankh::lang::TokenType::LT, 6, 9});
    REQUIRE(tokens[5] == ankh::lang::Token{"<=", ankh::lang::TokenType::LTE, 7, 9});
}

TEST_CASE("scan math tokens", "[lexer]") {
    const std::string source =
        R"(
        +
        -
        *
        /
        ++
        --
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"+", ankh::lang::TokenType::PLUS, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"-", ankh::lang::TokenType::MINUS, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"*", ankh::lang::TokenType::STAR, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"/", ankh::lang::TokenType::FSLASH, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"++", ankh::lang::TokenType::INC, 6, 9});
    REQUIRE(tokens[5] == ankh::lang::Token{"--", ankh::lang::TokenType::DEC, 7, 9});
}

TEST_CASE("scan grouping tokens", "[lexer]") {
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
        :
        .
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"(", ankh::lang::TokenType::LPAREN, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{")", ankh::lang::TokenType::RPAREN, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"{", ankh::lang::TokenType::LBRACE, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"}", ankh::lang::TokenType::RBRACE, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{";", ankh::lang::TokenType::SEMICOLON, 6, 9});
    REQUIRE(tokens[5] == ankh::lang::Token{",", ankh::lang::TokenType::COMMA, 7, 9});
    REQUIRE(tokens[6] == ankh::lang::Token{"[", ankh::lang::TokenType::LBRACKET, 8, 9});
    REQUIRE(tokens[7] == ankh::lang::Token{"]", ankh::lang::TokenType::RBRACKET, 9, 9});
    REQUIRE(tokens[8] == ankh::lang::Token{":", ankh::lang::TokenType::COLON, 10, 9});
    REQUIRE(tokens[9] == ankh::lang::Token{".", ankh::lang::TokenType::DOT, 11, 9});
}

TEST_CASE("scan boolean tokens", "[lexer]") {
    const std::string source =
        R"(
        ! 
        &&
        ||
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"!", ankh::lang::TokenType::BANG, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"&&", ankh::lang::TokenType::AND, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"||", ankh::lang::TokenType::OR, 4, 9});
}

TEST_CASE("scan keyword tokens", "[lexer]") {
    const std::string source =
        R"(
        true
        false
        nil
        if
        else
        while
        for
        fn
        return
        let
        break
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"true", ankh::lang::TokenType::ANKH_TRUE, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"false", ankh::lang::TokenType::ANKH_FALSE, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"nil", ankh::lang::TokenType::NIL, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"if", ankh::lang::TokenType::IF, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"else", ankh::lang::TokenType::ELSE, 6, 9});
    REQUIRE(tokens[5] == ankh::lang::Token{"while", ankh::lang::TokenType::WHILE, 7, 9});
    REQUIRE(tokens[6] == ankh::lang::Token{"for", ankh::lang::TokenType::FOR, 8, 9});
    REQUIRE(tokens[7] == ankh::lang::Token{"fn", ankh::lang::TokenType::FN, 9, 9});
    REQUIRE(tokens[8] == ankh::lang::Token{"return", ankh::lang::TokenType::ANKH_RETURN, 10, 9});
    REQUIRE(tokens[9] == ankh::lang::Token{"let", ankh::lang::TokenType::LET, 11, 9});
    REQUIRE(tokens[10] == ankh::lang::Token{"break", ankh::lang::TokenType::BREAK, 12, 9});

    for (const ankh::lang::Token &token : tokens) {
        if (token.type != ankh::lang::TokenType::ANKH_EOF) {
            REQUIRE(ankh::lang::is_keyword(token.str));
        }
    }
}

TEST_CASE("scan string tokens", "[lexer]") {
    const std::string source =
        R"(
        ""
        "non-empty string"
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"", ankh::lang::TokenType::STRING, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"non-empty string", ankh::lang::TokenType::STRING, 3, 9});
}

TEST_CASE("scan string tokens, backslash double quote", "[lexer]") {
    const std::string source =
        R"(
        "this string \" has a double quote"
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"this string \" has a double quote", ankh::lang::TokenType::STRING, 2, 10});
}

TEST_CASE("scan string tokens, backslash metacharacter", "[lexer]") {
    const std::string source =
        R"(
        "this string \b has a bell"
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"this string \\b has a bell", ankh::lang::TokenType::STRING, 2, 10});
}

TEST_CASE("scan number tokens", "[lexer]") {
    const std::string source =
        R"(
        123
        123.45
        123.
        0.1
        1.0
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"123", ankh::lang::TokenType::NUMBER, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"123.45", ankh::lang::TokenType::NUMBER, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"123.", ankh::lang::TokenType::NUMBER, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"0.1", ankh::lang::TokenType::NUMBER, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"1.0", ankh::lang::TokenType::NUMBER, 6, 9});
}

TEST_CASE("scan identifier tokens", "[lexer]") {
    const std::string source =
        R"(
        _foo
        foo_bar
        hello
        hello2
        zfh_3_2a
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens[0] == ankh::lang::Token{"_foo", ankh::lang::TokenType::IDENTIFIER, 2, 9});
    REQUIRE(tokens[1] == ankh::lang::Token{"foo_bar", ankh::lang::TokenType::IDENTIFIER, 3, 9});
    REQUIRE(tokens[2] == ankh::lang::Token{"hello", ankh::lang::TokenType::IDENTIFIER, 4, 9});
    REQUIRE(tokens[3] == ankh::lang::Token{"hello2", ankh::lang::TokenType::IDENTIFIER, 5, 9});
    REQUIRE(tokens[4] == ankh::lang::Token{"zfh_3_2a", ankh::lang::TokenType::IDENTIFIER, 6, 9});
}

TEST_CASE("lex non-terminated string", "[lexer]") {
    const std::string source = R"(
        "notice the lack of the terminating double quotes
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}

TEST_CASE("lex floating point with two decimals", "[lexer]") {
    const std::string source = R"(
        123.45.67
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}

TEST_CASE("lex comment", "[lexer]") {
    const std::string source = R"(
        "string" # this is a comment on the same line as an expression

        # here is a comment preceding the expression
        123.45
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE(tokens.size() == 3);
    REQUIRE((tokens[0].str == "string" && tokens[0].type == ankh::lang::TokenType::STRING));
    REQUIRE((tokens[1].str == "123.45" && tokens[1].type == ankh::lang::TokenType::NUMBER));
}

TEST_CASE("lex unary &", "[lexer]") {
    const std::string source = R"(
        &
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}

TEST_CASE("lex unary |", "[lexer]") {
    const std::string source = R"(
        |
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}

TEST_CASE("scan command operator", "[lexer]") {
    const std::string source = R"(
        $(echo hello)
    )";

    auto tokens = ankh::lang::scan(source);

    REQUIRE((tokens[0].str == "echo hello" && tokens[0].type == ankh::lang::TokenType::COMMAND));
}

TEST_CASE("scan command operator missing initial (", "[lexer]") {
    const std::string source = R"(
        $echo hello)
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}

TEST_CASE("scan command operator missing terminal (", "[lexer]") {
    const std::string source = R"(
        $(echo hello
    )";

    REQUIRE_THROWS_AS(ankh::lang::scan(source), ankh::lang::ScanException);
}
