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
    for (fk::lang::token token = lexer.next_token(); 
        token.type != fk::lang::token_type::T_EOF; 
        token = lexer.next_token()) 
    {
        tokens.push_back(token);
    }
    tokens.push_back(lexer.next_token());

    return tokens;
}

LEXER_TEST("scan all basic language lexemes")
{
    auto error_handler = std::make_unique<fk::lang::error_handler>();

    const std::string all_tokens = 
    R"(=

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

        if
        else
        &&
        ||
        while
        for
        let
        def
        return
        
        ;
        ,

        ""
        "non-empty string"

        123
        123.45
        123.
        0.1
        1.0
    )";

    fk::lang::lexer lexer(all_tokens, error_handler.get());

    std::vector<fk::lang::token> token_stream = extract_all_tokens(lexer);

    REQUIRE_FALSE(token_stream.empty());

    SECTION("lex all language tokens")
    {   
        int i = 0;
        
        // assignment
        REQUIRE(token_stream[i++] == fk::lang::token{ "=", fk::lang::token_type::EQ, 1, 0 });

        // comparison and equality
        REQUIRE(token_stream[i++] == fk::lang::token{ "!=", fk::lang::token_type::NEQ, 3, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "==", fk::lang::token_type::EQEQ, 4, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ ">", fk::lang::token_type::GT, 5, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ ">=", fk::lang::token_type::GTE, 6, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "<", fk::lang::token_type::LT, 7, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "<=", fk::lang::token_type::LTE, 8, 0 });

        // arithmetic
        REQUIRE(token_stream[i++] == fk::lang::token{ "-", fk::lang::token_type::MINUS, 10, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "+", fk::lang::token_type::PLUS, 11, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "/", fk::lang::token_type::FSLASH, 12, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "*", fk::lang::token_type::STAR, 13, 0 });
        
        // parens
        REQUIRE(token_stream[i++] == fk::lang::token{ "(", fk::lang::token_type::LPAREN, 15, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ ")", fk::lang::token_type::RPAREN, 16, 0 });
    
        // braces
        REQUIRE(token_stream[i++] == fk::lang::token{ "{", fk::lang::token_type::LBRACE, 18, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "}", fk::lang::token_type::RBRACE, 19, 0 });
        
        // boolean
        REQUIRE(token_stream[i++] == fk::lang::token{ "!", fk::lang::token_type::BANG, 21, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "true", fk::lang::token_type::BTRUE, 22, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "false", fk::lang::token_type::BFALSE, 23, 0 });

        // nil
        REQUIRE(token_stream[i++] == fk::lang::token{ "nil", fk::lang::token_type::NIL, 25, 0 });
        
        // print
        REQUIRE(token_stream[i++] == fk::lang::token{ "print", fk::lang::token_type::PRINT, 27, 0 });

        // conditionals
        REQUIRE(token_stream[i++] == fk::lang::token{ "if", fk::lang::token_type::IF, 29, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "else", fk::lang::token_type::ELSE, 30, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "&&", fk::lang::token_type::AND, 31, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "||", fk::lang::token_type::OR, 32, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "while", fk::lang::token_type::WHILE, 33, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "for", fk::lang::token_type::FOR, 34, 0 });

        // declaration
        REQUIRE(token_stream[i++] == fk::lang::token{ "let", fk::lang::token_type::LET, 35, 0 });

        // def
        REQUIRE(token_stream[i++] == fk::lang::token{ "def", fk::lang::token_type::DEF, 36, 0 });

        // return
        REQUIRE(token_stream[i++] == fk::lang::token{ "return", fk::lang::token_type::FK_RETURN, 37, 0 });

        // semicolon
        REQUIRE(token_stream[i++] == fk::lang::token{ ";", fk::lang::token_type::SEMICOLON, 39, 0 });

        // comma
        REQUIRE(token_stream[i++] == fk::lang::token{ ",", fk::lang::token_type::COMMA, 40, 0 });

        // strings
        REQUIRE(token_stream[i++] == fk::lang::token{ "", fk::lang::token_type::STRING, 42, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "non-empty string", fk::lang::token_type::STRING, 43, 0 });

        // number
        REQUIRE(token_stream[i++] == fk::lang::token{ "123", fk::lang::token_type::NUMBER, 45, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "123.45", fk::lang::token_type::NUMBER, 46, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "123.", fk::lang::token_type::NUMBER, 47, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "0.1", fk::lang::token_type::NUMBER, 48, 0 });
        REQUIRE(token_stream[i++] == fk::lang::token{ "1.0", fk::lang::token_type::NUMBER, 49, 0 });

        // EOF -- make sure this is LAST
        REQUIRE(token_stream[i++] == fk::lang::token{ "EOF", fk::lang::token_type::T_EOF, 50, 0 });

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

    SECTION("lex unary &") 
    {
        const std::string stream = R"(
            &
        )";

        fk::lang::lexer lexer(stream, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens.size() == 2);
        REQUIRE((tokens[0].str == "&" && tokens[0].type == fk::lang::token_type::UNKNOWN));

        REQUIRE(error_handler->error_count() == 1);
    }

    SECTION("lex unary |") 
    {
        const std::string stream = R"(
            |
        )";

        fk::lang::lexer lexer(stream, error_handler.get());

        std::vector<fk::lang::token> tokens = extract_all_tokens(lexer);
        
        REQUIRE(tokens.size() == 2);
        REQUIRE((tokens[0].str == "|" && tokens[0].type == fk::lang::token_type::UNKNOWN));

        REQUIRE(error_handler->error_count() == 1);
    }
}