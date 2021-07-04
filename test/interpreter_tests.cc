#include <catch/catch.hpp>

#include <initializer_list>
#include <string>
#include <memory>
#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/program.h>
#include <fak/lang/parser.h>
#include <fak/lang/interpreter.h>

#include <fak/def.h>

class TracingInterpreter
    : public fk::lang::Interpreter
{
public:
    TracingInterpreter(std::unique_ptr<fk::lang::Interpreter> interp)
        : interp_(std::move(interp)) {}

    virtual fk::lang::ExprResult evaluate(const fk::lang::ExpressionPtr& expr) override
    {
        fk::lang::ExprResult result = Interpreter::evaluate(expr);

        results_.push_back(result);

        return result;
    }

    const std::vector<fk::lang::ExprResult>& results() const noexcept
    {
        return results_;
    }

    FK_NO_DISCARD bool has_function(const std::string& name) const noexcept
    {
        return functions().count(name) == 1;
    }

private:
    std::unique_ptr<fk::lang::Interpreter> interp_;
    std::vector<fk::lang::ExprResult> results_;
};

struct ExecutionResult
{
    fk::lang::Program program;
    std::vector<fk::lang::ExprResult> results;    
};

ExecutionResult interpret(TracingInterpreter& interpreter, const std::string& source)
{
    fk::lang::Program program = fk::lang::parse(source);
    if (program.has_errors()) {
        return { std::move(program), {} };
    }

    interpreter.interpret(program);

    return { std::move(program), interpreter.results() };
}

TEST_CASE("primary expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("literals")
    {
        const std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "123", { 123.0 } }
            , { "\"here is a string\"", { std::string("here is a string") } }
            , { "true", { true } }
            , { "false", { false } }
            , { "nil", {} }
        };

        for (const auto& [source, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, source);

            REQUIRE(!program.has_errors());
            REQUIRE(interpreter.functions().size() == 0);

            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            switch (expected_result.type) {
            case fk::lang::ExprResultType::RT_NUMBER: REQUIRE(expected_result.n == actual_result.n);     break;
            case fk::lang::ExprResultType::RT_STRING: REQUIRE(expected_result.str == actual_result.str); break;
            case fk::lang::ExprResultType::RT_BOOL:   REQUIRE(expected_result.b == actual_result.b);     break;
            case fk::lang::ExprResultType::RT_NIL:                                                       break;
            default: FAIL("unknown ExprResultType not accounted for");
            };
        }
    }

    SECTION("lambda, rvalue")
    {
        const std::string source = R"(
            function := fn (a, b) {
                return a + b
            }
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.functions().size() == 1);

        fk::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == fk::lang::ExprResultType::RT_CALLABLE);
    }

    SECTION("command")
    {
        const std::string source = R"(
            result := $(echo hello)
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        fk::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(identifier.str == "hello\n");
    }

    SECTION("command, piping")
    {
        const std::string source = R"(
            result := $(echo hello | tr -s 'h' "j")
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        fk::lang::ExprResult identifier = results[0];
        REQUIRE(identifier.type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(identifier.str == "jello\n");
    }

    SECTION("parenthetic expression")
    {
        const std::string source = R"(
            result := ( 1 + 2 )
        )";
        
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        fk::lang::ExprResult identifier = results.back();
        REQUIRE(identifier.type == fk::lang::ExprResultType::RT_NUMBER);
        REQUIRE(identifier.n == 3);
    }
}

