#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/env.h>

namespace fk::lang {

class Interpreter
    : public ExpressionVisitor<ExprResult>
    , public StatementVisitor<void>
{
public:
    Interpreter();

    void interpret(const Program& program);

    virtual ExprResult visit(BinaryExpression *expr) override;
    virtual ExprResult visit(UnaryExpression *expr) override;
    virtual ExprResult visit(LiteralExpression *expr) override;
    virtual ExprResult visit(ParenExpression *expr) override;
    virtual ExprResult visit(IdentifierExpression *expr) override;
    virtual ExprResult visit(AndExpression *expr) override;
    virtual ExprResult visit(OrExpression *expr) override;
    virtual ExprResult visit(CallExpression *expr) override;

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
    ExprResult evaluate(const ExpressionPtr& expr);
    void execute(const StatementPtr& stmt);

    void enter_new_scope() noexcept;
    void leave_current_scope() noexcept;
    Environment& current_scope() noexcept;
    size_t scope() const noexcept;

private:
    std::vector<Environment> env_;

    // TODO: this assumes all functions are in global namespace
    // That's OK for now but needs to be revisited when implementing modules
    std::unordered_map<std::string, FunctionDeclaration*> functions_;

};

}

