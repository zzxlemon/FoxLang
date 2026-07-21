#pragma once
#include <string>
#include <stdexcept>
#include <vector>

class Value {
public:
    enum class Type { Int, String, Double, Void, Array, Unknown };

    Value();
    Value(int v);
    Value(double v);
    Value(const std::string& v);
    Value(const char* v);
    Value(const std::vector<Value>& v);

    Type getType() const;

    std::vector<Value>& asArrayRef();

    int asInt() const;
    double asDouble() const;
    const std::string& asString() const;
    const std::vector<Value>& asArray() const;
    // 获取类型字节大小
    int getByteSize() const;

    bool asBool() const {
        switch (type) {
        case Type::Int: return intVal != 0;
        case Type::Double: return doubleVal != 0.0;
        case Type::Array: return !arrVal.empty();
        default: throw std::runtime_error("Only int/double/array types supported as condition");
        }
    }

private:
    Type type;
    int intVal;
    double doubleVal;
    std::string strVal;
    std::vector<Value> arrVal;
};