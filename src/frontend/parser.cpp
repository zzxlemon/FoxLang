#include "parser.hpp"
#include "common.hpp"
#include "library_manager.hpp" 
#include <iostream>
#include <algorithm>

static std::string makeParseError(const Token& token, const std::string& message) {
    return "Syntax error: " + token.position() + ": " + message;
}

void Parser::skipWhitespace(Lexer& lexer, Token& currentToken) {
    // 仅跳过空格/制表符/回车，不跳过换行/EOF/有效Token
    while (currentToken.type != TOKEN_EOF && !currentToken.value.empty()
        && isspace(static_cast<unsigned char>(currentToken.value[0]))
        && currentToken.value[0] != '\n') {
        currentToken = lexer.nextToken();
    }
}

void Parser::eat(Lexer& lexer, Token& currentToken, TokenT expectedType) {
    if (currentToken.type == expectedType) {
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken); // 跳过空格
    }
    else {
        throw std::runtime_error(makeParseError(currentToken,
            "Expected token type " + std::to_string(expectedType) + ", got " + std::to_string(currentToken.type) + " (value: " + currentToken.value + ")"));
    }
}

std::unique_ptr<Expr> Parser::parsePostfix(Lexer& lexer, Token& currentToken, std::unique_ptr<Expr> expr) {
    while (currentToken.type == TOKEN_LBRACKET) {
        eat(lexer, currentToken, TOKEN_LBRACKET);
        auto indexExpr = parseExpr(lexer, currentToken);
        eat(lexer, currentToken, TOKEN_RBRACKET);
        expr = std::unique_ptr<IndexExpr>(new IndexExpr(std::move(expr), std::move(indexExpr)));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimary(Lexer& lexer, Token& currentToken) {
    skipWhitespace(lexer, currentToken);
    Token token = currentToken;

    if (token.type == TOKEN_INT_CAST) {
        eat(lexer, currentToken, TOKEN_INT_CAST);
        return parseCastExpr(lexer, currentToken, CastType::Int);
    }
    else if (token.type == TOKEN_DOUBLE_CAST) {
        eat(lexer, currentToken, TOKEN_DOUBLE_CAST);
        return parseCastExpr(lexer, currentToken, CastType::Double);
    }

    if (token.type == TOKEN_IDENTIFIER) {
        eat(lexer, currentToken, TOKEN_IDENTIFIER);
        std::unique_ptr<Expr> baseExpr;
        if (currentToken.type == TOKEN_LPAREN) {
            eat(lexer, currentToken, TOKEN_LPAREN);
            std::vector<std::unique_ptr<Expr>> args;
            while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
                args.push_back(parseExpr(lexer, currentToken));
                if (currentToken.type == TOKEN_COMMA) {
                    eat(lexer, currentToken, TOKEN_COMMA);
                    skipWhitespace(lexer, currentToken);
                }
            }
            eat(lexer, currentToken, TOKEN_RPAREN);
            baseExpr = std::unique_ptr<CallExpr>(new CallExpr(token.value, std::move(args)));
        }
        else {
            baseExpr = std::unique_ptr<IdentifierExpr>(new IdentifierExpr(token.value));
        }
        return parsePostfix(lexer, currentToken, std::move(baseExpr));
    }

    else if (token.type == TOKEN_NUMBER) {
        eat(lexer, currentToken, TOKEN_NUMBER);
        return std::unique_ptr<NumberExpr>(new NumberExpr(std::stoi(token.value)));
    }
    else if (token.type == TOKEN_DOUBLE_NUM) {
        eat(lexer, currentToken, TOKEN_DOUBLE_NUM);
        return std::unique_ptr<DoubleExpr>(new DoubleExpr(std::stod(token.value)));
    }
    else if (token.type == TOKEN_STRING) {
        eat(lexer, currentToken, TOKEN_STRING);
        return std::unique_ptr<StringExpr>(new StringExpr(token.value));
    }
    else if (token.type == TOKEN_LBRACKET) {
        eat(lexer, currentToken, TOKEN_LBRACKET);
        std::vector<std::unique_ptr<Expr>> elements;
        while (currentToken.type != TOKEN_RBRACKET && currentToken.type != TOKEN_EOF) {
            elements.push_back(parseExpr(lexer, currentToken));
            if (currentToken.type == TOKEN_COMMA) {
                eat(lexer, currentToken, TOKEN_COMMA);
                skipWhitespace(lexer, currentToken);
            }
        }
        eat(lexer, currentToken, TOKEN_RBRACKET);
        auto arrayExpr = std::unique_ptr<ArrayExpr>(new ArrayExpr(std::move(elements)));
        return parsePostfix(lexer, currentToken, std::move(arrayExpr));
    }
    else if (token.type == TOKEN_LPAREN) {
        eat(lexer, currentToken, TOKEN_LPAREN);
        auto expr = parseExpr(lexer, currentToken);
        eat(lexer, currentToken, TOKEN_RPAREN);
        return parsePostfix(lexer, currentToken, std::move(expr));
    }
    else {
        throw std::runtime_error(makeParseError(token, "Invalid expression start: " + token.value));
    }
}

// 处理加减优先级（低优先级于乘除）
std::unique_ptr<Expr> Parser::parseAdd(Lexer& lexer, Token& currentToken) {
    auto left = parsePrimary(lexer, currentToken);
    while (currentToken.type == TOKEN_PLUS || currentToken.type == TOKEN_MINUS) {
        TokenT op = currentToken.type;
        eat(lexer, currentToken, op);
        auto right = parsePrimary(lexer, currentToken);
        left = std::unique_ptr<BinaryExpr>(new BinaryExpr(std::move(left), op, std::move(right)));
    }
    return left;
}

std::unique_ptr<Expr> Parser::parseExpr(Lexer& lexer, Token& currentToken) {
    auto left = parseCompare(lexer, currentToken);
    while (currentToken.type == TOKEN_AND || currentToken.type == TOKEN_OR) {
        Token opToken = currentToken;
        eat(lexer, currentToken, opToken.type);
        auto right = parseCompare(lexer, currentToken);
        left = std::unique_ptr<ConditionExpr>(new ConditionExpr(std::move(left), opToken.type, std::move(right)));
    }
    return left;
}

void Parser::parseAssignment(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    std::string varName = currentToken.value;
    eat(lexer, currentToken, TOKEN_IDENTIFIER);
    eat(lexer, currentToken, TOKEN_EQUAL);
    auto expr = parseExpr(lexer, currentToken);
    variables[varName] = expr->evaluate(variables, functions);

    if (isOutInfo) {
        Value& assigned = variables[varName];
        std::cout << "[执行] 赋值变量：" << varName << " = ";
        switch (assigned.getType()) {
        case Value::Type::Int: std::cout << assigned.asInt(); break;
        case Value::Type::Double: std::cout << assigned.asDouble(); break;
        case Value::Type::String: std::cout << assigned.asString(); break;
        case Value::Type::Void: std::cout << "(void)"; break;
        case Value::Type::Array: std::cout << "[array]"; break;
        }
        std::cout << std::endl;
    }
}

void Parser::parsePrint(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_PRINT);
    eat(lexer, currentToken, TOKEN_LPAREN); 
    auto expr = parseExpr(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_RPAREN);

    Value result = expr->evaluate(variables, functions);
    switch (result.getType()) {
    case Value::Type::Int: std::cout << result.asInt(); break;
    case Value::Type::Double: std::cout << result.asDouble(); break;
    case Value::Type::String: std::cout << result.asString(); break;
    case Value::Type::Void: std::cout << "(void)"; break;
    }
    // std::cout << std::endl;
}

