#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <ankh/def.h>

#include <ankh/lang/expr.h>
#include <ankh/lang/expr_result.h>
#include <ankh/lang/statement.h>
#include <ankh/lang/hop_table.h>
#include <ankh/lang/program.h>

namespace ankh::lang {

class StaticAnalyzer
    : public ExpressionVisitor<ExprResult>
    , public StatementVisitor<void>
{
public:
    HopTable resolve(const Program& program);

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
    virtual ExprResult visit(DictionaryExpression *expr) override;
    virtual ExprResult visit(StringExpression *expr) override;

    virtual void visit(PrintStatement *stmt) override;
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

private:

    enum class FunctionType {
        NONE,
        FUNCTION
    };

    enum class LoopType {
        NONE,
        LOOP
    };

    struct Scope {
        std::unordered_map<std::string, bool> variables;
    };

    struct Analysis {
        FunctionType fn_type;
        LoopType loop_type;

        Analysis(FunctionType fn_type, LoopType loop_type)
            : fn_type(fn_type), loop_type(loop_type) {}
    };

    void begin_scope();
    void end_scope();

    void begin_analysis(FunctionType fn_type, LoopType loop_type) noexcept;
    void end_analysis() noexcept;

    Analysis& current_analysis() noexcept;
    const Analysis& current_analysis() const noexcept;

    bool in_loop_scope() const noexcept;
    bool in_function_scope() const noexcept;

    Scope& top() noexcept;
    const Scope& top() const noexcept;

    void declare(const Token& token);
    void define(const Token& token);

    bool is_declared_but_not_defined(const Token& token) const noexcept;

    ANKH_NO_DISCARD ExprResultType analyze(const ExpressionPtr& expr);
    void analyze(const StatementPtr& stmt);

    void resolve(const void *entity, const Token& name);

private:
    std::vector<Scope> scopes_;
    std::vector<Analysis> analyses_;
    HopTable hop_table_;
};

}