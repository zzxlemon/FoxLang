#include "value.hpp"
#include "common.hpp"

// 构造函数实现
Value::Value() : type(Type::Void), intVal(0), doubleVal(0.0), strVal(""), arrVal() {}
Value::Value(int v) : type(Type::Int), intVal(v), doubleVal(0.0), strVal(""), arrVal() {}
Value::Value(double v) : type(Type::Double), intVal(0), doubleVal(v), strVal(""), arrVal() {}
Value::Value(const std::string& v) : type(Type::String), intVal(0), doubleVal(0.0), strVal(v), arrVal() {}
Value::Value(const char* v) : type(Type::String), intVal(0), doubleVal(0.0), strVal(v), arrVal() {}
Value::Value(const std::vector<Value>& v) : type(Type::Array), intVal(0), doubleVal(0.0), strVal(""), arrVal(v) {}

std::vector<Value>& Value::asArrayRef() {
    if (type != Type::Array) throw std::runtime_error("Value is not an array type");
    return arrVal;
}

Value::Type Value::getType() const { return type; }

int Value::asInt() const {
    if (type != Type::Int) throw std::runtime_error("Value is not an integer type");
    return intVal;
}

double Value::asDouble() const {
    if (type != Type::Double) throw std::runtime_error("Value is not a double type");
    return doubleVal;
}

const std::string& Value::asString() const {
    if (type != Type::String) throw std::runtime_error("Value is not a string type");
    return strVal;
}

const std::vector<Value>& Value::asArray() const {
    if (type != Type::Array) throw std::runtime_error("Value is not an array type");
    return arrVal;
}

int Value::getByteSize() const {
    switch (type) {
    case Type::Int: return INT_BYTE_SIZE;
    case Type::Double: return DOUBLE_BYTE_SIZE;
    case Type::String: return STRING_BYTE_SIZE;
    case Type::Array: return STRING_BYTE_SIZE;
    case Type::Void: return VOID_BYTE_SIZE;
    default: return 0;
    }
}