void Parser::parseEnd(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
	eat(lexer, currentToken, TOKEN_END);
    std::cout << std::endl;
}

void Parser::parseExit(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_EXIT);
	eat(lexer, currentToken, TOKEN_LPAREN);
	auto expr = parseExpr(lexer, currentToken);
	eat(lexer, currentToken, TOKEN_RPAREN);
	Value exitCodeVal = expr->evaluate(variables, functions);
    if (exitCodeVal.asInt() == 0)
        exit(0);
    exit(exitCodeVal.asInt()); 
}

Value Parser::parseRet(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_RET);
    auto expr = parseExpr(lexer, currentToken);
    Value retVal = expr->evaluate(variables, functions);

    if (isOutInfo) {
        std::cout << "[执行] 返回值：" << retVal.asString() << std::endl;
    }
    return retVal;
}
    
// 解析单条语句
std::string Parser::parseSingleStatement(Lexer& lexer, Token& currentToken) {
    std::string stmt;
    skipWhitespace(lexer, currentToken);

    // 支持 if/while 块整体读取（跨行，直到遇到 '}' 或 EOF）
    if (currentToken.type == TOKEN_IF || currentToken.type == TOKEN_WHILE || currentToken.type == TOKEN_FOR) {
        int braceDepth = 0;
        bool insideBlock = false;
        while (currentToken.type != TOKEN_EOF) {
            if (currentToken.type == TOKEN_STRING) {
                stmt += "\"" + currentToken.value + "\"";
            }
            else {
                stmt += currentToken.value;
            }
            stmt += " ";
            if (currentToken.type == TOKEN_LBRACE) {
                insideBlock = true;
                braceDepth++;
            }
            else if (currentToken.type == TOKEN_RBRACE && insideBlock) {
                braceDepth--;
                if (braceDepth == 0) {
                    currentToken = lexer.nextToken();
                    skipWhitespace(lexer, currentToken);
                    break;
                }
            }
            currentToken = lexer.nextToken();
            skipWhitespace(lexer, currentToken);
        }
    }
    else {
        while (currentToken.type != TOKEN_EOF && currentToken.type != TOKEN_NEWLINE && currentToken.type != TOKEN_RBRACE) {
            if (currentToken.type == TOKEN_STRING) {
                stmt += "\"" + currentToken.value + "\"";
            }
            else {
                stmt += currentToken.value;
            }
            stmt += " ";
            currentToken = lexer.nextToken();
            skipWhitespace(lexer, currentToken);
        }

        if (currentToken.type == TOKEN_NEWLINE) {
            currentToken = lexer.nextToken();
        }
    }

    size_t start = stmt.find_first_not_of(" \t\n\r");
    size_t end = stmt.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        stmt = stmt.substr(start, end - start + 1);
    }
    else {
        stmt.clear();
    }
    return stmt;
}

