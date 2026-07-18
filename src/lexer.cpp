#include "lexer.hpp"

Lexer::Lexer(const std::string& src) : source(src), pos(0) {}

void Lexer::skipWhitespaceExceptNewline() {
    while (pos < source.size()) {
        char c = source[pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
        }
        else if (c == '\n') {
            break;
        }
        else {
            break;
        }
    }
}

std::string Lexer::readIdentifier() {
    size_t start = pos;
    while (pos < source.size() && (isalpha(static_cast<unsigned char>(source[pos])) || source[pos] == '_' || isdigit(static_cast<unsigned char>(source[pos])))) {
        pos++;
    }
    std::string ident = source.substr(start, pos - start);
    // 关键字判断
    if (ident == "func") return "func";
    if (ident == "void") return "void";
    if (ident == "int") return "int";
    if (ident == "string") return "string"; 
    if (ident == "double") return "double";
    if (ident == "print") return "print";
    if (ident == "ret") return "ret";
    if (ident == "input") return "input";
    if (ident == "if") return "if";
    if (ident == "or") return "or";
    if (ident == "and") return "and";
	if (ident == "while") return "while";
	if (ident == "end") return "end";
    if (ident == "exit") return "exit";
	if (ident == "import") return "import";
    return ident;
}

std::string Lexer::readNumber() {
    size_t start = pos;
    bool hasDot = false;
    while (pos < source.size()) {
        if (isdigit(static_cast<unsigned char>(source[pos]))) {
            pos++;
        }
        else if (source[pos] == '.' && !hasDot) {
            hasDot = true;
            pos++;
        }
        else {
            break;
        }
    }
    return source.substr(start, pos - start);
}

std::string Lexer::readString() {
    pos++;
    size_t start = pos;
    while (pos < source.size() && source[pos] != '"') {
        pos++;
    }
    if (pos >= source.size()) {
        throw std::runtime_error("语法错误：字符串未结束（缺少闭合双引号）");
    }
    std::string str = source.substr(start, pos - start);
    pos++;
    return str;
}

bool Lexer::readArrow() {
    if (pos + 1 < source.size() && source[pos] == '-' && source[pos + 1] == '>') {
        pos += 2;
        return true;
    }
    return false;
}

bool Lexer::readLeftArrow() {
    if (pos + 1 < source.size() && source[pos] == '<' && source[pos + 1] == '-') {
        pos += 2;
        return true;
    }
    return false;
}

Token Lexer::nextToken() {
    skipWhitespaceExceptNewline();
    if (pos >= source.size()) {
        return Token(TOKEN_EOF, "");
    }

    char c = source[pos];

    if (c == '\n') {
        pos++;
        return Token(TOKEN_NEWLINE, "\n");
    }

    if (c == '-') {
        if (readArrow()) {
            return Token(TOKEN_ARROW, "->");
        }
        else {
            pos++;
            return Token(TOKEN_MINUS, "-");
        }
    }
    else if (c == ':') {
        pos++;
        return Token(TOKEN_COLON, ":");
    }
    else if (c == '{') {
        pos++;
        return Token(TOKEN_LBRACE, "{");
    }
    else if (c == '}') {
        pos++;
        return Token(TOKEN_RBRACE, "}");
    }

    if (c == '=') {
        if (pos + 1 < source.size() && source[pos + 1] == '=') {
            pos += 2;
            return Token(TOKEN_EQ, "==");
        }
        else {
            pos++;
            return Token(TOKEN_EQUAL, "=");
        }
    }
    else if (c == '!') {
        if (pos + 1 < source.size() && source[pos + 1] == '=') {
            pos += 2;
            return Token(TOKEN_NE, "!=");
        }
        else {
            throw std::runtime_error("无效字符: !（仅支持!=）");
        }
    }
    else if (c == '>') {
        if (pos + 1 < source.size() && source[pos + 1] == '=') {
            pos += 2;
            return Token(TOKEN_GE, ">=");
        }
        else {
            pos++;
            return Token(TOKEN_GT, ">");
        }
    }
    else if (c == '<') {
        if (readLeftArrow()) {
            return Token(TOKEN_LEFT_ARROW, "<-");
        }
        else if (pos + 1 < source.size() && source[pos + 1] == '=') {
            pos += 2;
            return Token(TOKEN_LE, "<=");
        }
        else {
            pos++;
            return Token(TOKEN_LT, "<");
        }
    }

    // 基础符号
    switch (c) {
    case '+': pos++; return Token(TOKEN_PLUS, "+");
    case '(': pos++; return Token(TOKEN_LPAREN, "(");
    case ')': pos++; return Token(TOKEN_RPAREN, ")");
    case '"': return Token(TOKEN_STRING, readString());
    case ',': pos++; return Token(TOKEN_COMMA, ",");
    default:
        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string ident = readIdentifier();
            if (ident == "func") return Token(TOKEN_FUNC, "func");
            if (ident == "void") return Token(TOKEN_VOID, "void");
            if (ident == "int") {
                if (pos < source.size() && source[pos] == '(') {
                    pos++;
                    return Token(TOKEN_INT_CAST, "int");
                }
                return Token(TOKEN_INT, "int");
            }
            if (ident == "string") return Token(TOKEN_STRING_TYPE, "string");
            if (ident == "double") {
                if (pos < source.size() && source[pos] == '(') {
                    pos++;
                    return Token(TOKEN_DOUBLE_CAST, "double");
                }
                return Token(TOKEN_DOUBLE, "double");
            }
            if (ident == "print") return Token(TOKEN_PRINT, "print");
            if (ident == "ret") return Token(TOKEN_RET, "ret");
            if (ident == "input") return Token(TOKEN_INPUT, "input");
            if (ident == "if") return Token(TOKEN_IF, "if");
            if (ident == "or") return Token(TOKEN_OR, "or");
            if (ident == "and") return Token(TOKEN_AND, "and");
			if (ident == "while") return Token(TOKEN_WHILE, "while");
            if (ident == "end") return Token(TOKEN_END, "end");
            if (ident == "exit") return Token(TOKEN_EXIT, "exit");
            if (ident == "import") return Token(TOKEN_IMPORT, "import");
            return Token(TOKEN_IDENTIFIER, ident);
        }
        else if (isdigit(static_cast<unsigned char>(c))) {
            std::string num = readNumber();
            if (num.find('.') != std::string::npos) {
                return Token(TOKEN_DOUBLE_NUM, num);
            }
            else {
                return Token(TOKEN_NUMBER, num);
            }
        }
        else {
            throw std::runtime_error("无效字符: " + std::string(1, c));
        }
    }
}

void Lexer::ungetToken() {
    if (pos > 0) pos--;
}
