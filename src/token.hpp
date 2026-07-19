#pragma once
#include <string>

// Token类型枚举
enum TokenT {
    TOKEN_IDENTIFIER,  // 变量名或函数名
    TOKEN_NUMBER,      // 整数
    TOKEN_DOUBLE_NUM,  // 浮点数
    TOKEN_STRING,      // 字符串
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_EQUAL,       // =
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_PRINT,       // print
    TOKEN_EOF,         // 结束
    TOKEN_FUNC,        // func
    TOKEN_VOID,        // void
    TOKEN_INT,         // int
    TOKEN_STRING_TYPE, // string
    TOKEN_DOUBLE,      // double
    TOKEN_ARROW,       // ->
    TOKEN_LEFT_ARROW,  // <-
    TOKEN_LBRACE,      // {
    TOKEN_RBRACE,      // }
    TOKEN_COLON,       // 冒号
    TOKEN_RET,         // ret   
    TOKEN_NEWLINE,     // 换行
    TOKEN_INPUT,       // input关键字
    TOKEN_INT_CAST,    // int类型转换int()
    TOKEN_DOUBLE_CAST, // double类型转换double()
    TOKEN_IF,          // if
    TOKEN_OR,          // or
    TOKEN_AND,         // and
    TOKEN_EQ,          // ==
    TOKEN_NE,          // !=
    TOKEN_GT,          // >  
    TOKEN_LT,          // <
    TOKEN_GE,          // >= 
    TOKEN_LE,          // <=
    TOKEN_COMMA,       // ,
    TOKEN_WHILE,       // while
    TOKEN_END,         // end
    TOKEN_EXIT,        // exit 
    TOKEN_IMPORT,      // import
    TOKEN_FOR,         // for
    TOKEN_LBRACKET,    // [
    TOKEN_RBRACKET,    // ]
    TOKEN_SEMICOLON    // ;
};

// Token结构体
struct Token {
    TokenT type;
    std::string value;
    int line;
    int col;
    Token(TokenT t, const std::string& v, int l = 1, int c = 1)
        : type(t), value(v), line(l), col(c) {}
    std::string position() const {
        return std::to_string(line) + ":" + std::to_string(col);
    }
};