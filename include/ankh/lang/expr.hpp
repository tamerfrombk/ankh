#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ankh/lang/expr_result.hpp>
#include <ankh/lang/token.hpp>

namespace ankh::lang {

// forward declare our expression types for the visitor
struct BinaryExpression;
struct UnaryExpression;
struct LiteralExpression;
struct ParenExpression;
struct IdentifierExpression;
struct CallExpression;
struct LambdaExpression;
struct CommandExpression;
struct ArrayExpression;
struct IndexExpression;
struct SliceExpression;
struct DictionaryExpression;
struct StringExpression;

template <class R> struct ExpressionVisitor {
    virtual ~ExpressionVisitor() = default;

    virtual R visit(BinaryExpression *expr) = 0;
    virtual R visit(UnaryExpression *expr) = 0;
    virtual R visit(LiteralExpression *expr) = 0;
    virtual R visit(ParenExpression *expr) = 0;
    virtual R visit(IdentifierExpression *expr) = 0;
    virtual R visit(CallExpression *expr) = 0;
    virtual R visit(LambdaExpression *expr) = 0;
    virtual R visit(CommandExpression *expr) = 0;
    virtual R visit(ArrayExpression *expr) = 0;
    virtual R visit(IndexExpression *expr) = 0;
    virtual R visit(SliceExpression *expr) = 0;
    virtual R visit(DictionaryExpression *expr) = 0;
    virtual R visit(StringExpression *expr) = 0;
};

struct Expression;

using ExpressionPtr = std::unique_ptr<Expression>;

struct Expression {
    virtual ~Expression() = default;

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) = 0;
    virtual std::string stringify() const noexcept = 0;
};

template <class T> std::string stringify(const std::vector<T> &elems) noexcept {
    if (elems.empty()) {
        return "";
    }

    std::string result(elems[0]->stringify());
    for (size_t i = 1; i < elems.size(); ++i) {
        result += ", ";
        result += elems[i]->stringify();
    }

    return result;
}

template <class T, class... Args> ExpressionPtr make_expression(Args &&...args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct BinaryExpression : public Expression {
    ExpressionPtr left;
    Token op;
    ExpressionPtr right;

    BinaryExpression(ExpressionPtr left, Token op, ExpressionPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override {
        return left->stringify() + " " + op.str + " " + right->stringify();
    }
};

struct UnaryExpression : public Expression {
    Token op;
    ExpressionPtr right;

    UnaryExpression(Token op, ExpressionPtr right) : op(std::move(op)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return op.str + right->stringify(); }
};

struct LiteralExpression : public Expression {
    Token literal;

    LiteralExpression(Token literal) : literal(std::move(literal)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return literal.str; }

    bool is_number() const noexcept { return literal.type == TokenType::NUMBER; }
};

struct StringExpression : public Expression {
    Token str;

    StringExpression(Token str) : str(std::move(str)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return str.str; }
};

struct ParenExpression : public Expression {
    ExpressionPtr expr;

    ParenExpression(ExpressionPtr expr) : expr(std::move(expr)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return expr->stringify(); }
};

struct IdentifierExpression : public Expression {
    Token name;

    IdentifierExpression(Token name) : name(std::move(name)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return name.str; }
};

struct CallExpression : public Expression {
    Token marker;
    ExpressionPtr callee;
    std::vector<ExpressionPtr> args;

    CallExpression(Token marker, ExpressionPtr callee, std::vector<ExpressionPtr> args)
        : marker(std::move(marker)), callee(std::move(callee)), args(std::move(args)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override {
        return callee->stringify() + "(" + ankh::lang::stringify(args) + ")";
    }
};

struct CommandExpression : public Expression {
    Token cmd;

    CommandExpression(Token cmd) : cmd(std::move(cmd)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override { return cmd.str; }
};

struct ArrayExpression : public Expression {
    std::vector<ExpressionPtr> elems;

    ArrayExpression(std::vector<ExpressionPtr> elems) : elems(std::move(elems)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    size_t size() const noexcept { return elems.size(); }

    virtual std::string stringify() const noexcept override { return "[" + ankh::lang::stringify(elems) + "]"; }
};

struct IndexExpression : public Expression {
    Token marker;
    ExpressionPtr indexee;
    ExpressionPtr index;

    IndexExpression(Token marker, ExpressionPtr indexee, ExpressionPtr index)
        : marker(std::move(marker)), indexee(std::move(indexee)), index(std::move(index)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override {
        return indexee->stringify() + "[" + index->stringify() + "]";
    }
};

struct SliceExpression : public Expression {
    Token marker;
    ExpressionPtr indexee;
    ExpressionPtr begin, end;

    SliceExpression(Token marker, ExpressionPtr indexee, ExpressionPtr begin, ExpressionPtr end)
        : marker(std::move(marker)), indexee(std::move(indexee)), begin(std::move(begin)), end(std::move(end)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override {
        std::string begin_str = begin ? begin->stringify() : "";
        std::string end_str = end ? end->stringify() : "";
        return indexee->stringify() + "[" + begin_str + ":" + end_str + "]";
    }
};

struct DictionaryExpression : public Expression {
    Token marker;
    std::vector<Entry<ExpressionPtr>> entries;

    DictionaryExpression(Token marker, std::vector<Entry<ExpressionPtr>> entries)
        : marker(std::move(marker)), entries(std::move(entries)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override { return visitor->visit(this); }

    virtual std::string stringify() const noexcept override {
        if (entries.empty()) {
            return "{}";
        }

        std::string result("{\n");
        result += entries[0].key->stringify() + entries[0].value->stringify();
        for (const auto &[k, v] : entries) {
            result += "\n";
            result += k->stringify() + " : " + v->stringify();
        }
        result += "}\n";

        return result;
    }
};

} // namespace ankh::lang
