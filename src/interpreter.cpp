#include "interpreter.hpp"
#include "common.hpp"
#include "parser.hpp"
#include <iostream>
#include <stdexcept>
#include "../libs/system/random/SystemFunctionsRandom.h"
#include "../libs/system/fs/SystemFunctionsFile.h"
#include "library_manager.hpp"   

std::vector<std::string> functions_register_map;

// 解析所有函数定义
void Interpreter::parseCode(const std::string& code) {
    if (code.empty()) return;
    try {
        Parser parser(code, variables, functions);
        parser.parseAllFunctions();
        if (isOutInfo) {
            std::cout << "\n===== 解析完成的函数列表 =====" << std::endl;
            for (const auto& pair : functions) {
                const auto& func = pair.second;
                std::cout << "函数名：" << func.name << " 返回类型：" << func.returnType << " 语句数：" << func.body.size() << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "解析错误：" << e.what() << std::endl;
    }
}

// 执行单行语句
Value Interpreter::execute(const std::string& line) {
    if (line.empty()) return Value();
    try {
        return Parser::parseLine(line, variables, functions);
    }
    catch (const std::exception& e) {
        std::cerr << "执行错误：" << e.what() << std::endl;
        throw;  // 重新抛出异常而不是返回 Void
    }
}

// 执行函数
Value Interpreter::executeFunction(const Function& func) {
    Value returnValue;
    if (isOutInfo) {
        std::cout << "\n===== 执行函数：" << func.name << " =====" << std::endl;
    }
    for (const auto& stmt : func.body) {
        if (isOutInfo) {
            std::cout << "[执行] 语句：" << stmt << std::endl;
        }
        try {
            Value val = execute(stmt);
            if (stmt.find("ret") == 0) {
                returnValue = val;
                break;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "执行错误：" << e.what() << std::endl;
            throw;
        }
    }

    // 检查返回类型是否匹配（void 类型不需要检查）
    if (func.returnType != "void") {
        if (func.returnType == "int" && returnValue.getType() != Value::Type::Int) {
            throw std::runtime_error("函数 " + func.name + " 期望返回int类型，实际返回" +
                (returnValue.getType() == Value::Type::Void ? "void" : "其他类型"));
        }
        else if (func.returnType == "string" && returnValue.getType() != Value::Type::String) {
            throw std::runtime_error("函数 " + func.name + " 期望返回string类型，实际返回其他类型");
        }
        else if (func.returnType == "double" && returnValue.getType() != Value::Type::Double) {
            throw std::runtime_error("函数 " + func.name + " 期望返回double类型，实际返回其他类型");
        }
    }

    return returnValue;
}

void Interpreter::runMainFunc() {
    if (isOutInfo) {
        printf("==========RUN==========\n");
    }
    if (functions.find("main") == functions.end()) {
        std::cerr << "错误：未找到main函数" << std::endl;
        return;
    }
    try {
        RegFunc(); // 注册系统函数
        executeFunction(functions["main"]);
    }
    catch (const std::exception& e) {
        std::cerr << "执行错误：" << e.what() << std::endl;
    }
}

// System function registration
bool Interpreter::isSystemFunction(const std::string& funcName) {
    auto& libMgr = LibraryManager::getInstance();

    // 兼容旧格式：直接函数名
    if (funcName == "random") return true;
    if (funcName == "file_open" || funcName == "file_read" ||
        funcName == "file_write" || funcName == "file_close" || funcName == "file_remove" || 
        funcName == "cos" || funcName == "sin" || funcName == "tan") return true;

    // 新格式：库名.函数名
    size_t dotPos = funcName.find('.');
    if (dotPos != std::string::npos) {
        std::string libName = funcName.substr(0, dotPos);
        return libMgr.isSystemLibrary(libName);
    }

    return false;
}

Value Interpreter::SystemFunctionBuildIn(const std::string& funcName, const std::vector<Value>& args) {
    auto& libMgr = LibraryManager::getInstance();

    // 兼容旧格式
    if (funcName == "random") {
        return libMgr.callSystemFunction("random", "random", args);
    }
    else if (funcName == "file_open") {
        return libMgr.callSystemFunction("file", "file_open", args);
    }
    else if (funcName == "file_read") {
        return libMgr.callSystemFunction("file", "file_read", args);
    }
    else if (funcName == "file_write") {
        return libMgr.callSystemFunction("file", "file_write", args);
    }
    else if (funcName == "file_close") {
        return libMgr.callSystemFunction("file", "file_close", args);
    }
    else if (funcName == "file_remove") {
        return libMgr.callSystemFunction("file", "remove", args);
    }
    else if (funcName == "cos") {
        return libMgr.callSystemFunction("math", "cos", args);
    }
    else if (funcName == "sin") {
		return libMgr.callSystemFunction("math", "sin", args);
    }
    else if (funcName == "tan") {
        return libMgr.callSystemFunction("math", "tan", args);
	}
    else {
        // 新格式：lib.func
        size_t dotPos = funcName.find('.');
        if (dotPos != std::string::npos) {
            std::string libName = funcName.substr(0, dotPos);
            std::string funcOnly = funcName.substr(dotPos + 1);
            return libMgr.callSystemFunction(libName, funcOnly, args);
        }
        throw std::runtime_error("未实现的系统函数: " + funcName);
    }
}

void RegFunc() {
    static bool initialized = false;
    if (!initialized) {
        initSystemLibraries();
        initialized = true;
    }
}