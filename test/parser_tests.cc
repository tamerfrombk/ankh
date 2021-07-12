#include <catch/catch.hpp>

#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/exceptions.h>
#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/lambda.h>
#include <fak/lang/parser.h>

static void test_binary_expression_parse(const std::string& op) noexcept
{
    const std::string source("1" + op + "2" + "\n");
    
    auto program = fk::lang::parse(source);
    REQUIRE(program.size() == 1);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = fk::lang::instance<fk::lang::BinaryExpression>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
    REQUIRE(binary->op.str == op);
}

static void test_boolean_binary_expression(const std::string& op) noexcept
{
    const std::string source("true" + op + "false" + "\n");
    
    auto program = fk::lang::parse(source);
    REQUIRE(program.size() == 1);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);

    auto binary = fk::lang::instance<fk::lang::BinaryExpression>(stmt->expr);
    REQUIRE(binary != nullptr);
    REQUIRE(binary->left != nullptr);
    REQUIRE(binary->right != nullptr);
}

static void test_unary_expression(const std::string& op) noexcept
{
    const std::string source(op + "3" + "\n");

    auto program = fk::lang::parse(source);

    REQUIRE(program.size() == 1);

    auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
    REQUIRE(stmt != nullptr);
    
    auto unary = fk::lang::instance<fk::lang::UnaryExpression>(stmt->expr);
    REQUIRE(unary != nullptr);
    REQUIRE(unary->right != nullptr);
    REQUIRE(unary->op.str == op);
}

