#pragma once
#include "value.hpp"
#include "function.hpp"
#include "interpreter.hpp"
#include "common.hpp"
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <cstdio>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define access _access
#else
#include <unistd.h>
#endif

// 简单的文件存在检查
inline bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 库管理器单例类
class LibraryManager {
public:
    static LibraryManager& getInstance() {
        static LibraryManager instance;
        return instance;
    }

    // 注册系统库函数
    void registerSystemFunction(const std::string& libName,
        const std::string& funcName,
        std::function<Value(const std::vector<Value>&)> func) {
        systemLibraries[libName][funcName] = func;
    }

    // 注册系统库
    void registerLibrary(const std::string& libName) {
        if (systemLibraries.find(libName) == systemLibraries.end()) {
            systemLibraries[libName] = {};
        }
    }

    // 检查是否为系统库
    bool isSystemLibrary(const std::string& libName) {
        return systemLibraries.find(libName) != systemLibraries.end();
    }

    // 调用系统库函数
    Value callSystemFunction(const std::string& libName,
        const std::string& funcName,
        const std::vector<Value>& args) {
        auto libIt = systemLibraries.find(libName);
        if (libIt == systemLibraries.end()) {
            throw std::runtime_error("未找到库: " + libName);
        }

        auto funcIt = libIt->second.find(funcName);
        if (funcIt == libIt->second.end()) {
            throw std::runtime_error("库 " + libName + " 中未找到函数: " + funcName);
        }

        return funcIt->second(args);
    }

    // API: 添加外部库（从文件路径）
    void addExternalLibrary(const std::string& libName, const std::string& libPath) {
        externalLibraryPaths[libName] = libPath;
        externalLibsToLoad[libName] = true;
    }

    // API: 添加外部库函数（注意：三个参数！）
    void addExternalLibraryFunction(const std::string& libName,
        const std::string& funcName,
        std::function<Value(const std::vector<Value>&)> func) {
        externalLibraries[libName][funcName] = func;
        loadedExternalLibs.insert(libName);
    }

    // 加载外部库
    void loadExternalLibrary(const std::string& libName) {
        if (loadedExternalLibs.find(libName) != loadedExternalLibs.end()) {
            return;
        }

        if (externalLibraries.find(libName) != externalLibraries.end()) {
            loadedExternalLibs.insert(libName);
            return;
        }

        std::vector<std::string> searchPaths = {
            "C:\\FoxLibs\\",
            "C:\\Program Files\\FoxLang\\Libs\\",
            ".\\libs\\",
            ".\\"
        };

        for (const auto& basePath : searchPaths) {
            std::string libFilePath = basePath + libName + ".foxlib";
            if (fileExists(libFilePath)) {
                loadLibraryFromFile(libName, libFilePath);
                loadedExternalLibs.insert(libName);
                if (isOutInfo) {
                    std::cout << "[库管理] 从 " << libFilePath << " 加载库 " << libName << std::endl;
                }
                return;
            }
        }

        auto pathIt = externalLibraryPaths.find(libName);
        if (pathIt != externalLibraryPaths.end() && fileExists(pathIt->second)) {
            loadLibraryFromFile(libName, pathIt->second);
            loadedExternalLibs.insert(libName);
            return;
        }

        throw std::runtime_error("无法找到外部库: " + libName);
    }

    bool isLibraryAvailable(const std::string& libName) {
        if (isSystemLibrary(libName)) return true;
        if (loadedExternalLibs.find(libName) != loadedExternalLibs.end()) return true;
        if (externalLibraries.find(libName) != externalLibraries.end()) return true;

        std::vector<std::string> searchPaths = {
            "C:\\FoxLibs\\",
            "C:\\Program Files\\FoxLang\\Libs\\",
            ".\\libs\\",
            ".\\"
        };
        for (const auto& basePath : searchPaths) {
            std::string libFilePath = basePath + libName + ".foxlib";
            if (fileExists(libFilePath)) return true;
        }
        auto pathIt = externalLibraryPaths.find(libName);
        if (pathIt != externalLibraryPaths.end()) {
            return fileExists(pathIt->second);
        }
        return false;
    }

    const std::unordered_set<std::string>& getImportedLibraries() const {
        return loadedExternalLibs;
    }

private:
    LibraryManager() = default;

    void loadLibraryFromFile(const std::string& libName, const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("无法打开库文件: " + filePath);
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line.find("function:") == 0) {
                std::string funcName = line.substr(9);
                funcName.erase(0, funcName.find_first_not_of(" \t"));
                funcName.erase(funcName.find_last_not_of(" \t") + 1);
                if (isOutInfo) {
                    std::cout << "[库管理] 发现函数声明: " << funcName << std::endl;
                }
            }
            else if (line.find("library:") == 0) {
                std::string nameInFile = line.substr(8);
                nameInFile.erase(0, nameInFile.find_first_not_of(" \t"));
                nameInFile.erase(nameInFile.find_last_not_of(" \t") + 1);
                if (nameInFile != libName) {
                    throw std::runtime_error("库名不匹配: " + nameInFile + " != " + libName);
                }
            }
        }
        file.close();
        loadedExternalLibs.insert(libName);
    }

    std::unordered_map<std::string, std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>> systemLibraries;
    std::unordered_map<std::string, std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>> externalLibraries;
    std::unordered_map<std::string, std::string> externalLibraryPaths;
    std::unordered_map<std::string, bool> externalLibsToLoad;
    std::unordered_set<std::string> loadedExternalLibs;
};

// 初始化系统库（声明）
void initSystemLibraries();

// API 函数：添加外部库（从路径）
inline void AddExternalLibrary(const std::string& libName, const std::string& libPath) {
    LibraryManager::getInstance().addExternalLibrary(libName, libPath);
}

// API 函数：添加外部库函数（三个参数）
inline void AddExternalFunction(const std::string& libName,
    const std::string& funcName,
    std::function<Value(const std::vector<Value>&)> func) {
    LibraryManager::getInstance().addExternalLibraryFunction(libName, funcName, func);
}

// API 函数：检查库是否可用
inline bool IsLibraryAvailable(const std::string& libName) {
    return LibraryManager::getInstance().isLibraryAvailable(libName);
}