// 解析函数定义
void Parser::parseFunction() {
    eat(funcLexer, funcCurrentToken, TOKEN_FUNC);
    std::string funcName = funcCurrentToken.value;
    eat(funcLexer, funcCurrentToken, TOKEN_IDENTIFIER);
    eat(funcLexer, funcCurrentToken, TOKEN_LPAREN);

    // 解析参数列表
    std::vector<Parameter> params;
    while (funcCurrentToken.type != TOKEN_RPAREN && funcCurrentToken.type != TOKEN_EOF) {
        if (funcCurrentToken.type == TOKEN_NEWLINE || funcCurrentToken.value.empty()) {
            funcCurrentToken = funcLexer.nextToken();
            skipWhitespace(funcLexer, funcCurrentToken);
            continue;
        }

        // 参数名
        std::string paramName = funcCurrentToken.value;
        eat(funcLexer, funcCurrentToken, TOKEN_IDENTIFIER);

        // 期望 <-
        if (funcCurrentToken.type != TOKEN_LEFT_ARROW) {
            throw std::runtime_error(makeParseError(funcCurrentToken, "Parameter definition expected '<-', got: " + funcCurrentToken.value));
        }
        eat(funcLexer, funcCurrentToken, TOKEN_LEFT_ARROW);

        // 参数类型
        std::string paramType;
        if (funcCurrentToken.type == TOKEN_INT) {
            paramType = "int";
            eat(funcLexer, funcCurrentToken, TOKEN_INT);
        }
        else if (funcCurrentToken.type == TOKEN_DOUBLE) {
            paramType = "double";
            eat(funcLexer, funcCurrentToken, TOKEN_DOUBLE);
        }
        else if (funcCurrentToken.type == TOKEN_STRING_TYPE) {
            paramType = "string";
            eat(funcLexer, funcCurrentToken, TOKEN_STRING_TYPE);
        }
        else {
            throw std::runtime_error(makeParseError(funcCurrentToken, "Unsupported parameter type: " + funcCurrentToken.value));
        }

        params.push_back({ paramName, paramType });

        // 处理逗号分隔
        if (funcCurrentToken.type == TOKEN_COMMA) {
            eat(funcLexer, funcCurrentToken, TOKEN_COMMA);
            skipWhitespace(funcLexer, funcCurrentToken);
        }
    }

    eat(funcLexer, funcCurrentToken, TOKEN_RPAREN);
    eat(funcLexer, funcCurrentToken, TOKEN_ARROW);
    // 返回类型
    std::string returnType;
    if (funcCurrentToken.type == TOKEN_VOID) {
        returnType = "void";
        eat(funcLexer, funcCurrentToken, TOKEN_VOID);
    }
    else if (funcCurrentToken.type == TOKEN_INT) {
        returnType = "int";
        eat(funcLexer, funcCurrentToken, TOKEN_INT);
    }
    else if (funcCurrentToken.type == TOKEN_STRING_TYPE) {
        returnType = "string";
        eat(funcLexer, funcCurrentToken, TOKEN_STRING_TYPE);
    }
    else if (funcCurrentToken.type == TOKEN_DOUBLE) {
        returnType = "double";
        eat(funcLexer, funcCurrentToken, TOKEN_DOUBLE);
    }
    else {
        throw std::runtime_error(makeParseError(funcCurrentToken, "Unsupported return type: " + funcCurrentToken.value));
    }

    if (funcCurrentToken.type == TOKEN_COLON) {
        eat(funcLexer, funcCurrentToken, TOKEN_COLON);
    }
    eat(funcLexer, funcCurrentToken, TOKEN_LBRACE);

    Function func;
    func.name = funcName;
    func.returnType = returnType;
	func.parameters = params;

    funcCurrentToken = funcLexer.nextToken();
    skipWhitespace(funcLexer, funcCurrentToken);

    while (funcCurrentToken.type != TOKEN_RBRACE && funcCurrentToken.type != TOKEN_EOF) {
        std::string stmt = parseSingleStatement(funcLexer, funcCurrentToken);
        if (!stmt.empty()) {
            func.body.push_back(stmt);
            if (isOutInfo) {
                std::cout << "[解析] 函数体语句：" << stmt << std::endl;
            }
        }
        skipWhitespace(funcLexer, funcCurrentToken);
    }

    eat(funcLexer, funcCurrentToken, TOKEN_RBRACE);
    tempFunctions.push_back(func);
    if (isOutInfo) {
        std::cout << "[解析] 完成函数：" << func.name << "，共" << func.body.size() << "条语句" << std::endl;
    }
}

