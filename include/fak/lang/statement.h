#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/token.h>

namespace fk::lang {

// forward declare statement types for visitor
struct PrintStatement;
struct ExpressionStatement;
struct VariableDeclaration;
struct AssignmentStatement;
struct BlockStatement;
struct IfStatement;
struct WhileStatement;
struct FunctionDeclaration;
struct ReturnStatement;

template <class R>
struct StatementVisitor {
    virtual ~StatementVisitor() = default;

    virtual R visit(PrintStatement *stmt) = 0;
    virtual R visit(ExpressionStatement *stmt) = 0;
    virtual R visit(VariableDeclaration *stmt) = 0;
    virtual R visit(AssignmentStatement *stmt) = 0;
    virtual R visit(BlockStatement *stmt) = 0;
    virtual R visit(IfStatement *stmt) = 0;
    virtual R visit(WhileStatement *stmt) = 0;
    virtual R visit(FunctionDeclaration *stmt) = 0;
    virtual R visit(ReturnStatement *stmt) = 0;
};

struct Statement;
using StatementPtr = std::unique_ptr<Statement>;

struct Statement
{
    virtual ~Statement() = default;

    virtual void accept(StatementVisitor<void> *visitor) = 0;
    virtual std::string accept(StatementVisitor<std::string> *visitor) = 0;
    virtual StatementPtr clone() const noexcept = 0;
};


template <class T, class... Args>
StatementPtr make_statement(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct PrintStatement
    : public Statement
{
    ExpressionPtr expr;

    PrintStatement(ExpressionPtr expr)
        : expr(std::move(expr)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<PrintStatement>(expr->clone());
    }
};

struct ExpressionStatement
    : public Statement
{
    ExpressionPtr expr;

    ExpressionStatement(ExpressionPtr expr)
        : expr(std::move(expr)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<ExpressionStatement>(expr->clone());
    }
};

struct AssignmentStatement
    : public Statement
{
    Token name;
    ExpressionPtr initializer;

    AssignmentStatement(Token name, ExpressionPtr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<AssignmentStatement>(name, initializer->clone());
    }
};

enum class StorageClass
{
    LOCAL,
    EXPORT
};

struct VariableDeclaration
    : public Statement
{
    Token name;
    ExpressionPtr initializer;
    StorageClass storage_class;

    VariableDeclaration(Token name, ExpressionPtr initializer, StorageClass storage_class)
        : name(std::move(name)), initializer(std::move(initializer)), storage_class(storage_class) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<VariableDeclaration>(name, initializer->clone(), storage_class);
    }
};

struct BlockStatement
    : public Statement
{
    std::vector<StatementPtr> statements;

    BlockStatement(std::vector<StatementPtr> statements)
        : statements(std::move(statements)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        std::vector<StatementPtr> cloned;
        for (const auto& stmt : statements) {
            cloned.push_back(stmt->clone());
        }

        return make_statement<BlockStatement>(std::move(cloned));
    }
};

struct IfStatement
    : public Statement
{
    ExpressionPtr condition;
    StatementPtr then_block;
    // TODO: this may be nullptr Consider implementing a Null Statement just to prevent checking for null
    StatementPtr else_block;

    IfStatement(ExpressionPtr condition, StatementPtr then_block, StatementPtr else_block)
        : condition(std::move(condition)), then_block(std::move(then_block)), else_block(std::move(else_block)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<IfStatement>(condition->clone(), then_block->clone(), else_block ? else_block->clone() : nullptr);
    }
};

struct WhileStatement
    : public Statement
{
    ExpressionPtr condition;
    StatementPtr body;

    WhileStatement(ExpressionPtr condition, StatementPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<WhileStatement>(condition->clone(), body->clone());
    }
};

struct FunctionDeclaration
    : public Statement
{
    Token name;
    std::vector<Token> params;
    StatementPtr body;

    FunctionDeclaration(Token name, std::vector<Token> params, StatementPtr body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<FunctionDeclaration>(name, params, body->clone());
    }
};

struct ReturnStatement
    : public Statement
{
    ExpressionPtr expr;

    ReturnStatement(ExpressionPtr expr)
        : expr(std::move(expr)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(StatementVisitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual StatementPtr clone() const noexcept override
    {
        return make_statement<ReturnStatement>(expr->clone());
    }
};

}
