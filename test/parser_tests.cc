#include "fak/lang/expr.h"
#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/parser.h>
#include <fak/lang/error_handler.h>

#define PARSER_TEST(description) TEST_CASE(description, "[parser]")

static void test_binary_expression_parse(const std::string& op, fk::lang::ErrorHandler *error_handler) noexcept
{
    const std::string source("1" + op + "2" + "\n");
    
    auto program = fk::lang::parse(source, error_handler);
    REQUIRE(program.size() == 1);
    REQUIRE(error_handler->error_count() == 0);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = fk::lang::instance<fk::lang::BinaryExpression>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
    REQUIRE(binary->op.str == op);
}

template <class Boolean>
static void test_boolean_binary_expression(const std::string& op, fk::lang::ErrorHandler *error_handler) noexcept
{
    const std::string source("1" + op + "2" + "\n");
    
    auto program = fk::lang::parse(source, error_handler);
    REQUIRE(program.size() == 1);
    REQUIRE(error_handler->error_count() == 0);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = fk::lang::instance<Boolean>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
}

static void test_unary_expression(const std::string& op, fk::lang::ErrorHandler *error_handler) noexcept
{
    const std::string source(op + "3" + "\n");

    auto program = fk::lang::parse(source, error_handler);

    REQUIRE(program.size() == 1);
    REQUIRE(error_handler->error_count() == 0);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);
    
    auto unary = fk::lang::instance<fk::lang::UnaryExpression>(stmt->expr);
    REQUIRE(unary != nullptr);
    REQUIRE(unary->right != nullptr);
    REQUIRE(unary->op.str == op);
}

PARSER_TEST("parse language statements")
{
    auto error_handler = std::make_unique<fk::lang::ErrorHandler>();

    SECTION("parse expression statement")
    {
        const std::string source =
        R"(
            1 + 2
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto binary = fk::lang::instance<fk::lang::BinaryExpression>(stmt->expr);
        REQUIRE(binary != nullptr);
    }

    SECTION("parse declaration statement")
    {
        const std::string source =
        R"(
            i := 1
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto declaration = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(declaration != nullptr);

        REQUIRE(declaration->name.str == "i");

        auto literal = fk::lang::instance<fk::lang::LiteralExpression>(declaration->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "1");
    }

    SECTION("parse assignment statement")
    {
        const std::string source =
        R"(
            i := 2
            i = 3
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 2);
        REQUIRE(error_handler->error_count() == 0);

        auto assignment = fk::lang::instance<fk::lang::AssignmentStatement>(program[1]);
        REQUIRE(assignment != nullptr);

        REQUIRE(assignment->name.str == "i");

        auto literal = fk::lang::instance<fk::lang::LiteralExpression>(assignment->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "3");
    }

    SECTION("parse increment statement")
    {
        const std::string source =
        R"(
            i := 2
            ++i
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 2);
        REQUIRE(error_handler->error_count() == 0);

        auto assignment = fk::lang::instance<fk::lang::AssignmentStatement>(program[1]);
        REQUIRE(assignment != nullptr);

        REQUIRE(assignment->name.str == "i");

        auto binary = fk::lang::instance<fk::lang::BinaryExpression>(assignment->initializer);
        REQUIRE(binary != nullptr);
        REQUIRE(binary->op.type == fk::lang::TokenType::PLUS);
    }

    SECTION("parse decrement statement")
    {
        const std::string source =
        R"(
            i := 2
            --i
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 2);
        REQUIRE(error_handler->error_count() == 0);

        auto assignment = fk::lang::instance<fk::lang::AssignmentStatement>(program[1]);
        REQUIRE(assignment != nullptr);

        REQUIRE(assignment->name.str == "i");

        auto binary = fk::lang::instance<fk::lang::BinaryExpression>(assignment->initializer);
        REQUIRE(binary != nullptr);
        REQUIRE(binary->op.type == fk::lang::TokenType::MINUS);
    }

    SECTION("parse print statement")
    {
        const std::string source =
        R"(
            print "hello"
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto print = fk::lang::instance<fk::lang::PrintStatement>(program[0]);
        REQUIRE(print != nullptr);

        auto literal = fk::lang::instance<fk::lang::LiteralExpression>(print->expr);
        REQUIRE(literal != nullptr);
        REQUIRE(literal->literal.type == fk::lang::TokenType::STRING);
    }

    SECTION("parse block statement")
    {
        const std::string source =
        R"(
            {
                print "hello"
                print "world"
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto block = fk::lang::instance<fk::lang::BlockStatement>(program[0]);
        REQUIRE(block != nullptr);

        REQUIRE(block->statements.size() == 2);
    }

    SECTION("parse if statement with no else")
    {
        const std::string source =
        R"(
            if 1 == 1 {
                print "yes"
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto if_stmt = fk::lang::instance<fk::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);
        REQUIRE(if_stmt->else_block == nullptr);
    }

    SECTION("parse if statement with else")
    {
        const std::string source =
        R"(
            if 1 == 1 {
                print "yes"
            } else {
                print "no"
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto if_stmt = fk::lang::instance<fk::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);

        auto else_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->else_block);
        REQUIRE(else_block != nullptr);
    }

    SECTION("parse while statement")
    {
        const std::string source =
        R"(
            i := 1
            while i < 2 {
                print "I am less than 2"
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 2);
        REQUIRE(error_handler->error_count() == 0);

        auto while_stmt = fk::lang::instance<fk::lang::WhileStatement>(program[1]);
        REQUIRE(while_stmt != nullptr);

        auto body = fk::lang::instance<fk::lang::BlockStatement>(while_stmt->body);
        REQUIRE(body != nullptr);

        REQUIRE(while_stmt->condition != nullptr);
    }

    SECTION("parse for statement")
    {
        const std::string source =
        R"(
            for i := 1; i < 2; ++i {
                print i
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto block = fk::lang::instance<fk::lang::BlockStatement>(program[0]);
        REQUIRE(block != nullptr);
        REQUIRE(block->statements.size() == 2);

        auto init = fk::lang::instance<fk::lang::VariableDeclaration>(block->statements[0]);
        REQUIRE(init != nullptr);

        auto while_stmt = fk::lang::instance<fk::lang::WhileStatement>(block->statements[1]);
        REQUIRE(while_stmt != nullptr);

        auto while_block = fk::lang::instance<fk::lang::BlockStatement>(while_stmt->body);
        REQUIRE(while_block != nullptr);
        REQUIRE(while_block->statements.size() == 2);
    }

    SECTION("parse function declaration")
    {
        const std::string source =
        R"(
            def sum(a, b, c) {
                return a + b + c
            }
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto fn = fk::lang::instance<fk::lang::FunctionDeclaration>(program[0]);
        REQUIRE(fn != nullptr);
        REQUIRE(fn->body != nullptr);
        REQUIRE(!fn->name.str.empty());
        REQUIRE(fn->params.size() == 3);

        auto body = fk::lang::instance<fk::lang::BlockStatement>(fn->body);
        REQUIRE(body != nullptr);
        REQUIRE(body->statements.size() == 1);

        auto return_stmt = fk::lang::instance<fk::lang::ReturnStatement>(body->statements[0]);
        REQUIRE(return_stmt != nullptr);
        REQUIRE(return_stmt->expr != nullptr);
    }
    
}

