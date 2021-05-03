#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <initializer_list>
#include <functional>
#include <cerrno>
#include <cstring>

#include <fak/def.h>
#include <fak/log.h>

#include <fak/lang/interpreter.h>
#include <fak/lang/token.h>
#include <fak/lang/expr.h>
#include <fak/lang/interpretation_exception.h>

#include <fak/internal/pretty_printer.h>

FK_NO_RETURN static void panic(const std::string& msg)
{
    throw fk::lang::interpretation_exception("runtime error: " + msg);
}

FK_NO_RETURN static void panic(const fk::lang::expr_result& result, const std::string& msg)
{
    const std::string typestr = fk::lang::expr_result_type_str(result.type);

    throw fk::lang::interpretation_exception("runtime error: " + msg + " instead of '" + typestr + "'");
}

FK_NO_RETURN static void panic(const fk::lang::expr_result& left, const fk::lang::expr_result& right, const std::string& msg)
{
    const std::string ltypestr = fk::lang::expr_result_type_str(left.type);
    const std::string rtypestr = fk::lang::expr_result_type_str(right.type);

    throw fk::lang::interpretation_exception("runtime error: " + msg 
        + " with LHS '" + ltypestr + "', RHS '" + rtypestr + "'");
}

static bool operands_are(fk::lang::expr_result_type type, std::initializer_list<fk::lang::expr_result> elems) noexcept
{
    return std::all_of(elems.begin(), elems.end(), [=](const fk::lang::expr_result& result) {
        return type == result.type;
    });
}

static fk::lang::number to_num(const std::string& s)
{
    char *end;

    fk::lang::number n = std::strtod(s.c_str(), &end);
    if (*end == '\0') {
        return n;
    }

    const std::string errno_msg(std::strerror(errno));

    panic("'" + s + "' could not be turned into a number due to " + errno_msg);
}

static fk::lang::expr_result negate(const fk::lang::expr_result& result)
{
    if (result.type == fk::lang::expr_result_type::RT_NUMBER) {
        return fk::lang::expr_result::num(-1 * result.n);
    }

    panic(result, "unary (-) operator expects a number expression");
}

static fk::lang::expr_result invert(const fk::lang::expr_result& result)
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return fk::lang::expr_result::boolean(!(result.b));
    }

    panic(result, "(!) operator expects a boolean expression");
}

static fk::lang::expr_result eqeq(const fk::lang::expr_result& left, const fk::lang::expr_result& right)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::boolean(left.n == right.n);
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::boolean(left.str == right.str);
    }

    if (operands_are(fk::lang::expr_result_type::RT_BOOL, {left, right})) {
        return fk::lang::expr_result::boolean(left.b == right.b);
    }

    if (operands_are(fk::lang::expr_result_type::RT_NIL, {left, right})) {
        return fk::lang::expr_result::boolean(true);
    }

    panic(left, right, "unknown overload of (==) operator");
}

template <class BinaryOperation>
static fk::lang::expr_result 
arithmetic(const fk::lang::expr_result& left, const fk::lang::expr_result& right, BinaryOperation op)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::num(op(left.n, right.n));
    }

    panic(left, right, "unknown overload of arithmetic operator");
}

// We handle + separately as it has two overloads for numbers and strings
// The generic arithmetic() function overloads all of the general arithmetic operations
// on only numbers
static fk::lang::expr_result plus(const fk::lang::expr_result& left, const fk::lang::expr_result& right)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::num(left.n + right.n);
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::string(left.str + right.str);
    }

    panic(left, right, "unknown overload of (+) operator");
}

template <class Compare>
static fk::lang::expr_result compare(const fk::lang::expr_result& left, const fk::lang::expr_result& right, Compare cmp)
{
    if (operands_are(fk::lang::expr_result_type::RT_NUMBER, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.n, right.n));
    }

    if (operands_are(fk::lang::expr_result_type::RT_STRING, {left, right})) {
        return fk::lang::expr_result::boolean(cmp(left.str, right.str));
    }

    panic(left, right, "unknown overload of comparison operator");
}

// TODO: rethink truthiness -- do we really only want boolean expressions?
static bool truthy(const fk::lang::expr_result& result) noexcept
{
    if (result.type == fk::lang::expr_result_type::RT_BOOL) {
        return result.b;
    }

    return false;
}

static void print(const fk::lang::expr_result& result)
{
    switch (result.type) {
    case fk::lang::expr_result_type::RT_STRING:
        std::puts(result.str.c_str());
        break;
    case fk::lang::expr_result_type::RT_NUMBER:
        std::printf("%f\n", result.n);
        break;
    case fk::lang::expr_result_type::RT_BOOL:
        std::puts(result.b ? "true" : "false");
        break;
    case fk::lang::expr_result_type::RT_NIL:
        std::puts("nil");
        break;
    default:
        panic("unhandled expression result type");
    }
}

fk::lang::interpreter::interpreter()
{
    // initialize the global scope
    enter_new_scope();
}