Parser::Parser(const std::string& src, std::unordered_map<std::string, Value>& vars,
    std::unordered_map<std::string, Function>& funcs)
    : funcLexer(src), variables(vars), functions(funcs), funcCurrentToken(funcLexer.nextToken()) {
    skipWhitespace(funcLexer, funcCurrentToken);
}

void Parser::parseAllFunctions() {
    tempFunctions.clear();
    while (funcCurrentToken.type != TOKEN_EOF) {
        skipWhitespace(funcLexer, funcCurrentToken);
        if (funcCurrentToken.type == TOKEN_FUNC) {
            parseFunction();
        }
        else if (funcCurrentToken.type == TOKEN_IMPORT) {
            parseImportStatement(funcLexer, funcCurrentToken, variables, functions);
        }
        else {
            funcCurrentToken = funcLexer.nextToken();
        }
    }
    for (const auto& func : tempFunctions) {
        functions[func.name] = func;
    }
}

Value Parser::parseLine(const std::string& line,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Lexer lineLexer(line);
    Token currentToken = lineLexer.nextToken();
    skipWhitespace(lineLexer, currentToken);

    if (currentToken.type == TOKEN_IMPORT) {
        parseImportStatement(lineLexer, currentToken, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_IF) {
        IfStatement ifStmt = parseIfStatement(lineLexer, currentToken);
        executeIfStatement(ifStmt, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_FOR) {
        ForStatement forStmt = parseForStatement(lineLexer, currentToken);
        executeForStatement(forStmt, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_WHILE) {
        WhileStatement whileStmt = parseWhileStatement(lineLexer, currentToken);
        executeWhileStatement(whileStmt, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_INPUT) {
        parseInputStatement(lineLexer, currentToken, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_PRINT) {
        parsePrint(lineLexer, currentToken, variables, functions);
        return Value();
    }
    if (currentToken.type == TOKEN_END) {
		parseEnd(lineLexer, currentToken, variables, functions);
        return Value();
    }
    
    if (currentToken.type == TOKEN_EXIT) {
		parseExit(lineLexer, currentToken, variables, functions);
        return Value();
    }

    else if (currentToken.type == TOKEN_RET) {
		return parseRet(lineLexer, currentToken, variables, functions);
    }

    else if (currentToken.type == TOKEN_IDENTIFIER) {
        // 区分函数调用和赋值
        std::string identName = currentToken.value;
        Token nextToken = lineLexer.nextToken();
        skipWhitespace(lineLexer, nextToken);

        if (nextToken.type == TOKEN_LPAREN) {
            // 情况1: func() 直接调用
            // 情况2: var = func() 赋值

            currentToken = nextToken;
            eat(lineLexer, currentToken, TOKEN_LPAREN);

            std::vector<std::unique_ptr<Expr>> args;
            while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
                args.push_back(parseExpr(lineLexer, currentToken));

                if (currentToken.type == TOKEN_COMMA) {
                    eat(lineLexer, currentToken, TOKEN_COMMA);
                    skipWhitespace(lineLexer, currentToken);
                }
            }

            eat(lineLexer, currentToken, TOKEN_RPAREN);
            CallExpr callExpr(identName, std::move(args));
            Value callResult = callExpr.evaluate(variables, functions);

            // 检查后面是否还有赋值
            skipWhitespace(lineLexer, currentToken);
            if (currentToken.type == TOKEN_NEWLINE || currentToken.type == TOKEN_EOF) {
                // 纯函数调用
                return callResult;
            }
            else {
                // 这里不应该出现其他情况
                throw std::runtime_error(makeParseError(currentToken, "Unexpected token after function call: " + currentToken.value));
            }
        }
        else if (nextToken.type == TOKEN_EQUAL) {
            // 赋值：varName = ...
            currentToken = nextToken;
            eat(lineLexer, currentToken, TOKEN_EQUAL);
            auto expr = parseExpr(lineLexer, currentToken);
            Value result = expr->evaluate(variables, functions);
            variables[identName] = result;

            if (isOutInfo) {
                std::cout << "[执行] 赋值变量：" << identName << " = " << result.asString() << std::endl;
            }
            return Value();
        }
        else if (nextToken.type == TOKEN_LBRACKET) {
            // 数组元素赋值：name[index] = expr
            currentToken = nextToken;
            eat(lineLexer, currentToken, TOKEN_LBRACKET);
            auto indexExpr = parseExpr(lineLexer, currentToken);
            eat(lineLexer, currentToken, TOKEN_RBRACKET);
            skipWhitespace(lineLexer, currentToken);
            eat(lineLexer, currentToken, TOKEN_EQUAL);
            auto expr = parseExpr(lineLexer, currentToken);
            Value result = expr->evaluate(variables, functions);

            if (variables.find(identName) == variables.end()) {
                throw std::runtime_error("Undefined array variable: " + identName);
            }
            std::vector<Value>& arr = variables[identName].asArrayRef();
            int idx = indexExpr->evaluate(variables, functions).asInt();
            if (idx < 0 || idx >= static_cast<int>(arr.size())) {
                throw std::runtime_error("Array index out of bounds: " + std::to_string(idx));
            }
            arr[idx] = result;
            return Value();
        }
        else {
            throw std::runtime_error(makeParseError(nextToken, "Expected '(' or '=' after identifier, got: " + nextToken.value));
        }
    }

    return Value();
}

std::unique_ptr<Expr> Parser::parseCastExpr(Lexer& lexer, Token& currentToken, CastType castType) {
    eat(lexer, currentToken, TOKEN_LPAREN);
    auto expr = parseExpr(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_RPAREN);
    return std::unique_ptr<CastExpr>(new CastExpr(castType, std::move(expr)));
}

void Parser::parseInputStatement(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_INPUT);
    eat(lexer, currentToken, TOKEN_LPAREN);
    eat(lexer, currentToken, TOKEN_RPAREN);
    eat(lexer, currentToken, TOKEN_ARROW);
    std::string varName = currentToken.value;
    eat(lexer, currentToken, TOKEN_IDENTIFIER);

    InputExpr inputExpr;
    variables[varName] = inputExpr.evaluate(variables, functions);

    if (isOutInfo) {
        std::cout << "[执行] 输入赋值：" << varName << " = \"" << variables[varName].asString() << "\"" << std::endl;
    }
}

// 将比较操作的两侧表达式由 parsePrimary 改为 parseAdd，以支持 a + b 比较写法
std::unique_ptr<Expr> Parser::parseCompare(Lexer& lexer, Token& currentToken) {
    auto left = parseAdd(lexer, currentToken);
    while (currentToken.type == TOKEN_EQ || currentToken.type == TOKEN_NE ||
        currentToken.type == TOKEN_GT || currentToken.type == TOKEN_LT ||
        currentToken.type == TOKEN_GE || currentToken.type == TOKEN_LE) {
        Token opToken = currentToken;
        eat(lexer, currentToken, opToken.type);
        auto right = parseAdd(lexer, currentToken);

        CompareType cmpType;
        switch (opToken.type) {
        case TOKEN_EQ: cmpType = CompareType::EQ; break;
        case TOKEN_NE: cmpType = CompareType::NE; break;
        case TOKEN_GT: cmpType = CompareType::GT; break;
        case TOKEN_LT: cmpType = CompareType::LT; break;
        case TOKEN_GE: cmpType = CompareType::GE; break;
        case TOKEN_LE: cmpType = CompareType::LE; break;
        default: throw std::runtime_error("Unsupported comparison operator");
        }
        left = std::unique_ptr<CompareExpr>(new CompareExpr(std::move(left), cmpType, std::move(right)));
    }
    return left;
}

IfStatement Parser::parseIfStatement(Lexer& lexer, Token& currentToken) {
    IfStatement ifStmt;
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_IF);
    eat(lexer, currentToken, TOKEN_LPAREN);

    std::string condStr;
    while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
        condStr += currentToken.value + " ";
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken);
    }
    size_t start = condStr.find_first_not_of(" ");
    size_t end = condStr.find_last_not_of(" ");
    ifStmt.condition = (start != std::string::npos) ? condStr.substr(start, end - start + 1) : "";
    eat(lexer, currentToken, TOKEN_RPAREN);
    eat(lexer, currentToken, TOKEN_LBRACE);
    while (currentToken.type != TOKEN_RBRACE && currentToken.type != TOKEN_EOF) {
        if (currentToken.type == TOKEN_NEWLINE) {
            currentToken = lexer.nextToken();
            continue;
        }
        std::string stmt = parseSingleStatement(lexer, currentToken);
        if (!stmt.empty()) {
            ifStmt.body.push_back(stmt);
            if (isOutInfo) {
                std::cout << "[解析] if块内语句：" << stmt << std::endl;
            }
        }
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_RBRACE);
    return ifStmt;
}

