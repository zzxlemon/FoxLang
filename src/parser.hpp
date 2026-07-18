#pragma once
#include "lexer.hpp"
#include "token.hpp"
#include "value.hpp"
#include "function.hpp"
#include "ast.hpp"
#include <unordered_map>
#include <vector>
#include <string>

class Parser {
private:
    Lexer funcLexer;
    Token funcCurrentToken;
    std::vector<Function> tempFunctions;
    // 执行语句专用的工具方法
    static void skipWhitespace(Lexer& lexer, Token& currentToken);
    static void eat(Lexer& lexer, Token& currentToken, TokenT expectedType);
    // 解析表达式
    static std::unique_ptr<Expr> parsePrimary(Lexer& lexer, Token& currentToken);
    static std::unique_ptr<Expr> parseExpr(Lexer& lexer, Token& currentToken);
    static std::unique_ptr<Expr> parseAdd(Lexer& lexer, Token& currentToken); // 新增：处理 +/-
    // 解析赋值语句
    static void parseAssignment(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    // 解析print语句
    static void parsePrint(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
	// 解析end语句
    static void parseEnd(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
		std::unordered_map<std::string, Function>& functions);
	// 解析exit语句
    static void parseExit(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
		std::unordered_map<std::string, Function>& functions);
    // 解析ret返回语句
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
    // 解析条件表达式
    static std::unique_ptr<Expr> parseCondition(Lexer& lexer, Token& currentToken);
    // 解析if语句
    static IfStatement parseIfStatement(Lexer& lexer, Token& currentToken);
	// 解析while语句
    static WhileStatement parseWhileStatement(Lexer& lexer, Token& currentToken);
    // 解析import语句
    static void parseImportStatement(Lexer& lexer, Token& currentToken,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
public:
    std::unordered_map<std::string, Value>& variables;
    std::unordered_map<std::string, Function>& functions;
    std::unordered_map<std::string, bool> importedLibraries;

    Parser(const std::string& src, std::unordered_map<std::string, Value>& vars,
        std::unordered_map<std::string, Function>& funcs);
    void parseAllFunctions();
    static Value parseLine(const std::string& line,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    static void executeIfStatement(const IfStatement& ifStmt,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
    static void executeWhileStatement(const WhileStatement& whileStmt,
        std::unordered_map<std::string, Value>& variables,
        std::unordered_map<std::string, Function>& functions);
};