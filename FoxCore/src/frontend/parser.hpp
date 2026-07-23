#pragma once
#include "lexer.hpp"
#include "token.hpp"
#include "value.hpp"
#include "function.hpp"
#include "ast.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

struct GotoException : std::exception {
    std::string label;
    explicit GotoException(const std::string& l) : label(l) {}
    const char* what() const noexcept override { return label.c_str(); }
};

struct StmtHandler {
    virtual ~StmtHandler() = default;
    virtual void onPrint(std::unique_ptr<Expr> arg) = 0;
    virtual void onPrintln(std::unique_ptr<Expr> arg) = 0;
    virtual void onExit(std::unique_ptr<Expr> arg) = 0;
    virtual Value onRet(std::unique_ptr<Expr> arg) = 0;
    virtual void onEndl() = 0;
    virtual void onInput(const std::string& varName) = 0;
    virtual void onCall(const std::string& name, std::vector<std::unique_ptr<Expr>> args) = 0;
    virtual void onAssign(const std::string& name, std::unique_ptr<Expr> expr) = 0;
    virtual void onIndexAssign(const std::string& name, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value) = 0;
    virtual void onIf(IfStatement ifStmt) = 0;
    virtual void onWhile(WhileStatement whileStmt) = 0;
    virtual void onFor(ForStatement forStmt) = 0;
    virtual void onFnLabel(const std::string& name) = 0;
    virtual void onGoto(const std::string& name) = 0;
};

class Parser {
private:
    Lexer funcLexer;
    Token funcCurrentToken;
    std::vector<Function> tempFunctions;
    
    // 执行/解析专用的工具方法
    static void skipWhitespace(Lexer& lexer, Token& currentToken);
    static void eat(Lexer& lexer, Token& currentToken, TokenT expectedType);
    
    // 解析基础表达式
    static std::unique_ptr<Expr> parsePrimary(Lexer& lexer, Token& currentToken);
    static std::unique_ptr<Expr> parsePostfix(Lexer& lexer, Token& currentToken, std::unique_ptr<Expr> expr);
    static std::unique_ptr<Expr> parseAdd(Lexer& lexer, Token& currentToken); // 解析加减法 +/-
    
    // 解析赋值语句
    static void parseAssignment(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
        
    // 解析 print 语句
    static void parsePrint(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
        
    // 解析 endl 语句
    static void parseEndl(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
        
    // 解析 exit 语句
    static void parseExit(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
        
    // 解析 ret 返回语句
    static Value parseRet(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
        
    static void parseInputStatement(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);

    static std::unique_ptr<Expr> parseCastExpr(Lexer& lexer, Token& currentToken, CastType castType);
    
    // 解析单条语句
    static std::string parseSingleStatement(Lexer& lexer, Token& currentToken);
    
    // 解析函数定义
    void parseFunction();
    
    // 解析比较表达式
    static std::unique_ptr<Expr> parseCompare(Lexer& lexer, Token& currentToken);
    
    // 解析逻辑条件表达式
    static std::unique_ptr<Expr> parseCondition(Lexer& lexer, Token& currentToken);
    
    // 解析 if 语句
    static IfStatement parseIfStatement(Lexer& lexer, Token& currentToken);
    
    // 解析 while 语句
    static WhileStatement parseWhileStatement(Lexer& lexer, Token& currentToken);
    
    static ForStatement parseForStatement(Lexer& lexer, Token& currentToken);
    
    // 解析 import 语句
    static void parseImportStatement(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);

public:
    std::unordered_map<std::string, Value>& variables;
    std::unordered_map<std::string, Function>& functions;
    std::unordered_map<std::string, bool> importedLibraries;

    static std::unique_ptr<Expr> parseExpr(Lexer& lexer, Token& currentToken);

    // 基于 Handler 的单行解析（仅解析，不执行）
    static void parseLine(const std::string& line, StmtHandler& handler);

    static Value executeIfStatement(const IfStatement& ifStmt,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    static Value executeWhileStatement(const WhileStatement& whileStmt,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    static Value executeForStatement(const ForStatement& forStmt,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);

    Parser(const std::string& src, std::unordered_map<std::string, Value>& vars,
        std::unordered_map<std::string, Function>& funcs);
    void parseAllFunctions();
    static Value parseLine(const std::string& line,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    static void resetNewAllocBytes();
    static bool checkNewAllocBytes(int size);
};