void Parser::executeIfStatement(const IfStatement& ifStmt,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    if (ifStmt.condition.empty()) return;
    Lexer condLexer(ifStmt.condition);
    Token condToken = condLexer.nextToken();
    skipWhitespace(condLexer, condToken);
    auto condExpr = parseExpr(condLexer, condToken);
    Value condResult = condExpr->evaluate(variables, functions);
    bool isTrue = condResult.asBool();
    if (isOutInfo) {
        std::cout << "[执行] if条件：" << ifStmt.condition << " → " << (isTrue ? "真" : "假") << std::endl;
    }
    if (isTrue) {
        for (const auto& stmt : ifStmt.body) {
            if (isOutInfo) {
                std::cout << "[执行] if块内执行：" << stmt << std::endl;
            }
            parseLine(stmt, variables, functions);
        }
    }
}

// 解析 while 语句（结构与 if 相同）
WhileStatement Parser::parseWhileStatement(Lexer& lexer, Token& currentToken) {
    WhileStatement whileStmt;
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_WHILE);
    eat(lexer, currentToken, TOKEN_LPAREN);

    std::string condStr;
    while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
        condStr += currentToken.value + " ";
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken);
    }
    size_t start = condStr.find_first_not_of(" ");
    size_t end = condStr.find_last_not_of(" ");
    whileStmt.condition = (start != std::string::npos) ? condStr.substr(start, end - start + 1) : "";
    eat(lexer, currentToken, TOKEN_RPAREN);
    eat(lexer, currentToken, TOKEN_LBRACE);
    while (currentToken.type != TOKEN_RBRACE && currentToken.type != TOKEN_EOF) {
        if (currentToken.type == TOKEN_NEWLINE) {
            currentToken = lexer.nextToken();
            continue;
        }
        std::string stmt = parseSingleStatement(lexer, currentToken);
        if (!stmt.empty()) {
            whileStmt.body.push_back(stmt);
            if (isOutInfo) {
                std::cout << "[解析] while块内语句：" << stmt << std::endl;
            }
        }
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_RBRACE);
    return whileStmt;
}

