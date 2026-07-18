#include "./SystemFunctionsRandom.h"
#include <random>
#include <string>
#include <cmath>
#include <sstream>
#include <climits>   // for INT_MIN, INT_MAX

std::mt19937& Random::rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

// 将 Value 转为 int
static int toIntSafe(const Value& v, const char* paramName) {
    if (v.getType() == Value::Type::Int) return v.asInt();
    if (v.getType() == Value::Type::Double) {
        double d = v.asDouble();
        // 检查是否超出 int 范围
        if (d < static_cast<double>(INT_MIN) || d > static_cast<double>(INT_MAX)) {
            std::ostringstream oss;
            oss << "random函数参数错误：" << paramName << " 值 " << d << " 超出 int 范围";
            throw std::runtime_error(oss.str());
        }
        int i = static_cast<int>(d);
        if (std::fabs(d - i) > 1e-9) {
            std::ostringstream oss;
            oss << "random函数参数错误：" << paramName << " 需要整数，但传入了非整数浮点数 " << d;
            throw std::runtime_error(oss.str());
        }
        return i;
    }
    std::ostringstream oss;
    oss << "random函数参数错误：" << paramName << " 必须是数字（int或double）";
    throw std::runtime_error(oss.str());
}

// 将 Value 转为 double
static double toDoubleSafe(const Value& v, const char* paramName) {
    if (v.getType() == Value::Type::Int) return static_cast<double>(v.asInt());
    if (v.getType() == Value::Type::Double) return v.asDouble();
    std::ostringstream oss;
    oss << "random函数参数错误：" << paramName << " 必须是数字（int或double）";
    throw std::runtime_error(oss.str());
}

Value Random::RandomStart(const std::vector<Value>& args) {
    if (args.size() != 3) {
        throw std::runtime_error("random函数需要3个参数：random('int', min, max) 或 random(\"double\", min, max)");
    }

    const Value& random_type_v = args[0];
    const Value& random_min = args[1];  
    const Value& random_max = args[2];

    if (random_type_v.asString() == "int") {
        int min = toIntSafe(random_min, "min");
        int max = toIntSafe(random_max, "max");
        return RandomInt(min, max);
    }
    else if (random_type_v.asString() == "double") {
        double min = toDoubleSafe(random_min, "min");
        double max = toDoubleSafe(random_max, "max");
        return RandomDouble(min, max);
    }
    else {
        throw std::runtime_error("random函数第一个参数必须是字符串 'int' 或 'double'");
    }
}

Value Random::RandomInt(const int min, const int max) {
    if (min > max) {
        std::ostringstream oss;
        oss << "random函数参数错误：RandomInt 要求 min <= max，实际 min=" << min << ", max=" << max;
        throw std::runtime_error(oss.str());
    }
    // uniform_int_distribution 包含两端 [min, max]
    return Value(std::uniform_int_distribution<int>(min, max)(rng()));
}

Value Random::RandomDouble(const double min, const double max) {
    if (min > max) {
        std::ostringstream oss;
        oss << "random函数参数错误：RandomDouble 要求 min <= max，实际 min=" << min << ", max=" << max;
        throw std::runtime_error(oss.str());
    }
    // uniform_real_distribution 生成 [min, max) 范围内的浮点数
    return Value(std::uniform_real_distribution<double>(min, max)(rng()));
}