PARSER_TEST("parse language expressions")
{
    auto error_handler = std::make_unique<fk::lang::ErrorHandler>();

    SECTION("parse primary")
    {
        const std::string source =
        R"(
            1
            "a string"
            true
            false
            nil
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 5);
        REQUIRE(error_handler->error_count() == 0);

        for (const auto& stmt : program) {
            auto literal = fk::lang::instance<fk::lang::ExpressionStatement>(stmt);
            REQUIRE(literal != nullptr);
            REQUIRE(fk::lang::instanceof<fk::lang::LiteralExpression>(literal->expr));
        }
    }

    SECTION("parse paren")
    {
        const std::string source =
        R"(
            ( "an expression" )
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        REQUIRE(fk::lang::instanceof<fk::lang::ParenExpression>(stmt->expr));
    }

    SECTION("parse identifier")
    {
        const std::string source =
        R"(
            a
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        REQUIRE(fk::lang::instanceof<fk::lang::IdentifierExpression>(stmt->expr));
    }

    SECTION("parse function call, no args")
    {
        const std::string source =
        R"(
            a()
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 0);

        auto identifier = fk::lang::instance<fk::lang::IdentifierExpression>(call->name);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse function call, >0 args")
    {
        const std::string source =
        R"(
            a(1, 2)
        )";

        auto program = fk::lang::parse(source, error_handler.get());

        REQUIRE(program.size() == 1);
        REQUIRE(error_handler->error_count() == 0);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 2);

        auto identifier = fk::lang::instance<fk::lang::IdentifierExpression>(call->name);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse unary !")
    {
        auto error_handler_ptr = error_handler.get();
        test_unary_expression("!", error_handler_ptr);
        test_unary_expression("-", error_handler_ptr);
    } 

    SECTION("parse factor")
    {
        auto error_handler_ptr = error_handler.get();
        test_binary_expression_parse("*", error_handler_ptr);
        test_binary_expression_parse("/", error_handler_ptr);
    }

    SECTION("parse term")
    {
        auto error_handler_ptr = error_handler.get();
        test_binary_expression_parse("-", error_handler_ptr);
        test_binary_expression_parse("+", error_handler_ptr);
    }

    SECTION("parse comparison")
    {
        auto error_handler_ptr = error_handler.get();
        test_binary_expression_parse(">", error_handler_ptr);
        test_binary_expression_parse(">=", error_handler_ptr);
        test_binary_expression_parse("<", error_handler_ptr);
        test_binary_expression_parse("<=", error_handler_ptr);
    }

    SECTION("parse equality")
    {
        auto error_handler_ptr = error_handler.get();
        test_binary_expression_parse("!=", error_handler_ptr);
        test_binary_expression_parse("==", error_handler_ptr);
    }

    SECTION("parse boolean binary")
    {
        auto error_handler_ptr = error_handler.get();
        test_boolean_binary_expression<fk::lang::AndExpression>("&&", error_handler_ptr);
        test_boolean_binary_expression<fk::lang::OrExpression>("||", error_handler_ptr);
    }

}