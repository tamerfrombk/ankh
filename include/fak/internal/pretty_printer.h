#pragma once

#include <initializer_list>
#include <string>
#include <numeric>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/token.h>
#include <fak/log.h>

namespace fk::internal {

class pretty_printer
    : public fk::lang::expression_visitor<fk::lang::expr_result>
    , public fk::lang::statement_visitor<std::string>
{
public:
    inline virtual fk::lang::expr_result visit(fk::lang::binary_expression *expr) override
    {
        return fk::lang::expr_result::string(binary(expr->left, expr->op.str, expr->right));
    }

    inline virtual fk::lang::expr_result visit(fk::lang::unary_expression *expr) override
    {
        return fk::lang::expr_result::string(expr->op.str + " " + stringify(expr->right));
    }

    inline virtual fk::lang::expr_result visit(fk::lang::literal_expression *expr) override
    {
        return fk::lang::expr_result::string(expr->literal.str);
    }

    inline virtual fk::lang::expr_result visit(fk::lang::paren_expression *expr) override
    {
        return fk::lang::expr_result::string("( " + stringify(expr->expr) + " )");
    }

    inline virtual fk::lang::expr_result visit(fk::lang::identifier_expression *expr) override
    {
        return fk::lang::expr_result::string(expr->name.str);
    }

    inline virtual fk::lang::expr_result visit(fk::lang::and_expression *expr) override
    {
        return fk::lang::expr_result::string(binary(expr->left, "&&", expr->right));
    }

    inline virtual fk::lang::expr_result visit(fk::lang::or_expression *expr) override
    {
        return fk::lang::expr_result::string(binary(expr->left, "||", expr->right));
    }

    inline virtual fk::lang::expr_result visit(fk::lang::call_expression *expr) override
    {
        std::string result = stringify(expr->name);
        
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

        return fk::lang::expr_result::string(result);
    }

    inline virtual std::string visit(fk::lang::print_statement *stmt) override
    {
        return "print " + stringify(stmt->expr);
    }
    
    inline virtual std::string visit(fk::lang::expression_statement *stmt) override
    {
        return stringify(stmt->expr);
    }

    inline virtual std::string visit(fk::lang::variable_declaration *stmt) override
    {
        std::string result("let ");
        result += stmt->name.str;
        result += " = ";
        result += stringify(stmt->initializer);

        return result;
    }
    
    inline virtual std::string visit(fk::lang::assignment_statement *stmt) override
    {
        std::string result = stmt->name.str;
        result += " = ";
        result += stringify(stmt->initializer);

        return result;
    }
    
    inline virtual std::string visit(fk::lang::block_statement *stmt) override
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
    
    inline virtual std::string visit(fk::lang::if_statement *stmt) override
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
    
    inline virtual std::string visit(fk::lang::while_statement *stmt) override
    {
        std::string result("while ");
        result += stringify(stmt->condition);
        result += " ";
        result += stringify(stmt->body);
        result += "\n";

        return result;
    }

    inline virtual std::string visit(fk::lang::function_declaration *stmt) override
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

    inline virtual std::string visit(fk::lang::return_statement *stmt) override
    {
        return "return " + stringify(stmt->expr);
    }

private:
    inline std::string stringify(const fk::lang::expression_ptr& expr)
    {
        return expr->accept(this).str;
    }

    inline std::string stringify(const fk::lang::statement_ptr& expr)
    {
        return expr->accept(this);
    }

    inline std::string stringify(const fk::lang::Token& token)
    {
        return token.str;
    }

    inline std::string binary(const fk::lang::expression_ptr& left, const std::string& op, const fk::lang::expression_ptr& right)
    {
        return stringify(left) + " " + op + " " + stringify(right);
    }
};

}
