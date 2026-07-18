#pragma once
#include <string>
#include <stdexcept>

class Value {
public:
    enum class Type { Int, String, Double, Void };

    Value();
    Value(int v);
    Value(double v);
    Value(const std::string& v);
    Value(const char* v);

    Type getType() const;

    int asInt() const;
    double asDouble() const;
    const std::string& asString() const;
    // 获取类型字节大小
    int getByteSize() const;

    bool asBool() const {
        switch (type) {
        case Type::Int: return intVal != 0;
        case Type::Double: return doubleVal != 0.0;
        default: throw std::runtime_error("仅支持int/double类型作为条件判断");
        }
    }

private:
    Type type;
    int intVal;
    double doubleVal;
    std::string strVal;
};