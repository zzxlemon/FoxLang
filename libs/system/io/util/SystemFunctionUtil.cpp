#include "SystemFunctionUtil.h"

Value Util::length(const std::vector<Value>& args) {
    if (args.size() != 1) {
            throw std::runtime_error("len() requires one array argument");
    }
    const Value& arg = args[0];
    if (arg.getType() != Value::Type::Array) {
        throw std::runtime_error("len() only supports array type");
    }
    return Value(static_cast<int>(arg.asArray().size()));
}

Value Util::IntChangeString(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("IntChangeString: need 1 arg (int)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::Int) {
        throw std::runtime_error("IntChangeString: arg must be an int");
    }

    return Value(std::to_string(a.asInt()));
}

Value Util::StringChangeInt(const std::vector<Value>& args) {
    if (args.size() != 1){
        throw std::runtime_error("StringChangeInt: need 1 arg (string)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::String) {
        throw std::runtime_error("StringChangeInt: arg must be a string");
    }

    int result = std::stoi(a.asString());
    return Value(result);
}

Value Util::StringChangeDouble(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("StringChangeDouble: need 1 arg (string)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::String) {
        throw std::runtime_error("StringChangeDouble: arg must be a string");
    }

    double result = std::stod(a.asString());
    return Value(result);
}

Value Util::DoubleChangeString(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("DoubleChangeString: need 1 arg (double)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::Double) {
        throw std::runtime_error("DoubleChangeString: arg must be a double");
    }

    return Value(std::to_string(a.asDouble()));
}

Value Util::DoubleChangeInt(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("DoubleChangeInt: need 1 arg (double)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::Double) {
        throw std::runtime_error("DoubleChangeInt: arg must be a double");
    }

    return Value(static_cast<int>(a.asDouble()));
}

Value Util::IntChangeDouble(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("IntChangeDouble: need 1 arg (int)");
    }
    const Value& a = args[0];
    if (a.getType() != Value::Type::Int) {
        throw std::runtime_error("IntChangeDouble: arg must be an int");
    }

    return Value(static_cast<double>(a.asInt()));
}