// 执行 while 语句
void Parser::executeWhileStatement(const WhileStatement& whileStmt,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    if (whileStmt.condition.empty()) return;
    Lexer condLexer(whileStmt.condition);
    Token condToken = condLexer.nextToken();
    skipWhitespace(condLexer, condToken);
    auto condExpr = parseExpr(condLexer, condToken);
    Value condResult = condExpr->evaluate(variables, functions);
    while (condResult.asBool()) {
        if (isOutInfo) {
            std::cout << "[执行] while条件：" << whileStmt.condition << " → 真，进入循环" << std::endl;
        }
        for (const auto& stmt : whileStmt.body) {
            if (isOutInfo) {
                std::cout << "[执行] while块内执行：" << stmt << std::endl;
            }
            parseLine(stmt, variables, functions);
        }
        // 重新计算条件
        condLexer = Lexer(whileStmt.condition);
        condToken = condLexer.nextToken();
        skipWhitespace(condLexer, condToken);
        condExpr = parseExpr(condLexer, condToken);
        condResult = condExpr->evaluate(variables, functions);
    }
    if (isOutInfo) {
        std::cout << "[执行] while条件：" << whileStmt.condition << " → 假，退出循环" << std::endl;
    }
}

ForStatement Parser::parseForStatement(Lexer& lexer, Token& currentToken) {
    ForStatement forStmt;
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_FOR);
    eat(lexer, currentToken, TOKEN_LPAREN);

    std::string init;
    while (currentToken.type != TOKEN_SEMICOLON && currentToken.type != TOKEN_EOF) {
        init += currentToken.value;
        init += " ";
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_SEMICOLON);
    std::string condition;
    while (currentToken.type != TOKEN_SEMICOLON && currentToken.type != TOKEN_EOF) {
        condition += currentToken.value;
        condition += " ";
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_SEMICOLON);
    std::string iter;
    while (currentToken.type != TOKEN_RPAREN && currentToken.type != TOKEN_EOF) {
        iter += currentToken.value;
        iter += " ";
        currentToken = lexer.nextToken();
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_RPAREN);
    size_t start = init.find_first_not_of(" ");
    size_t end = init.find_last_not_of(" ");
    forStmt.init = (start != std::string::npos) ? init.substr(start, end - start + 1) : "";
    start = condition.find_first_not_of(" ");
    end = condition.find_last_not_of(" ");
    forStmt.condition = (start != std::string::npos) ? condition.substr(start, end - start + 1) : "";
    start = iter.find_first_not_of(" ");
    end = iter.find_last_not_of(" ");
    forStmt.iter = (start != std::string::npos) ? iter.substr(start, end - start + 1) : "";

    eat(lexer, currentToken, TOKEN_LBRACE);
    while (currentToken.type != TOKEN_RBRACE && currentToken.type != TOKEN_EOF) {
        if (currentToken.type == TOKEN_NEWLINE) {
            currentToken = lexer.nextToken();
            continue;
        }
        std::string stmt = parseSingleStatement(lexer, currentToken);
        if (!stmt.empty()) {
            forStmt.body.push_back(stmt);
            if (isOutInfo) {
                std::cout << "[解析] for块内语句：" << stmt << std::endl;
            }
        }
        skipWhitespace(lexer, currentToken);
    }
    eat(lexer, currentToken, TOKEN_RBRACE);
    return forStmt;
}

