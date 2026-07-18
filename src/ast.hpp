#pragma once
#include "value.hpp"
#include "token.hpp"
#include "function.hpp"
#include <unordered_map>
#include <memory>
#include <string>

class Interpreter;
enum class CastType { Int, Double };

enum class CompareType {
    EQ, NE, GT, LT, GE, LE
};

// AST节点基类
class Expr {
public:
    virtual ~Expr() = default;
    virtual Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) = 0;
};

// 变量节点
class IdentifierExpr : public Expr {
private:
    std::string name;
public:
    explicit IdentifierExpr(const std::string& n);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 整数节点
class NumberExpr : public Expr {
private:
    int value;
public:
    explicit NumberExpr(int v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 浮点数节点
class DoubleExpr : public Expr {
private:
    double value;
public:
    explicit DoubleExpr(double v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 字符串节点
class StringExpr : public Expr {
private:
    std::string value;
public:
    explicit StringExpr(const std::string& v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 函数调用节点
class CallExpr : public Expr {
private:
    std::string funcName;
    std::vector<std::unique_ptr<Expr>> args; 
public:
    explicit CallExpr(const std::string& name);
    CallExpr(const std::string& name, std::vector<std::unique_ptr<Expr>>&& arguments);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 整数浮点数（二元运算节点）
class BinaryExpr : public Expr {
private:
    std::unique_ptr<Expr> left;
    TokenT op;
    std::unique_ptr<Expr> right;
public:
    BinaryExpr(std::unique_ptr<Expr> l, TokenT o, std::unique_ptr<Expr> r);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class InputExpr : public Expr {
public:
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 类型转换表达式节点
class CastExpr : public Expr {
private:
    CastType castType;
    std::unique_ptr<Expr> expr; 
public:
    CastExpr(CastType type, std::unique_ptr<Expr> e) : castType(type), expr(std::move(e)) {}
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class CompareExpr : public Expr {
private:
    std::unique_ptr<Expr> left;
    CompareType op;
    std::unique_ptr<Expr> right;
public:
    CompareExpr(std::unique_ptr<Expr> l, CompareType o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {
    }
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

// 条件组合节点
class ConditionExpr : public Expr {
private:
    std::unique_ptr<Expr> left;
    TokenT op; 
    std::unique_ptr<Expr> right;
public:
    ConditionExpr(std::unique_ptr<Expr> l, TokenT o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {
    }
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

struct IfStatement {
    std::string condition;
    std::vector<std::string> body; 
};

struct WhileStatement {
    std::string condition;
    std::vector<std::string> body;
};