TEST_CASE("call expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("function call, non-recursive")
    {
        const std::string source = R"(
            fn foo() {
                return "foobar"
            }

            foo()
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("foo"));

        REQUIRE(results.size() == 3);

        REQUIRE(results[0].type == fk::lang::ExprResultType::RT_CALLABLE);
        
        REQUIRE(results[1].type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(results[1].str == "foobar");
       
        REQUIRE(results[2].type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(results[2].str == "foobar");
    }

    SECTION("function call, recursive")
    {
        const std::string source = R"(
            fn fib(n) {
                # base case
                if n <= 1 { return n }

                # general case
                return fib(n - 2) + fib(n - 1)
            }

            fib(3)
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("fib"));

        fk::lang::ExprResult result = results.back();
        REQUIRE(result.type == fk::lang::ExprResultType::RT_NUMBER);
        REQUIRE(result.n == 2);
    }

    SECTION("function call, no return statement -- should return nil")
    {
        const std::string source = R"(
            fn foo() {
                "bar"
            }

            foo()
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.has_function("foo"));

        fk::lang::ExprResult result = results.back();
        REQUIRE(result.type == fk::lang::ExprResultType::RT_NIL);
    }
    
    SECTION("lambda call")
    {
        const std::string source = R"(
            f := fn (a, b) {
                return a + b
            }

            f("a", "b")
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().contains("f"));

        fk::lang::ExprResult result = results.back();
        REQUIRE(result.type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(result.str == "ab");
    }

    SECTION("lambda call, no return statement -- should return nil")
    {
        const std::string source = R"(
            f := fn (a, b) {
                a + b
            }

            f("a", "b")
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().contains("f"));

        fk::lang::ExprResult result = results.back();
        REQUIRE(result.type == fk::lang::ExprResultType::RT_NIL);
    }
}

TEST_CASE("unary expressions", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("operator (!), boolean")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "!true", { false } }
            , { "!false", { true } }
            , { "!(1 == 2)", { true } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("operator (!), non-boolean")
    {
        const std::string source = R"(
            !9
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }

    SECTION("operator (-), number")
    {
        const std::string source = R"(
            -2
        )";

        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());

        fk::lang::ExprResult result = results.back();
        REQUIRE(result.type == fk::lang::ExprResultType::RT_NUMBER);
        REQUIRE(result.n == -2);
    }

    SECTION("operator (-), non-number")
    {
        const std::string source = R"(
            -"what"
        )";

        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("PEMDAS", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("factors, numbers")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "4 / 2", fk::lang::Number{ 2 } }
            , { "4.2 / 2", fk::lang::Number{ 2.1 } }
            , { "6 / (1 + 1)", fk::lang::Number{ 3 } }
            , { "4 * 3", fk::lang::Number{ 12 } }
            , { "2 * 8.3", fk::lang::Number{ 16.6 } }
            , { "(2 * 3) / 2", fk::lang::Number{ 3 } }
            , { "12 / 3 * 2", fk::lang::Number{ 8 } }
            , { "12 * 3 / 2", fk::lang::Number{ 18 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.n == expected_result.n);
        }
    }    
    
    SECTION("factors, non-numbers")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "true / 2", {} }
            , { "\"fwat\" / 2", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("terms, numbers")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 - 2", fk::lang::Number{ -1 } }
            , { "2 + 5.4", fk::lang::Number{ 7.4 } }
            , { "1 - 2 + 3", fk::lang::Number{ 2 } }
            , { "2 + 3 - 1", fk::lang::Number{ 4 } }
            , { "1 - 3 + 2", fk::lang::Number{ 0 } }
            , { "7 - (2 + 3)", fk::lang::Number{ 2 } }
            , { "7 + (2 - 3)", fk::lang::Number{ 6 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.n == expected_result.n);
        }
    }    
    
    SECTION("terms, non-number")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 + true", {} }
            , { "\"fwat\" - true", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("terms, string operator(+)")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "\"foo\" + \"bar\"", std::string{"foobar"} }
            , { "\"\" + \"huh\"", std::string{"huh"} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.str == expected_result.str);
        }
    }

    SECTION("interleaved terms and factors")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 + 2 * 3", fk::lang::Number{ 7 } }
            , { "24 / (3 * 4)", fk::lang::Number{ 2 } }
            , { "8 + 2 * 3 / 2", fk::lang::Number{ 11 } }
            , { "(1 - (2 * 3)) * 2 * (21 / 7)", fk::lang::Number{ -24 } }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.str == expected_result.str);
        }
    }

    SECTION("divide by zero")
    {
        REQUIRE_THROWS(interpret(interpreter, "3 / 0"));
    }
}

TEST_CASE("ordering", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("comparison")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "2 > 1", true }
            , { "2 < 3", true }
            , { "1 >= 1", true }
            , { "5 <= 4", false }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("comparison, non-numbers")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "2 > \"foo\"", {} }
            , { "2 < true", {} }
            , { "1 >= \"\"", {} }
            , { "5 <= def () {}", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("equality")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 != 2", true }
            , { "3 == 2", false }
            , { "true == true", true}
            , { "false != true", true}
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("equality, non-numbers")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 != \"foo\"", {} }
            , { "false == 9.1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }
}

