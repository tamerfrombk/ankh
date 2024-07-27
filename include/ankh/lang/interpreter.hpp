#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <ankh/lang/expr_result.hpp>
#include <ankh/lang/expr.hpp>
#include <ankh/lang/statement.hpp>
#include <ankh/lang/program.hpp>
#include <ankh/lang/lambda.hpp>
#include <ankh/lang/env.hpp>
#include <ankh/lang/callable.hpp>

namespace ankh::lang {

class Interpreter
    : public ExpressionVisitor<ExprResult>
    , public StatementVisitor<void>
{
public:
    Interpreter();

    void interpret(Program&& program);

    virtual ExprResult evaluate(const ExpressionPtr& expr);
    void execute(const StatementPtr& stmt);

    void execute_block(const BlockStatement *stmt, EnvironmentPtr<ExprResult> environment);

    // Builtins

    void print(const std::vector<ExprResult>& args) const;
    void exit(const std::vector<ExprResult>& args) const;
    void length(const std::vector<ExprResult>& args) const;
    void cast_int(const std::vector<ExprResult>& args) const;
    void append(const std::vector<ExprResult>& args) const;
    void str(const std::vector<ExprResult>& args) const;
    void keys(const std::vector<ExprResult>& args) const;
    void exportfn(const std::vector<ExprResult>& args) const;

    inline const Environment<ExprResult>& environment() const noexcept
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
    virtual ExprResult visit(IndexExpression *expr) override;
    virtual ExprResult visit(SliceExpression *expr) override;
    virtual ExprResult visit(DictionaryExpression *expr) override;
    virtual ExprResult visit(StringExpression *expr) override;

    virtual void visit(ExpressionStatement *stmt) override;
    virtual void visit(VariableDeclaration *stmt) override;
    virtual void visit(AssignmentStatement *stmt) override;
    virtual void visit(CompoundAssignment* stmt) override;
    virtual void visit(IncOrDecIdentifierStatement* stmt) override;
    virtual void visit(BlockStatement *stmt) override;
    virtual void visit(IfStatement *stmt) override;
    virtual void visit(WhileStatement *stmt) override;
    virtual void visit(ForStatement *stmt) override;
    virtual void visit(BreakStatement *stmt) override;
    virtual void visit(FunctionDeclaration *stmt) override;
    virtual void visit(ReturnStatement *stmt) override;

    std::string substitute(const StringExpression *expr);
    ExprResult evaluate_single_expr(const Token& marker, const std::string& str);
    void declare_function(FunctionDeclaration *decl, EnvironmentPtr<ExprResult> env);
private:
    EnvironmentPtr<ExprResult> current_env_;
    EnvironmentPtr<ExprResult> global_;
    std::vector<Program> programs_;

    class ScopeGuard {
    public:
        ScopeGuard(ankh::lang::Interpreter *interpreter, ankh::lang::EnvironmentPtr<ExprResult> enclosing);
        ~ScopeGuard();
    private:
        ankh::lang::Interpreter *interpreter_;
        ankh::lang::EnvironmentPtr<ExprResult> prev_;
    };

    // TODO: this assumes all functions are in global namespace
    // That's OK for now but needs to be revisited when implementing modules
    std::unordered_map<std::string, CallablePtr> functions_;
};

}

