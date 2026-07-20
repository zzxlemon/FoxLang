#pragma once

#include "../../../../src/interpreter/interpreter.hpp"

// Util
class Util {
public:
    Value length(const std::vector<Value>& args);
    Value IntChangeString(const std::vector<Value>& args);
    Value StringChangeInt(const std::vector<Value>& args);
    Value StringChangeDouble(const std::vector<Value>& args);
    Value DoubleChangeString(const std::vector<Value>& args);
    Value DoubleChangeInt(const std::vector<Value>& args);
    Value IntChangeDouble(const std::vector<Value>& args);
};