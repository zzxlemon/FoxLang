#include "ast.hpp"
#include "../interpreter/interpreter.hpp"
#include "../interpreter/library_manager.hpp"
#include <iostream>   
#include <stdexcept> 

IdentifierExpr::IdentifierExpr(const std::string& n) : name(n) {}
Value IdentifierExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    if (variables.find(name) == variables.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return variables[name];
}

NumberExpr::NumberExpr(int v) : value(v) {}
Value NumberExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    return Value(value);
}

DoubleExpr::DoubleExpr(double v) : value(v) {}
Value DoubleExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    return Value(value);
}

StringExpr::StringExpr(const std::string& v) : value(v) {}
Value StringExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    return Value(value);
}

ArrayExpr::ArrayExpr(std::vector<std::unique_ptr<Expr>>&& elems) : elements(std::move(elems)) {}

Value ArrayExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    std::vector<Value> values;
    for (const auto& elem : elements) {
        values.push_back(elem->evaluate(variables, functions));
    }
    return Value(values);
}

IndexExpr::IndexExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx)
    : arrayExpr(std::move(arr)), indexExpr(std::move(idx)) {}

Value IndexExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Value arrValue = arrayExpr->evaluate(variables, functions);
    if (arrValue.getType() != Value::Type::Array) {
        throw std::runtime_error("Index target is not an array type");
    }
    int idx = indexExpr->evaluate(variables, functions).asInt();
    const std::vector<Value>& arr = arrValue.asArray();
    if (idx < 0 || idx >= static_cast<int>(arr.size())) {
        throw std::runtime_error("Array index out of bounds: " + std::to_string(idx));
    }
    return arr[idx];
}

CallExpr::CallExpr(const std::string& name) : funcName(name) {}
CallExpr::CallExpr(const std::string& name, std::vector<std::unique_ptr<Expr>>&& arguments)
    : funcName(name), args(std::move(arguments)) {
}
Value CallExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    // 先计算参数值
    std::vector<Value> argVals;
    argVals.reserve(args.size());
    for (const auto& a : args) {
        argVals.push_back(a->evaluate(variables, functions));
    }

    // 如果是系统函数表中登记的函数名，调用 SystemFunctionBuildIn
    Interpreter sys;
    if (sys.isSystemFunction(funcName)) {
        return sys.SystemFunctionBuildIn(funcName, argVals);
    }

    /* 
    * @Abandon
    if (!functions_register_map.empty()) {
        auto it = std::find(functions_register_map.begin(), functions_register_map.end(), funcName);
        if (it != functions_register_map.end()) {
            Interpreter sys;
            return sys.SystemFunctionBuildIn(funcName, argVals);
        }
    }
    */

    // 否则按原有流程查找脚本函数
    if (functions.find(funcName) == functions.end()) {
        auto& libMgr = LibraryManager::getInstance();
        std::string libName = libMgr.getBlockedLibName(funcName);
        if (!libName.empty()) {
            std::string shortName = LibraryManager::getLastSegment(libName);
            throw std::runtime_error("Function '" + funcName + "' is from the '" + libName
                + "' library. You must call it with the library prefix: '" + shortName + "." + funcName + "(...)'.");
        }
        std::string sysLibPath = libMgr.getSystemFuncExternalPath(funcName);
        if (!sysLibPath.empty()) {
            std::string shortName = LibraryManager::getLastSegment(sysLibPath);
            throw std::runtime_error("Function '" + funcName + "' requires importing a library first.\n"
                "  Use: import " + sysLibPath + "\n"
                "  Then: " + shortName + "." + funcName + "(...)\n"
                "  Or with alias: import " + sysLibPath + " -> my_alias\n"
                "  Then: my_alias." + funcName + "(...)");
        }
        throw std::runtime_error("Undefined function: " + funcName);
    }

    const Function& func = functions[funcName];

    // 检查参数数量
    if (argVals.size() != func.parameters.size()) {
        throw std::runtime_error("Function " + funcName + " expects " +
            std::to_string(func.parameters.size()) + " arguments, got " +
            std::to_string(argVals.size()));
    }

    // 创建局部作用域并绑定参数
    Interpreter funcInterp;
    funcInterp.variables = variables;
    funcInterp.functions = functions;

    for (size_t i = 0; i < argVals.size(); ++i) {
        funcInterp.variables[func.parameters[i].name] = argVals[i];
    }

    Value result = funcInterp.executeFunction(func);

    return result;
}

BinaryExpr::BinaryExpr(std::unique_ptr<Expr> l, TokenT o, std::unique_ptr<Expr> r)
    : left(std::move(l)), op(o), right(std::move(r)) {
}

