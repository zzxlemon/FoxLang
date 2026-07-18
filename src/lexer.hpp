#pragma once
#include "token.hpp"
#include <string>
#include <stdexcept>
#include <cctype>

class Lexer {
private:
    std::string source;
    size_t pos = 0;

    void skipWhitespaceExceptNewline();
    std::string readIdentifier();
    std::string readNumber();
    std::string readString();
    bool readArrow();
	bool readLeftArrow();

public:
    explicit Lexer(const std::string& src);
    Token nextToken();
    void ungetToken();
};