void fk::lang::interpreter::interpret(const program& program)
{
    for (const auto& stmt : program) {
#ifndef NDEBUG
        fk::internal::pretty_printer printer;
        const std::string pretty = stmt->accept(&printer);
        fk::log::debug("%s\n", pretty.c_str());
#endif
        execute(stmt);
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(binary_expression *expr)
{
    const expr_result left  = evaluate(expr->left);
    const expr_result right = evaluate(expr->right);

    switch (expr->op.type) {
    case token_type::EQEQ:
        return eqeq(left, right);
    case token_type::NEQ:
        return invert(eqeq(left, right));
    case token_type::GT:
        return compare(left, right, std::greater<>{});
    case token_type::GTE:
        return compare(left, right, std::greater_equal<>{});
    case token_type::LT:
        return compare(left, right, std::less<>{});
    case token_type::LTE:
        return compare(left, right, std::less_equal<>{});
    case token_type::MINUS:
        return arithmetic(left, right, std::minus<>{});
    case token_type::PLUS:
        return plus(left, right);
    case token_type::STAR:
        return arithmetic(left, right, std::multiplies<>{});
    case token_type::FSLASH:
        // TODO: right now, dividing by 0 yields 'inf' revisit this and make sure that's the behavior we want for the language
        return arithmetic(left, right, std::divides<>{});
    default:
        panic(left, right, "unknown binary operator (" + expr->op.str + ")");
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(unary_expression *expr)
{
    const expr_result result = evaluate(expr->right);
    switch (expr->op.type) {
    case token_type::MINUS:
        return negate(result);
    case token_type::BANG:
        return invert(result);
    default:
        panic(result, "unknown unary (" + expr->op.str + ") operator");
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(literal_expression *expr)
{
    switch (expr->literal.type) {
    case token_type::NUMBER:
        return expr_result::num(to_num(expr->literal.str));
    case token_type::STRING:
        return expr_result::string(expr->literal.str);
    case token_type::BTRUE:
        return expr_result::boolean(true);
    case token_type::BFALSE:
        return expr_result::boolean(false);
    case token_type::NIL:
        return expr_result::nil();
    default:
        panic("unkown literal expression: '" + expr->literal.str + "'");
    }
}

fk::lang::expr_result fk::lang::interpreter::visit(paren_expression *expr)
{
    return evaluate(expr->expr);
}

fk::lang::expr_result fk::lang::interpreter::visit(identifier_expression *expr)
{
    for (auto it = env_.rbegin(); it != env_.rend(); ++it) {
        auto possible_value = it->value(expr->name.str); 
        if (possible_value.has_value()) {
            return possible_value.value();
        }
    }

    panic("identifier " + expr->name.str + " is not defined in the current scope");
}

fk::lang::expr_result fk::lang::interpreter::visit(and_expression *expr)
{
    const expr_result left = evaluate(expr->left);
    if (!truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

fk::lang::expr_result fk::lang::interpreter::visit(or_expression *expr)
{
    const expr_result left = evaluate(expr->left);
    if (truthy(left)) {
        return left;
    }

    return evaluate(expr->right);
}

void fk::lang::interpreter::visit(print_statement *stmt)
{
    const expr_result result = evaluate(stmt->expr);
    print(result);
}

void fk::lang::interpreter::visit(expression_statement *stmt)
{
    evaluate(stmt->expr);
}

void fk::lang::interpreter::visit(assignment_statement *stmt)
{
    const expr_result result = evaluate(stmt->initializer);

    // TODO: think about adding scope specifiers (export, local, global) to allow the user
    // to mutate global variables with the same name or to add variables to the environment
    for (size_t i = 0; i < env_.size(); ++i) {
        auto& env = env_[i];
        if (env.contains(stmt->name.str)) {
            env.assign(stmt->name.str, result);
            fk::log::debug("'%s' found at scope '%d' with value ('%s, '%f')\n"
                , stmt->name.str.c_str(), i, fk::lang::expr_result_type_str(result.type).c_str(), result.n);
            return;
        }
    }
    current_scope().assign(stmt->name.str, result);
}

void fk::lang::interpreter::visit(block_statement *stmt)
{
    try {
        enter_new_scope();
        for (const statement_ptr& statement : stmt->statements) {
            execute(statement);
        }
        leave_current_scope();
    } catch (...) {
        leave_current_scope();
    }
}

void fk::lang::interpreter::visit(if_statement *stmt)
{
    const expr_result result = evaluate(stmt->condition);
    if (truthy(result)) {
        execute(stmt->then_block);
    } else if (stmt->else_block != nullptr) {
        execute(stmt->else_block);
    }
}

void fk::lang::interpreter::visit(while_statement *stmt)
{
    expr_result result = evaluate(stmt->condition);
    while (truthy(result)) {
        execute(stmt->body);
        result = evaluate(stmt->condition);
    }
}

fk::lang::expr_result fk::lang::interpreter::evaluate(expression_ptr& expr)
{
    return expr->accept(this);
}

void fk::lang::interpreter::execute(const statement_ptr& stmt)
{
    stmt->accept(this);
}

void fk::lang::interpreter::enter_new_scope() noexcept
{
    env_.emplace_back();
}

void fk::lang::interpreter::leave_current_scope() noexcept
{
    env_.pop_back();
}

fk::lang::environment& fk::lang::interpreter::current_scope() noexcept
{
    return env_.back();
}