TEST_CASE("boolean", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("and")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "true && true", true }
            , { "true && false", false }
            , { "false && false", false }
            , { "false && true", false }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("and, non-boolean")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "2 && \"foo\"", {} }
            , { "true && -1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("and, strict evaluation")
    {
        const std::string source = R"(
            count := 0

            fn update() {
                count = count + 1
                return count
            }

            if update() > 0 && update() < 0 {
                print "yay"
            } else {
                print "nay"
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("count").value().n == 2);
    }

    SECTION("or")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "true || true", true }
            , { "true || false", true }
            , { "false || true", true}
            , { "false || false", false}
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.b == expected_result.b);
        }
    }

    SECTION("or, non-boolean")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "1 || \"foo\"", {} }
            , { "false || 9.1", {} }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            REQUIRE_THROWS(interpret(interpreter, src));
        }
    }

    SECTION("or, strict evaluation")
    {
        const std::string source = R"(
            count := 0

            fn update() {
                count = count + 1
                return count
            }

            if update() > 0 || update() < 0 {
                print "yay"
            } else {
                print "nay"
            }
        )";

        INFO(source);
        auto [program, results] = interpret(interpreter, source);

        REQUIRE(!program.has_errors());
        REQUIRE(interpreter.environment().value("count").value().n == 2);
    }
}

TEST_CASE("arrays", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("array expressions")
    {
        std::unordered_map<std::string, fk::lang::ExprResult> src_to_expected_result = {
            { "[1, 2]", fk::lang::Array(std::vector<fk::lang::ExprResult>{fk::lang::Number{1}, fk::lang::Number{2}}) }
            , { "[]", fk::lang::Array(std::vector<fk::lang::ExprResult>{}) }
        };

        for (const auto& [src, expected_result] : src_to_expected_result) {
            INFO(src);
            auto [program, results] = interpret(interpreter, src);

            REQUIRE(!program.has_errors());
            fk::lang::ExprResult actual_result = results.back();

            REQUIRE(actual_result.type == expected_result.type);
            REQUIRE(actual_result.array == expected_result.array);
        }
    }

    SECTION("index expressions")
    {
        const std::string source = R"(
            a := [1, 2]
            a[0] + a[1]
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        fk::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == fk::lang::ExprResultType::RT_NUMBER);
        REQUIRE(actual_result.n == 3);
    }

    SECTION("index expressions, out of range")
    {
        const std::string source = R"(
            a := [1, 2]
            a[3]
        )";

        INFO(source);
        
        REQUIRE_THROWS(interpret(interpreter, source));
    }
}

TEST_CASE("dicts", "[interpreter]")
{
    TracingInterpreter interpreter(std::make_unique<fk::lang::Interpreter>());

    SECTION("dict declaration")
    {
        const std::string source = R"(
            a := {
                a: "b",
                c: 2,
                d: []
            }
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        fk::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == fk::lang::ExprResultType::RT_DICT);

        std::initializer_list<std::string> keys{"a", "c", "d"};
        for (const auto& k : keys) {
            REQUIRE(actual_result.dict.value(k)->key.type == fk::lang::ExprResultType::RT_STRING);
        }
    }

    SECTION("dict declaration, key expression")
    {
        const std::string source = R"(
            a := {
                ["a" + "b"]: "abc"
            }
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        fk::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == fk::lang::ExprResultType::RT_DICT);

        REQUIRE(actual_result.dict.value(std::string{"ab"})->key.type == fk::lang::ExprResultType::RT_STRING);  
        REQUIRE(actual_result.dict.value(std::string{"ab"})->value.str == "abc");      
    }

    SECTION("dict lookup, string")
    {
        const std::string source = R"(
            a := {
                f: "g"
            }

            a["f"]
        )";

        INFO(source);
        
        auto [program, results] = interpret(interpreter, source);
        REQUIRE(!program.has_errors());

        fk::lang::ExprResult actual_result = results.back();
        REQUIRE(actual_result.type == fk::lang::ExprResultType::RT_STRING);
        REQUIRE(actual_result.str == "g");    
    }

    SECTION("dict lookup, non-string")
    {
        const std::string source = R"(
            a := {
                f: "g"
            }

            a[x]
        )";

        INFO(source);
        REQUIRE_THROWS(interpret(interpreter, source));    
    }

}
