#pragma once
#include <string>
#include <vector>

struct Parameter {
    std::string name;
    std::string type;
};

struct Function {
    std::string name;              
    std::string returnType;        
    std::vector<Parameter> parameters; 
    std::vector<std::string> body;  
};