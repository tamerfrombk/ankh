#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <ankh/lang/expr.h>
#include <ankh/lang/token.h>

namespace ankh::lang {

// forward declare statement types for visitor
struct ExpressionStatement;
struct VariableDeclaration;
struct AssignmentStatement;
struct CompoundAssignment;
struct IncOrDecIdentifierStatement;
struct BlockStatement;
struct IfStatement;
struct WhileStatement;
struct ForStatement;
struct BreakStatement;
struct FunctionDeclaration;
struct ReturnStatement;


template <class R>
struct StatementVisitor {
    virtual ~StatementVisitor() = default;
    
    virtual R visit(ExpressionStatement *stmt) = 0;
    virtual R visit(VariableDeclaration *stmt) = 0;
    virtual R visit(AssignmentStatement *stmt) = 0;
    virtual R visit(IncOrDecIdentifierStatement* stmt) = 0;
    virtual R visit(CompoundAssignment* stmt) = 0;
    virtual R visit(BlockStatement *stmt) = 0;
    virtual R visit(IfStatement *stmt) = 0;
    virtual R visit(WhileStatement *stmt) = 0;
    virtual R visit(ForStatement *stmt) = 0;
    virtual R visit(BreakStatement *stmt) = 0;
    virtual R visit(FunctionDeclaration *stmt) = 0;
    virtual R visit(ReturnStatement *stmt) = 0;
};

struct Statement;

using StatementPtr = std::unique_ptr<Statement>;

struct Statement
{
    virtual ~Statement() = default;

    virtual void accept(StatementVisitor<void> *visitor) = 0;
    virtual std::string stringify() const noexcept = 0;
};


template <class T, class... Args>
StatementPtr make_statement(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

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

    virtual std::string stringify() const noexcept override
    {
        return expr->stringify();
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

    virtual std::string stringify() const noexcept override
    {
        return name.str + " = " + initializer->stringify();
    }
};

struct CompoundAssignment
    : public Statement
{
    Token target;
    Token op;
    ExpressionPtr value;

    CompoundAssignment(Token target, Token op, ExpressionPtr value)
        : target(std::move(target)), op(std::move(op)), value(std::move(value)) {}

    virtual void accept(StatementVisitor<void>* visitor) override
    {
        return visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return target.str + " " + op.str + " " + value->stringify();
    }
};

enum class StorageClass
{
    LOCAL,
    CONST
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

    virtual std::string stringify() const noexcept override
    {
        std::string result;
        switch (storage_class) {
        case StorageClass::LOCAL:  result = "let";     break;
        case StorageClass::CONST:  result = "const";     break;
        default:                   ANKH_FATAL("unknown storage_class");
        }

        return result + " " + name.str + " = " + initializer->stringify();
    }

    bool is_local() const noexcept
    {
        return storage_class == StorageClass::LOCAL;
    }

    bool is_const() const noexcept
    {
        return storage_class == StorageClass::CONST;
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

    virtual std::string stringify() const noexcept override
    {
        if (statements.empty()) {
            return "{}";
        }

        std::string result = "{\n" + statements[0]->stringify();
        for (size_t i = 1; i < statements.size(); ++i) {
            result += "\n";
            result += statements[i]->stringify();
        }
        result += "}";

        return result;
    }
};

template <class Derived>
struct IncOrDecStatement
    : public Statement
{
    Token op;
    ExpressionPtr expr;

    IncOrDecStatement(Token op, ExpressionPtr expr)
        : op(std::move(op)), expr(std::move(expr)) {}

    virtual void accept(StatementVisitor<void>* visitor) override
    {
        visitor->visit(static_cast<Derived*>(this));
    }

    virtual std::string stringify() const noexcept override
    {
        return op.str + expr->stringify();
    }
};

struct IncOrDecIdentifierStatement 
    : public IncOrDecStatement<IncOrDecIdentifierStatement> 
{ 
    using IncOrDecStatement::IncOrDecStatement; 
};

struct IfStatement
    : public Statement
{
    Token marker;
    ExpressionPtr condition;
    StatementPtr then_block;
    // TODO: this may be nullptr Consider implementing a Null Statement just to prevent checking for null
    StatementPtr else_block;

    IfStatement(Token marker, ExpressionPtr condition, StatementPtr then_block, StatementPtr else_block)
        : marker(std::move(marker))
        , condition(std::move(condition))
        , then_block(std::move(then_block))
        , else_block(std::move(else_block)) 
    {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return "if " + condition->stringify() + " " + then_block->stringify() + (else_block ? " " + else_block->stringify() : "");
    }
};

struct WhileStatement
    : public Statement
{
    Token marker;
    ExpressionPtr condition;
    StatementPtr body;

    WhileStatement(Token marker, ExpressionPtr condition, StatementPtr body)
        : marker(std::move(marker)), condition(std::move(condition)), body(std::move(body)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return "while " + condition->stringify() + " " + body->stringify();
    }
};

struct ForStatement
    : public Statement
{
    Token marker;
    StatementPtr init;
    ExpressionPtr condition;
    StatementPtr mutator;
    StatementPtr body;

    ForStatement(Token marker, StatementPtr init, ExpressionPtr condition, StatementPtr mutator, StatementPtr body)
        : marker(std::move(marker))
        , init(std::move(init))
        , condition(std::move(condition))
        , mutator(std::move(mutator))
        , body(std::move(body))
    {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return "for " 
            + ( init      ? init->stringify()       + "; " : "" )
            + ( condition ? condition->stringify()  + "; " : "" )
            + ( mutator   ? mutator->stringify()    + "; " : "" )
            + body->stringify();
    }
};

struct BreakStatement
    : public Statement
{
    Token tok;
    
    BreakStatement(Token tok)
        : tok(std::move(tok)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return tok.str; 
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

    virtual std::string stringify() const noexcept override
    {
        std::string result("fn (");
        if (params.size() > 0) {
            result += params[0].str;
            for (size_t i = 1; i < params.size(); ++i) {
                result += ", ";
                result += params[i].str;
            }
        }
        result += ") ";

        result += body->stringify();

        return result;
    }
};

struct ReturnStatement
    : public Statement
{
    Token tok;
    ExpressionPtr expr;

    ReturnStatement(Token tok, ExpressionPtr expr)
        : tok(std::move(tok)), expr(std::move(expr)) {}

    virtual void accept(StatementVisitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        return "return " + (expr ? expr->stringify() : "");
    }
};

}
