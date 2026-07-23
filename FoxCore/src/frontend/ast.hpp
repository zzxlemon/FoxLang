#pragma once
#include "value.hpp"
#include "token.hpp"
#include "function.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

class Interpreter;
enum class CastType { Int, Double };

enum class CompareType {
    EQ, NE, GT, LT, GE, LE
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) = 0;
};

class IdentifierExpr : public Expr {
public:
    std::string name;
    explicit IdentifierExpr(const std::string& n);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class NumberExpr : public Expr {
public:
    int value;
    explicit NumberExpr(int v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class DoubleExpr : public Expr {
public:
    double value;
    explicit DoubleExpr(double v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class StringExpr : public Expr {
public:
    std::string value;
    explicit StringExpr(const std::string& v);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class ArrayExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    explicit ArrayExpr(std::vector<std::unique_ptr<Expr>>&& elems);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> arrayExpr;
    std::unique_ptr<Expr> indexExpr;
    IndexExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class CallExpr : public Expr {
public:
    std::string funcName;
    std::vector<std::unique_ptr<Expr>> args; 
    explicit CallExpr(const std::string& name);
    CallExpr(const std::string& name, std::vector<std::unique_ptr<Expr>>&& arguments);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    TokenT op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, TokenT o, std::unique_ptr<Expr> r);
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class InputExpr : public Expr {
public:
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class NewExpr : public Expr {
public:
    std::unique_ptr<Expr> sizeExpr;
    explicit NewExpr(std::unique_ptr<Expr> size) : sizeExpr(std::move(size)) {}
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class CastExpr : public Expr {
public:
    CastType castType;
    std::unique_ptr<Expr> expr; 
    CastExpr(CastType type, std::unique_ptr<Expr> e) : castType(type), expr(std::move(e)) {}
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class CompareExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    CompareType op;
    std::unique_ptr<Expr> right;
    CompareExpr(std::unique_ptr<Expr> l, CompareType o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {
    }
    Value evaluate(std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions) override;
};

class ConditionExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    TokenT op; 
    std::unique_ptr<Expr> right;
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

struct ForStatement {
    std::string init;
    std::string condition;
    std::string iter;
    std::vector<std::string> body;
};