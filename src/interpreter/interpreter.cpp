#include "interpreter.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "error_reporter.hpp"
#include <iostream>
#include <stdexcept>
#include "../libs/system/random/SystemFunctionsRandom.h"
#include "../libs/system/fs/SystemFunctionsFile.h"
#include "library_manager.hpp"   

std::vector<std::string> functions_register_map;

void Interpreter::parseCode(const std::string& code, const std::string& filename) {
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
        parse_failed = true;
        ErrorReporter::reportParseError(e.what());
    }
}

Value Interpreter::execute(const std::string& line) {
    if (line.empty()) return Value();
    return Parser::parseLine(line, variables, functions);
}

Value Interpreter::executeFunction(const Function& func) {
    Value returnValue;
    if (isOutInfo) {
        std::cout << "\n===== 执行函数：" << func.name << " =====" << std::endl;
    }
    for (const auto& stmt : func.body) {
        if (isOutInfo) {
            std::cout << "[执行] 语句：" << stmt << std::endl;
        }
        Value val = execute(stmt);
        if (stmt.find("ret") == 0) {
            returnValue = val;
            break;
        }
    }

    if (func.returnType != "void") {
        if (func.returnType == "int" && returnValue.getType() != Value::Type::Int) {
            throw std::runtime_error("Function " + func.name + " expects to return int type, actually returned " +
                (returnValue.getType() == Value::Type::Void ? "void" : "other type"));
        }
        else if (func.returnType == "string" && returnValue.getType() != Value::Type::String) {
            throw std::runtime_error("Function " + func.name + " expects to return string type, actually returned other type");
        }
        else if (func.returnType == "double" && returnValue.getType() != Value::Type::Double) {
            throw std::runtime_error("Function " + func.name + " expects to return double type, actually returned other type");
        }
    }

    return returnValue;
}

void Interpreter::runMainFunc() {
    if (isOutInfo) {
        printf("==========RUN==========\n");
    }
    if (parse_failed) {
        return;
    }
    if (functions.find("main") == functions.end()) {
        ErrorReporter::reportSimple("RuntimeError", "main function not found",
            "every FoxLang program must define a 'main' function");
        return;
    }
    try {
        RegFunc();
        executeFunction(functions["main"]);
    }
    catch (const std::exception& e) {
        ErrorReporter::reportFromException("RuntimeError", e.what());
    }
}

// System function registration
bool Interpreter::isSystemFunction(const std::string& funcName) {
    auto& libMgr = LibraryManager::getInstance();

    // 兼容旧格式：直接函数名
    // 但如果该函数名属于某个已导入的库，则禁止平名调用，必须使用 lib.func 格式
    auto checkAndBlock = [&](const std::string& name) -> bool {
        if (libMgr.isFlatNameBlockedByImport(name)) return false;
        return true;
    };

    if (funcName == "random") return checkAndBlock(funcName);
    if (funcName == "file_open" || funcName == "file_read" ||
        funcName == "file_write" || funcName == "file_close" || funcName == "file_remove" || 
        funcName == "len" || funcName == "length" ||
        funcName == "cos" || funcName == "sin" || funcName == "tan") return checkAndBlock(funcName);

    // 新格式：库名.函数名（支持多段路径如 fox.sys.io.fs.file_open）
    size_t dotPos = funcName.rfind('.');
    if (dotPos != std::string::npos) {
        std::string libName = funcName.substr(0, dotPos);
        libName = libMgr.resolveAlias(libName);
        return libMgr.isSystemLibrary(libName) || libMgr.hasLibrary(libName);
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
    else if (funcName == "len" || funcName == "length") {
        return libMgr.callSystemFunction("util", "len", args);
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
        // 新格式：lib.func（支持多段路径如 fox.sys.io.fs.file_open）
        size_t dotPos = funcName.rfind('.');
        if (dotPos != std::string::npos) {
            std::string libName = funcName.substr(0, dotPos);
            std::string funcOnly = funcName.substr(dotPos + 1);
            libName = libMgr.resolveAlias(libName);
            return libMgr.callSystemFunction(libName, funcOnly, args);
        }
        throw std::runtime_error("Unimplemented system function: " + funcName);
    }
}

void RegFunc() {
    static bool initialized = false;
    if (!initialized) {
        initSystemLibraries();
        initialized = true;
    }
}