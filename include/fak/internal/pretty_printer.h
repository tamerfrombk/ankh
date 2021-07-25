#pragma once

#include <initializer_list>
#include <string>
#include <numeric>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/lambda.h>
#include <fak/lang/token.h>

#include <fak/def.h>
#include <fak/log.h>


namespace fk::internal {

class PrettyPrinter
    : public fk::lang::ExpressionVisitor<fk::lang::ExprResult>
    , public fk::lang::StatementVisitor<std::string>
{
public:
    inline virtual fk::lang::ExprResult visit(fk::lang::BinaryExpression *expr) override
    {
        return binary(expr->left, expr->op.str, expr->right);
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::UnaryExpression *expr) override
    {
        return expr->op.str + " " + stringify(expr->right);
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::LiteralExpression *expr) override
    {
        return expr->literal.str;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::ParenExpression *expr) override
    {
        return "( " + stringify(expr->expr) + " )";
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::IdentifierExpression *expr) override
    {
        return expr->name.str;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::CallExpression *expr) override
    {
        std::string result = stringify(expr->callee);
        
        result += "(";
        for (const auto& arg : expr->args) {
            result += stringify(arg);
            result += ", ";
        }

        if (expr->args.size() > 0) {
            result.pop_back(); // remove the extra space
            result.pop_back(); // remove the extra comma
        }
        result += ")";

        return result;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::LambdaExpression *lambda) override
    {
        std::string result("def ");

        result += lambda->generated_name;
        result += '(';
        for (const auto& param : lambda->params) {
            result += stringify(param);
            result += ", ";
        }

        if (lambda->params.size() > 0) {
            result.pop_back(); // remove the trailing space
            result.pop_back(); // remove the trailing comma
        }
        result += ") ";

        result += stringify(lambda->body);

        return result;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::CommandExpression *cmd) override
    {
        return "$(" + stringify(cmd->cmd) + ")";
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::ArrayExpression *arr) override
    {
        if (arr->size() == 0) {
            return "[]";
        }

        std::string result = "[" + stringify(arr->elems[0]);
        for (size_t i = 1; i < arr->size(); ++i) {
            result += ",";
            result += stringify(arr->elems[i]);
        }
        result += "]";

        return result;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::IndexExpression *arr) override
    {
        return stringify(arr->indexee) + "[" + stringify(arr->index) + "]";
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::DictionaryExpression *arr) override
    {
        if (arr->entries.empty()) {
            return "{}";
        }

        std::string result = "{ " + stringify(arr->entries[0]);
        for (size_t i = 1; i < arr->entries.size(); ++i) {
            result += "\n";
            result += ", ";
            result += stringify(arr->entries[i]);
        }
        result += "\n";
        result += "}";

        return result;
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::StringExpression *arr) override
    {
        return stringify(arr->str);
    }

    inline virtual fk::lang::ExprResult visit(fk::lang::AccessExpression *arr) override
    {
        return stringify(arr->accessible) + "." + stringify(arr->accessor);
    }

    inline virtual std::string visit(fk::lang::PrintStatement *stmt) override
    {
        return "print " + stringify(stmt->expr);
    }
    
    inline virtual std::string visit(fk::lang::ExpressionStatement *stmt) override
    {
        return stringify(stmt->expr);
    }

    inline virtual std::string visit(fk::lang::VariableDeclaration *stmt) override
    {
        std::string result("let ");
        result += stmt->name.str;
        result += " = ";
        result += stringify(stmt->initializer);

        return result;
    }
    
    inline virtual std::string visit(fk::lang::AssignmentStatement *stmt) override
    {
        std::string result = stmt->name.str;
        result += " = ";
        result += stringify(stmt->initializer);

        return result;
    }
    
    inline virtual std::string visit(fk::lang::BlockStatement *stmt) override
    {
        static const char *const tab = " "" "" "" ";

        std::string result;
        result += "{\n";
        for (auto& s : stmt->statements) {
            result += tab;
            result += stringify(s);
            result += '\n';
        }
        result += "}";

        return result;
    }
    
    inline virtual std::string visit(fk::lang::IfStatement *stmt) override
    {
        std::string result("if ");
        
        result += stringify(stmt->condition);
        result += " ";
        result += stringify(stmt->then_block);

        if (stmt->else_block) {
            result += " else ";
            result += stringify(stmt->else_block);
        }
        result += "\n";

        return result;
    }
    
    inline virtual std::string visit(fk::lang::WhileStatement *stmt) override
    {
        std::string result("while ");
        result += stringify(stmt->condition);
        result += " ";
        result += stringify(stmt->body);
        result += "\n";

        return result;
    }

    inline virtual std::string visit(fk::lang::FunctionDeclaration *stmt) override
    {
        std::string result("def ");

        result += stringify(stmt->name);
        
        result += "(";
        for (const auto& param : stmt->params) {
            result += stringify(param);
            result += ",";
        }

        if (stmt->params.size() > 0) {
            result.pop_back(); // remove the extra comma
        }
        result += ") ";

        result += stringify(stmt->body);
        result += "\n";

        return result;
    }

    inline virtual std::string visit(fk::lang::ReturnStatement *stmt) override
    {
        return "return " + stringify(stmt->expr);
    }

private:
    inline std::string stringify(const fk::lang::ExpressionPtr& expr)
    {
        return expr->accept(this).str;
    }

    inline std::string stringify(const fk::lang::StatementPtr& expr)
    {
        return expr->accept(this);
    }

    inline std::string stringify(const fk::lang::Token& token)
    {
        return token.str;
    }

    template <class T>
    inline std::string stringify(const fk::lang::Entry<T>& entry)
    {
        return stringify(entry.key) + " : " + stringify(entry.value);
    }

    inline std::string binary(const fk::lang::ExpressionPtr& left, const std::string& op, const fk::lang::ExpressionPtr& right)
    {
        return stringify(left) + " " + op + " " + stringify(right);
    }
};

}