Value BinaryExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Value leftVal = left->evaluate(variables, functions);
    Value rightVal = right->evaluate(variables, functions);

    if (leftVal.getType() == Value::Type::Int && rightVal.getType() == Value::Type::Int) {
        int result;
        switch (op) {
        case TOKEN_PLUS: result = leftVal.asInt() + rightVal.asInt(); break;
        case TOKEN_MINUS: result = leftVal.asInt() - rightVal.asInt(); break;
        default: throw std::runtime_error("Unsupported operator");
        }
        return Value(result);
    }
    else if (leftVal.getType() == Value::Type::Double && rightVal.getType() == Value::Type::Double) {
        double result;
        switch (op) {
        case TOKEN_PLUS: result = leftVal.asDouble() + rightVal.asDouble(); break;
        case TOKEN_MINUS: result = leftVal.asDouble() - rightVal.asDouble(); break;
        default: throw std::runtime_error("Unsupported operator");
        }
        return Value(result);
    }
    else {
        throw std::runtime_error("Operation error: type mismatch (only int/double of same type supported)");
    }
}

Value InputExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    std::string userInput;
    std::getline(std::cin, userInput);
    return Value(userInput);
}

Value CastExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Value val = expr->evaluate(variables, functions);
    switch (val.getType()) {
    case Value::Type::Int:
        if (castType == CastType::Int) {
            return Value(val.asInt());
        }
        else if (castType == CastType::Double) {
            return Value(static_cast<double>(val.asInt()));
        }
        break;
    case Value::Type::Double:
        if (castType == CastType::Int) {
            return Value(static_cast<int>(val.asDouble()));
        }
        else if (castType == CastType::Double) {
            return Value(val.asDouble());
        }
        break;
    case Value::Type::String: {
        std::string strVal = val.asString();
        try {
            if (castType == CastType::Int) {
                int intVal = std::stoi(strVal);
                return Value(intVal);
            }
            else if (castType == CastType::Double) {
                double doubleVal = std::stod(strVal);
                return Value(doubleVal);
            }
        }
        catch (const std::invalid_argument& e) {
            throw std::runtime_error("Cast error: cannot convert \"" + strVal + "\" to numeric value");
        }
        catch (const std::out_of_range& e) {
            throw std::runtime_error("Cast error: \"" + strVal + "\" out of numeric range");
        }
        break;
    }
    default:
        break;
    }

    throw std::runtime_error("Cast error: unsupported source type for cast");
}

Value CompareExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Value leftVal = left->evaluate(variables, functions);
    Value rightVal = right->evaluate(variables, functions);
    bool result = false;

    if (leftVal.getType() == Value::Type::Int && rightVal.getType() == Value::Type::Int) {
        int l = leftVal.asInt(), r = rightVal.asInt();
        switch (op) {
        case CompareType::EQ: result = (l == r); break;
        case CompareType::NE: result = (l != r); break;
        case CompareType::GT: result = (l > r); break;
        case CompareType::LT: result = (l < r); break;
        case CompareType::GE: result = (l >= r); break;
        case CompareType::LE: result = (l <= r); break;
        }
    }
    else if (leftVal.getType() == Value::Type::Double && rightVal.getType() == Value::Type::Double) {
        double l = leftVal.asDouble(), r = rightVal.asDouble();
        switch (op) {
        case CompareType::EQ: result = (l == r); break;
        case CompareType::NE: result = (l != r); break;
        case CompareType::GT: result = (l > r); break;
        case CompareType::LT: result = (l < r); break;
        case CompareType::GE: result = (l >= r); break;
        case CompareType::LE: result = (l <= r); break;
        }
    }
    else {
        throw std::runtime_error("Comparison error: only int/double of same type supported");
    }

    return Value(result ? 1 : 0);
}

Value ConditionExpr::evaluate(std::unordered_map<std::string, Value>& variables,
    std::unordered_map<std::string, Function>& functions) {
    Value leftVal = left->evaluate(variables, functions);
    bool leftBool = leftVal.asBool();

    if (op == TOKEN_OR) {
        if (leftBool) return Value(1);
        Value rightVal = right->evaluate(variables, functions);
        return Value(rightVal.asBool() ? 1 : 0);
    }
    else if (op == TOKEN_AND) {
        if (!leftBool) return Value(0);
        Value rightVal = right->evaluate(variables, functions);
        return Value(rightVal.asBool() ? 1 : 0);
    }
    else {
        throw std::runtime_error("Unsupported condition operator");
    }
}