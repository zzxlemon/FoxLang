#include "library_manager.hpp"
#include "common.hpp"
// lib include
#include "../libs/system/random/SystemFunctionsRandom.h"
#include "../libs/system/fs/SystemFunctionsFile.h"
#include "../libs/system/math/SystemFunctionsMath.h"

void initSystemLibraries() {
    auto& libMgr = LibraryManager::getInstance();

    // 注册 random 库
    libMgr.registerLibrary("random");
    libMgr.registerSystemFunction("random", "random", [](const std::vector<Value>& args) -> Value {
        Random random;
        return random.RandomStart(args);
        });

    // 注册 file 库
    libMgr.registerLibrary("file");
    libMgr.registerSystemFunction("file", "file_open", [](const std::vector<Value>& args) -> Value {
        File file;
        return file.FileOpen(args);
        });
    libMgr.registerSystemFunction("file", "file_read", [](const std::vector<Value>& args) -> Value {
        File file;
        return file.FileRead(args);
        });
    libMgr.registerSystemFunction("file", "file_write", [](const std::vector<Value>& args) -> Value {
        File file;
        return file.FileWrite(args);
        });
    libMgr.registerSystemFunction("file", "file_close", [](const std::vector<Value>& args) -> Value {
        File file;
        return file.FileClose(args);
        });
    libMgr.registerSystemFunction("file", "file_remove", [](const std::vector<Value>& args) -> Value {
        File file;
        return file.FileDelete(args);
        });
    
    // Register Math lib
	libMgr.registerLibrary("math");
    libMgr.registerSystemFunction("math", "sin", [](const std::vector<Value>& args) -> Value {
        Math math;
        return math.sin(args);
		});
    libMgr.registerSystemFunction("math", "cos", [](const std::vector<Value>& args)->Value {
        Math math;
        return math.cos(args);
	});
    libMgr.registerSystemFunction("math", "tan", [](const std::vector<Value>& args) -> Value {
        Math math;
        return math.tan(args);
        });


    if (isOutInfo) {
        std::cout << "[系统] 系统库初始化完成" << std::endl;
    }
}