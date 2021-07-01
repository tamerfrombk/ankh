#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/program.h>
#include <fak/lang/lambda.h>
#include <fak/lang/env.h>
#include <fak/lang/callable.h>

namespace fk::lang {

class Interpreter
    : public ExpressionVisitor<ExprResult>
    , public StatementVisitor<void>
{
public:
    Interpreter();

    void interpret(const Program& program);

    virtual ExprResult evaluate(const ExpressionPtr& expr);
    void execute(const StatementPtr& stmt);

    void execute_block(const BlockStatement *stmt, Environment *environment);

    inline const Environment& environment() const noexcept
    {
        return *current_env_;
    }

    inline const std::unordered_map<std::string, CallablePtr>& functions() const noexcept
    {
        return functions_;
    }

private:
    virtual ExprResult visit(BinaryExpression *expr) override;
    virtual ExprResult visit(UnaryExpression *expr) override;
    virtual ExprResult visit(LiteralExpression *expr) override;
    virtual ExprResult visit(ParenExpression *expr) override;
    virtual ExprResult visit(IdentifierExpression *expr) override;
    virtual ExprResult visit(CallExpression *expr) override;
    virtual ExprResult visit(LambdaExpression *expr) override;
    virtual ExprResult visit(CommandExpression *cmd) override;
    virtual ExprResult visit(ArrayExpression *cmd) override;
    virtual ExprResult visit(IndexExpression *cmd) override;

    virtual void visit(PrintStatement *stmt) override;
    virtual void visit(ExpressionStatement *stmt) override;
    virtual void visit(VariableDeclaration *stmt) override;
    virtual void visit(AssignmentStatement *stmt) override;
    virtual void visit(BlockStatement *stmt) override;
    virtual void visit(IfStatement *stmt) override;
    virtual void visit(WhileStatement *stmt) override;
    virtual void visit(FunctionDeclaration *stmt) override;
    virtual void visit(ReturnStatement *stmt) override;

private:
    EnvironmentPtr current_env_;

    class Scope {
    public:
        Scope(fk::lang::Interpreter *interpreter, fk::lang::Environment *enclosing);
        ~Scope();
    private:
        fk::lang::Interpreter *interpreter_;
        fk::lang::EnvironmentPtr prev_;
    };

    // TODO: this assumes all functions are in global namespace
    // That's OK for now but needs to be revisited when implementing modules
    std::unordered_map<std::string, CallablePtr> functions_;

};

}