TEST_CASE("parse language statements", "[parser]")
{
    SECTION("parse expression statement")
    {
        const std::string source =
        R"(
            1 + 2
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto binary = fk::lang::instance<fk::lang::BinaryExpression>(stmt->expr);
        REQUIRE(binary != nullptr);
    }

    SECTION("parse declaration statement, local storage")
    {
        const std::string source =
        R"(
            let i = 1
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto declaration = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(declaration != nullptr);

        REQUIRE(declaration->name.str == "i");

        auto literal = fk::lang::instance<fk::lang::LiteralExpression>(declaration->initializer);
        REQUIRE(literal != nullptr);

        REQUIRE(literal->literal.str == "1");
    }

    SECTION("parse declaration statement, export storage")
    {
        const std::string source =
        R"(
            export i = 1
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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
            let i = 2
            i = 3
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 2);

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
            let i = 2
            ++i
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 2);

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
            let i = 2
            --i
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 2);

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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto print = fk::lang::instance<fk::lang::PrintStatement>(program[0]);
        REQUIRE(print != nullptr);

        auto literal = fk::lang::instance<fk::lang::StringExpression>(print->expr);
        REQUIRE(literal != nullptr);
        REQUIRE(literal->str.type == fk::lang::TokenType::STRING);
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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto if_stmt = fk::lang::instance<fk::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);

        auto else_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->else_block);
        REQUIRE(else_block != nullptr);
    }

    SECTION("parse if statement with else-if")
    {
        const std::string source =
        R"(
            if 1 == 2 {
                print "what?"
            } else if 2 == 2 {
                print "yay"
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto if_stmt = fk::lang::instance<fk::lang::IfStatement>(program[0]);
        REQUIRE(if_stmt != nullptr);

        auto then_block = fk::lang::instance<fk::lang::BlockStatement>(if_stmt->then_block);
        REQUIRE(then_block != nullptr);

        auto else_block = fk::lang::instance<fk::lang::IfStatement>(if_stmt->else_block);
        REQUIRE(else_block != nullptr);
    }

    SECTION("parse while statement")
    {
        const std::string source =
        R"(
            let i = 1
            while i < 2 {
                print "I am less than 2"
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 2);

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
            for let i = 1; i < 2; ++i {
                print i
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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
            fn sum(a, b, c) {
                return a + b + c
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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

    SECTION("return statement injected into return-less function")
    {
        const std::string source =
            R"(
                fn foo(a, b) {
                    print a + b
                }
            )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto fn = fk::lang::instance<fk::lang::FunctionDeclaration>(program[0]);

        auto body = fk::lang::instance<fk::lang::BlockStatement>(fn->body);
        REQUIRE(body->statements.size() == 2);

        REQUIRE(fk::lang::instanceof<fk::lang::PrintStatement>(body->statements[0]));

        auto return_stmt = fk::lang::instance<fk::lang::ReturnStatement>(body->statements[1]);
        REQUIRE(return_stmt != nullptr);
        
        auto nil = fk::lang::instance<fk::lang::LiteralExpression>(return_stmt->expr);
        REQUIRE(nil != nullptr);
        REQUIRE(nil->literal.str == "nil");
    }

    SECTION("return statement NOT injected into function with return")
    {
        const std::string source =
            R"(
                fn foo(a, b) {
                    let s = a + b
                    {
                        return s
                    }
                }
            )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto fn = fk::lang::instance<fk::lang::FunctionDeclaration>(program[0]);

        auto body = fk::lang::instance<fk::lang::BlockStatement>(fn->body);
        REQUIRE(body->statements.size() == 2);

        REQUIRE(fk::lang::instanceof<fk::lang::VariableDeclaration>(body->statements[0]));

        auto block = fk::lang::instance<fk::lang::BlockStatement>(body->statements[1]);
        REQUIRE(block != nullptr);
        REQUIRE(block->statements.size() == 1);
        REQUIRE(fk::lang::instanceof<fk::lang::ReturnStatement>(block->statements[0]));
    }
}

TEST_CASE("parse language expressions", "[parser]")
{
    SECTION("parse primary")
    {
        const std::string source =
        R"(
            1
            true
            false
            nil
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 4);

        const auto& statements = program.statements();
        for (const auto& stmt : statements) {
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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

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

        auto program = fk::lang::parse(source);
        for (auto e : program.errors()) {
            INFO(e);
        }

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 0);

        auto identifier = fk::lang::instance<fk::lang::IdentifierExpression>(call->callee);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse function call, >0 args")
    {
        const std::string source =
        R"(
            a(1, 2)
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto call = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(call != nullptr);
        REQUIRE(call->args.size() == 2);

        auto identifier = fk::lang::instance<fk::lang::IdentifierExpression>(call->callee);
        REQUIRE(identifier != nullptr);
        REQUIRE(identifier->name.str == "a");
    }

    SECTION("parse function call, multicall")
    {
        const std::string source =
        R"(
            a(1, 2)()
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto inner_call = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(inner_call != nullptr);
        REQUIRE(inner_call->args.size() == 0);

        auto callee = fk::lang::instance<fk::lang::CallExpression>(inner_call->callee);
        REQUIRE(callee != nullptr);
        REQUIRE(callee->args.size() == 2);

        auto callee_name = fk::lang::instance<fk::lang::IdentifierExpression>(callee->callee);
        REQUIRE(callee_name != nullptr);
        REQUIRE(callee_name->name.str == "a");
    }

    SECTION("parse lambda expression")
    {
        const std::string source =
        R"(
            let lambda = fn (a, b) {
                return a + b
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto lambda = fk::lang::instance<fk::lang::LambdaExpression>(stmt->initializer);
        REQUIRE(lambda != nullptr);
        REQUIRE(lambda->params.size() == 2);
        REQUIRE(lambda->body != nullptr);
        REQUIRE(!lambda->generated_name.empty());
    }

    SECTION("parse unary !")
    {
        test_unary_expression("!");
        test_unary_expression("-");
    } 

    SECTION("parse factor")
    {
        test_binary_expression_parse("*");
        test_binary_expression_parse("/");
    }

    SECTION("parse term")
    {
        test_binary_expression_parse("-");
        test_binary_expression_parse("+");
    }

    SECTION("parse comparison")
    {
        test_binary_expression_parse(">");
        test_binary_expression_parse(">=");
        test_binary_expression_parse("<");
        test_binary_expression_parse("<=");
    }

    SECTION("parse equality")
    {
        test_binary_expression_parse("!=");
        test_binary_expression_parse("==");
    }

    SECTION("parse logical")
    {
        test_boolean_binary_expression("&&");
        test_boolean_binary_expression("||");
    }

    SECTION("parse command")
    {
        const std::string source =
        R"(
            $(echo hello)
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto cmd = fk::lang::instance<fk::lang::CommandExpression>(stmt->expr);
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->cmd.str == "echo hello");
        REQUIRE(cmd->cmd.type == fk::lang::TokenType::COMMAND);
    }

    SECTION("parse empty command")
    {
        const std::string source =
        R"(
            $()
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("parse array")
    {
        const std::string source =
        R"(
            [1, 2]
        )";

        auto program = fk::lang::parse(source);
        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto arr = fk::lang::instance<fk::lang::ArrayExpression>(stmt->expr);
        REQUIRE(arr != nullptr);
        REQUIRE(arr->elems.size() == 2);
    }

    SECTION("parse empty array")
    {
        const std::string source =
        R"(
            []
        )";

        auto program = fk::lang::parse(source);
        REQUIRE(program.size() == 1);
        
        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);

        auto arr = fk::lang::instance<fk::lang::ArrayExpression>(stmt->expr);
        REQUIRE(arr != nullptr);
        REQUIRE(arr->elems.size() == 0);
    }

    SECTION("interleave call and index expressions")
    {
        const std::string source =
        R"(
            foo()[0]
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto idx = fk::lang::instance<fk::lang::IndexExpression>(stmt->expr);
        REQUIRE(idx != nullptr);
    }

    SECTION("interleave index and call expressions")
    {
        const std::string source =
        R"(
            foo[0]()
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.size() == 1);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[0]);
        REQUIRE(stmt != nullptr);
        
        auto idx = fk::lang::instance<fk::lang::CallExpression>(stmt->expr);
        REQUIRE(idx != nullptr);
    }

    SECTION("index with no index expression")
    {
        const std::string source =
        R"(
            foo[]
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("dictionary, one key")
    {
        const std::string source =
        R"(
            let dict = {
                hello: "world"
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 1);
        
        auto key = fk::lang::instance<fk::lang::StringExpression>(dict->entries[0].key);
        REQUIRE(key != nullptr);

        auto value = fk::lang::instance<fk::lang::StringExpression>(dict->entries[0].value);
        REQUIRE(value != nullptr);
    }

    SECTION("dictionary, empty")
    {
        const std::string source =
        R"(
            let dict = {}
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.empty());
    }

    SECTION("dictionary, multi entry")
    {
        const std::string source =
        R"(
            let dict = {
                hello: "world"
                , foo: "1"
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 2);
        
        for (const auto& e : dict->entries) {
            auto key = fk::lang::instance<fk::lang::StringExpression>(e.key);
            REQUIRE(key != nullptr);

            auto value = fk::lang::instance<fk::lang::StringExpression>(e.value);
            REQUIRE(value != nullptr);
        }
    }

    SECTION("dictionary, expression key, single member")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 1);

        auto key = fk::lang::instance<fk::lang::BinaryExpression>(dict->entries[0].key);
        REQUIRE(key != nullptr);

        auto value = fk::lang::instance<fk::lang::LiteralExpression>(dict->entries[0].value);
        REQUIRE(value != nullptr);
    }

    SECTION("dictionary, expression key, multi member")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                , [3 + 4] : 2
                , foo : "bar"
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 1);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 3);
    }

    SECTION("dictionary, multi member, missing comma")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                 [3 + 4] : 2
                , welp
            }
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(program.has_errors());
    }

    SECTION("dictionary, lookup")
    {
        const std::string source =
        R"(
            let dict = {
                [1 + 1] : 2
                , [3 + 4] : 2
                , welp: "gulp"
            }

            dict["f"]
        )";

        auto program = fk::lang::parse(source);

        REQUIRE(!program.has_errors());
        REQUIRE(program.size() == 2);

        auto decl = fk::lang::instance<fk::lang::VariableDeclaration>(program[0]);
        REQUIRE(decl != nullptr);
        
        auto dict = fk::lang::instance<fk::lang::DictionaryExpression>(decl->initializer);
        REQUIRE(dict != nullptr);
        REQUIRE(dict->entries.size() == 3);

        auto stmt = fk::lang::instance<fk::lang::ExpressionStatement>(program[1]);
        REQUIRE(stmt != nullptr);

        auto lookup = fk::lang::instance<fk::lang::IndexExpression>(stmt->expr);
        REQUIRE(lookup != nullptr);
    }
}

TEST_CASE("test parse statement without a empty line at the end does not infinite loop", "[parser]")
{
    auto program = fk::lang::parse("1 + 2");

    REQUIRE(program.size() == 1);
}

TEST_CASE("parse two arrays as two separate statements rather than an index operation", "[parser][!mayfail]")
{
    const std::string source =
    R"(
        [1, 2]
        [0]
    )";

    auto program = fk::lang::parse(source);
    REQUIRE(program.size() == 2);
}
