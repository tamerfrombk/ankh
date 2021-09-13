#include "ankh/lang/expr_result.h"
#include <ankh/def.h>
#include <ankh/log.h>

#include <ankh/lang/token.h>
#include <ankh/lang/static_analyzer.h>
#include <ankh/lang/exceptions.h>
#include <ankh/lang/lambda.h>

ankh::lang::HopTable ankh::lang::StaticAnalyzer::resolve(const Program& program)
{
    hop_table_.clear();
    scopes_.clear();

    // initialize global scope
    begin_scope();
    begin_analysis(FunctionType::NONE, LoopType::NONE);

    for (const auto& stmt : program.statements) {
        analyze(stmt);
    }

#ifndef NDEBUG
    //ANKH_DEBUG("========================== HOP TABLE BEGIN ===================================");
    for (const auto& [entity, hops] : hop_table_) {
        const std::string serialized = reinterpret_cast<const Expression*>(entity)
            ? static_cast<const Expression*>(entity)->stringify()
            : reinterpret_cast<const Statement*>(entity)
            ? static_cast<const Statement*>(entity)->stringify()
            : "";

        ANKH_DEBUG("HOP TABLE: '{}' @ {} => {}", serialized, entity, hops);
    }
    //ANKH_DEBUG("========================== HOP TABLE END ===================================");
#endif

    return hop_table_;
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(BinaryExpression *expr)
{
    analyze(expr->left);
    analyze(expr->right);

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(UnaryExpression *expr)
{
    ExprResultType type = analyze(expr->right);
    
    if (expr->op.type == TokenType::BANG && type != ExprResultType::RT_BOOL) {
        panic<ParseException>("{}:{}, (!) operator expects a boolean expression"
            , expr->op.line, expr->op.col);
    }

    if (expr->op.type == TokenType::MINUS && type != ExprResultType::RT_NUMBER) {
        panic<ParseException>("{}:{}, unary (-) operator expects a number expression"
            , expr->op.line, expr->op.col);
    }

    return type;
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(LiteralExpression *expr)
{
    switch (expr->literal.type) {
    case TokenType::NUMBER:
        return ExprResultType::RT_NUMBER;
    case TokenType::STRING:
        return ExprResultType::RT_STRING;
    case TokenType::ANKH_TRUE:
    case TokenType::ANKH_FALSE:
        return ExprResultType::RT_BOOL;
    case TokenType::NIL:
        return ExprResultType::RT_NIL;
    default:
        panic<ParseException>("{}:{}, unknown literal expression type {}'"
            , expr->literal.line, expr->literal.col, token_type_str(expr->literal.type));
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(ParenExpression *expr)
{
    return analyze(expr->expr);
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(IdentifierExpression *expr)
{
    ANKH_DEBUG("static analyzer: analyzing '{}'", expr->stringify());

    if (is_declared_but_not_defined(expr->name)) {
        panic<ParseException>("can't read local variable in its own initializer");
    }

    resolve(expr, expr->name);

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(CallExpression *expr)
{
    analyze(expr->callee);
    for (const auto& arg : expr->args) {
        analyze(arg);
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(LambdaExpression *expr)
{
    ANKH_DEBUG("static analyzer: analyzing '{}'", expr->stringify());

    const Token name{ expr->generated_name, TokenType::IDENTIFIER, 0 , 0 };
    declare(name);
    define(name);

    begin_analysis(FunctionType::FUNCTION, current_analysis().loop_type);
    begin_scope();
    for (const auto& param : expr->params) {
        declare(param);
        define(param);
    }
    analyze(expr->body);

    end_scope();
    end_analysis();

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(CommandExpression *expr)
{
    // TODO: no alaysis or variable resolution is done here but this isn't quite
    // right since we have interpolated expressions
    ANKH_UNUSED(expr);

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(ArrayExpression *expr)
{
    for (const auto& elem : expr->elems) {
        analyze(elem);
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(IndexExpression *expr)
{
    analyze(expr->indexee);
    analyze(expr->index);

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(DictionaryExpression *expr)
{
    for (const auto& [k, v] : expr->entries) {
        analyze(k);
        analyze(v);
    }

    return {};
}

ankh::lang::ExprResult ankh::lang::StaticAnalyzer::visit(StringExpression *expr)
{
    ANKH_UNUSED(expr);

    return ExprResultType::RT_STRING;
}

void ankh::lang::StaticAnalyzer::visit(PrintStatement *stmt)
{
    analyze(stmt->expr);
}

void ankh::lang::StaticAnalyzer::visit(ExpressionStatement *stmt)
{
    analyze(stmt->expr);
}

void ankh::lang::StaticAnalyzer::visit(VariableDeclaration *stmt)
{
    ANKH_DEBUG("static analyzer: analyzing '{}'", stmt->stringify());
    
    declare(stmt->name);
    analyze(stmt->initializer);
    define(stmt->name);
}

void ankh::lang::StaticAnalyzer::visit(AssignmentStatement *stmt)
{
    ANKH_DEBUG("static analyzer: analyzing '{}'", stmt->stringify());

    analyze(stmt->initializer);
    resolve(stmt, stmt->name);
}

void ankh::lang::StaticAnalyzer::visit(CompoundAssignment* stmt)
{
    analyze(stmt->value);
    resolve(stmt, stmt->target);
}

void ankh::lang::StaticAnalyzer::visit(IncOrDecIdentifierStatement* stmt)
{
    analyze(stmt->expr);
}

void ankh::lang::StaticAnalyzer::visit(BlockStatement *stmt)
{
    begin_analysis(current_analysis().fn_type, current_analysis().loop_type);

    begin_scope();
    for (const auto& stmt : stmt->statements) {
        analyze(stmt);
    }
    end_scope();

    end_analysis();
}

void ankh::lang::StaticAnalyzer::visit(IfStatement *stmt)
{
    if (ExprResultType type = analyze(stmt->condition); type != ExprResultType::RT_BOOL) {
        panic<ParseException>("while statement condition must be boolean");
    }

    analyze(stmt->then_block);
    
    if (stmt->else_block != nullptr) {
        analyze(stmt->else_block);
    }
}

void ankh::lang::StaticAnalyzer::visit(WhileStatement *stmt)
{
    begin_analysis(current_analysis().fn_type, LoopType::LOOP);
    
    if (ExprResultType type = analyze(stmt->condition); type != ExprResultType::RT_BOOL) {
        panic<ParseException>("while statement condition must be boolean");
    }
    
    analyze(stmt->body);
    
    end_analysis();
}

void ankh::lang::StaticAnalyzer::visit(ForStatement *stmt)
{
    begin_analysis(current_analysis().fn_type, LoopType::LOOP);
    begin_scope();

    if (stmt->init)         { analyze(stmt->init);      }

    if (stmt->condition) { 
        ExprResultType type = analyze(stmt->condition);
        if (type != ExprResultType::RT_BOOL) {
            panic<ParseException>("for loop condition expression must be boolean");
        } 
    }

    if (stmt->mutator)      { analyze(stmt->mutator);   }
        
    analyze(stmt->body);

    end_scope();
    end_analysis();
}

void ankh::lang::StaticAnalyzer::visit(BreakStatement *stmt)
{
    ANKH_UNUSED(stmt);

    if (!in_loop_scope()) {
        panic<ParseException>("a break statement can only be within loop scope");
    }
}

void ankh::lang::StaticAnalyzer::visit(FunctionDeclaration *stmt)
{
    ANKH_DEBUG("static analyzer: analyzing '{}'", stmt->stringify());

    declare(stmt->name);
    define(stmt->name);

    // we can't define functions in loops so we hardcode NONE
    begin_analysis(FunctionType::FUNCTION, LoopType::NONE);
    begin_scope();
    for (const auto& param : stmt->params) {
        declare(param);
        define(param);
    }
    analyze(stmt->body);

    end_scope();
    end_analysis();
}

void ankh::lang::StaticAnalyzer::visit(ReturnStatement *stmt)
{
    if (!in_function_scope()) {
        panic<ParseException>("a return statement can only be within function scope");
    }

    ANKH_UNUSED(analyze(stmt->expr));
}

void ankh::lang::StaticAnalyzer::begin_scope()
{
    scopes_.emplace_back();
}

void ankh::lang::StaticAnalyzer::end_scope()
{
    scopes_.pop_back();
}

void ankh::lang::StaticAnalyzer::begin_analysis(FunctionType fn_type, LoopType loop_type) noexcept
{
    analyses_.emplace_back(fn_type, loop_type);
}

void ankh::lang::StaticAnalyzer::end_analysis() noexcept
{
    analyses_.pop_back();
}

ankh::lang::StaticAnalyzer::Analysis& ankh::lang::StaticAnalyzer::current_analysis() noexcept
{
    return analyses_.back();
}

const ankh::lang::StaticAnalyzer::Analysis& ankh::lang::StaticAnalyzer::current_analysis() const noexcept
{
    return analyses_.back();
}

bool ankh::lang::StaticAnalyzer::in_loop_scope() const noexcept
{
    return analyses_.back().loop_type != LoopType::NONE;
}

bool ankh::lang::StaticAnalyzer::in_function_scope() const noexcept
{
    return analyses_.back().fn_type != FunctionType::NONE;
}

ankh::lang::StaticAnalyzer::Scope& ankh::lang::StaticAnalyzer::top() noexcept
{
    return scopes_.back();
}

const ankh::lang::StaticAnalyzer::Scope& ankh::lang::StaticAnalyzer::top() const noexcept
{
    return scopes_.back();
}

void ankh::lang::StaticAnalyzer::declare(const ankh::lang::Token& token)
{
    ANKH_VERIFY(top().variables.count(token.str) == 0);
    
    top().variables.insert({token.str, false});
    
    ANKH_DEBUG("'{}' declared at scope {}", token.str, scopes_.size() - 1);
}

void ankh::lang::StaticAnalyzer::define(const ankh::lang::Token& token)
{
    ANKH_VERIFY(top().variables.count(token.str) > 0);

    top().variables[token.str] = true;

    ANKH_DEBUG("'{}' defined at scope {}", token.str, scopes_.size() - 1);
}

bool ankh::lang::StaticAnalyzer::is_declared_but_not_defined(const Token& token) const noexcept
{
    return top().variables.count(token.str) > 0 && top().variables.at(token.str) == false;
}

ankh::lang::ExprResultType ankh::lang::StaticAnalyzer::analyze(const ExpressionPtr& expr)
{
    return expr->accept(this).type;
}

void ankh::lang::StaticAnalyzer::analyze(const StatementPtr& stmt)
{
    stmt->accept(this);
}

void ankh::lang::StaticAnalyzer::resolve(const void *entity, const Token& name)
{
    for (auto it = scopes_.crbegin(); it != scopes_.crend(); ++it) {
        if (it->variables.count(name.str) > 0) {
            const size_t hops = it - scopes_.crbegin();
            ANKH_DEBUG("'{}' is {} hops away from current scope {}", name.str, hops, scopes_.size() - 1);
            ANKH_VERIFY(hop_table_.count(entity) == 0);
            hop_table_[entity] = hops;
            return;
        }
    }
}