#include "value.hpp"
#include "common.hpp"

// 构造函数实现
Value::Value() : type(Type::Void), intVal(0), doubleVal(0.0), strVal("") {}
Value::Value(int v) : type(Type::Int), intVal(v), doubleVal(0.0), strVal("") {}
Value::Value(double v) : type(Type::Double), intVal(0), doubleVal(v), strVal("") {}
Value::Value(const std::string& v) : type(Type::String), intVal(0), doubleVal(0.0), strVal(v) {}
Value::Value(const char* v) : type(Type::String), intVal(0), doubleVal(0.0), strVal(v) {}

Value::Type Value::getType() const { return type; }

int Value::asInt() const {
    if (type != Type::Int) throw std::runtime_error("值不是整数类型");
    return intVal;
}

double Value::asDouble() const {
    if (type != Type::Double) throw std::runtime_error("值不是浮点类型");
    return doubleVal;
}

const std::string& Value::asString() const {
    if (type != Type::String) throw std::runtime_error("值不是字符串类型");
    return strVal;
}

int Value::getByteSize() const {
    switch (type) {
    case Type::Int: return INT_BYTE_SIZE;
    case Type::Double: return DOUBLE_BYTE_SIZE;
    case Type::String: return STRING_BYTE_SIZE;
    case Type::Void: return VOID_BYTE_SIZE;
    default: return 0;
    }
}