void Parser::executeForStatement(const ForStatement& forStmt,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    if (!forStmt.init.empty()) {
        parseLine(forStmt.init, variables, functions);
    }
    while (true) {
        if (!forStmt.condition.empty()) {
            Lexer condLexer(forStmt.condition);
            Token condToken = condLexer.nextToken();
            skipWhitespace(condLexer, condToken);
            auto condExpr = parseExpr(condLexer, condToken);
            Value condResult = condExpr->evaluate(variables, functions);
            if (!condResult.asBool()) break;
        }
        else {
            break;
        }
        if (isOutInfo) {
            std::cout << "[执行] for循环执行一次" << std::endl;
        }
        for (const auto& stmt : forStmt.body) {
            if (isOutInfo) {
                std::cout << "[执行] for块内执行：" << stmt << std::endl;
            }
            parseLine(stmt, variables, functions);
        }
        if (!forStmt.iter.empty()) {
            parseLine(forStmt.iter, variables, functions);
        }
    }
}

void Parser::parseImportStatement(Lexer& lexer, Token& currentToken,
    std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    skipWhitespace(lexer, currentToken);
    eat(lexer, currentToken, TOKEN_IMPORT);

    // 获取库名
    std::string libName = currentToken.value;
    eat(lexer, currentToken, TOKEN_IDENTIFIER);

    // 可选别名：import math -> m
    std::string alias;
    if (currentToken.type == TOKEN_ARROW) {
        eat(lexer, currentToken, TOKEN_ARROW);
        skipWhitespace(lexer, currentToken);
        if (currentToken.type != TOKEN_IDENTIFIER) {
            throw std::runtime_error(makeParseError(currentToken, "Expected alias name after '->'"));
        }
        alias = currentToken.value;
        eat(lexer, currentToken, TOKEN_IDENTIFIER);
    }

    // 检查库是否存在并加载
    auto& libMgr = LibraryManager::getInstance();

    // 解析外部调用路径 → 内部库名（如 "fox.sys.io.fs" → "file"）
    std::string internalName = libMgr.resolveExternalPath(libName);

    // 先检查原始名字（如果是外部路径，会被 isSystemLibrary 识别）
    if (libMgr.isSystemLibrary(libName)) {
        if (isOutInfo) {
            std::cout << "[导入] " << libName;
            if (!alias.empty()) std::cout << " -> " << alias;
            std::cout << std::endl;
        }
    }
    else if (libMgr.isLibraryAvailable(internalName)) {
        try {
            libMgr.loadExternalLibrary(internalName);
            if (isOutInfo) {
                std::cout << "[导入] ";
                if (libName != internalName) std::cout << libName << " (" << internalName << ")";
                else std::cout << libName;
                if (!alias.empty()) std::cout << " -> " << alias;
                std::cout << std::endl;
            }
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to import library " + libName + ": " + e.what());
        }
    }
    else {
        throw std::runtime_error(makeParseError(currentToken, "Library not found: " + libName + ", please ensure the library file exists in C:\\FoxLibs\\ directory"));
    }

    // 用 import 时写的名字作为调用名前缀
    // 外部路径（如 fox.sys.io.fs）自动取最后一段（fs），别名优先
    std::string callName;
    if (!alias.empty()) {
        callName = alias;
    } else if (libMgr.isExternalPath(libName)) {
        size_t lastDot = libName.rfind('.');
        callName = (lastDot != std::string::npos) ? libName.substr(lastDot + 1) : libName;
    } else {
        callName = libName;
    }
    libMgr.markImported(internalName, callName);
}