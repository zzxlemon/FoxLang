#include "./common.hpp"
#include "./interpreter.hpp"
#include "utils.hpp"
#include "error_reporter.hpp"
#include <iostream>
#include <cstdio>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <locale.h>
#endif

std::string fz_name_file = "";
std::string fz_name_file_v = "";
const char XOR_KEY = 0x2A;
std::string encrypt_key = "";

#ifdef _WIN32
static void enableWindowsUtf8() {
    // 1. Console output code page → UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 2. CRT stdout/stderr → binary mode，不做任何编码转换
    //    UTF-8 字节直达终端（终端已是 CP_UTF8）
    fflush(stdout);
    _setmode(_fileno(stdout), _O_BINARY);
    fflush(stderr);
    _setmode(_fileno(stderr), _O_BINARY);

    // 3. C++ locale
    setlocale(LC_ALL, ".UTF-8");

    // 4. Enable ANSI escape processing（for error_reporter colors）
    HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (outHandle != INVALID_HANDLE_VALUE) {
        DWORD mode;
        if (GetConsoleMode(outHandle, &mode)) {
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(outHandle, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
            SetConsoleMode(outHandle, mode | ENABLE_PROCESSED_OUTPUT);
#endif
        }
    }
    HANDLE errHandle = GetStdHandle(STD_ERROR_HANDLE);
    if (errHandle != INVALID_HANDLE_VALUE) {
        DWORD mode;
        if (GetConsoleMode(errHandle, &mode)) {
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(errHandle, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
            SetConsoleMode(errHandle, mode | ENABLE_PROCESSED_OUTPUT);
#endif
        }
    }
}
#endif

static void CreateFile(const std::string& source_code) {
    std::ofstream outfile(fz_name_file_v, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(source_code.c_str(), source_code.size());
        outfile.close();
        std::cout << "文件" << fz_name_file_v << "创建成功。" << std::endl;
    }
    else {
        ErrorReporter::reportSimple("FileError", "File " + fz_name_file_v + " open/create failed");
    }
}

static void fz(const std::string& key) {
    encrypt_file_with_key(fz_name_file, fz_name_file_v, key);
}

static void fz1(const std::string& key) {
    std::string source_code = decrypt_file_with_key(fz_name_file, key);
    CreateFile(source_code);
}

int main(int argc, char** argv) { 
#ifdef _WIN32
    enableWindowsUtf8();
#endif
    Interpreter interpreter;
    if (argc == 1) {
        return 0;  
    }

    int v1 = 1;
    std::string filename;
    for (int i = 1; i < argc; i++) {
        std::string in = argv[i];
        if (in == "-version" || in == "-v") {
            printf("Version: v%s\n", fox_version.c_str());
        }
        else if (in == "-f" || in == "--file") {
            if (i + 1 >= argc) {
                ErrorReporter::reportSimple("ArgumentError", "'-f' requires a filename");
                return 1;
            }
            filename = argv[i + 1];
            i++;
        }
        else if (argv[i] == "-str"){
            if (argv[i + 1] == "utf-8" || argv[i + 1] == "UTF-8"){
                system("chcp 65001");
            }
            else if (argv[i + 1] == "gbk" || argv[i + 1] == "GBK"){
                system("chcp 936");
            }
            else{
                ErrorReporter::reportSimple("ArgumentError", "'-str' requires an encoding format (utf-8 or gbk)");
                return 1;
            }
        }
        else if (in == "OutInfo=true") {
            isOutInfo = true;
        }
        else if (in == "pack=true") {
            v1 = 2;
            if (i + 3 >= argc) {
                ErrorReporter::reportSimple("ArgumentError", "pack=true requires 3 args: input output key");
                return 1;
            }
            fz_name_file = argv[i + 1];
            fz_name_file += ".fx";
            fz_name_file_v = argv[i + 2];
            fz_name_file_v += ".fz";
            encrypt_key = argv[i + 3];
            i += 3;
        }
        else if (in == "pack=false") {
            v1 = 3;
            if (i + 3 >= argc) {
                ErrorReporter::reportSimple("ArgumentError", "pack=false requires 3 args: input output key");
                return 1;
            }
            fz_name_file = argv[i + 1];
            fz_name_file += ".fz";
            fz_name_file_v = argv[i + 2];
            fz_name_file_v += ".fx";
            encrypt_key = argv[i + 3];
            i += 3;
        }
        else if (in == "-help" || in == "-h") {
            std::cout << "FoxLang 解释器使用说明:" << std::endl;
            std::cout << "  -f <filename>       指定要执行的 FoxLang 文件" << std::endl;
            std::cout << "  version             显示版本信息" << std::endl;
            std::cout << "  OutInfo=true        输出详细信息" << std::endl;
            std::cout << "  pack=true           将 .fx 文件加密为 .fz 文件" << std::endl;
            std::cout << "  pack=false          将 .fz 文件解密为 .fx 文件" << std::endl;
            return 0;
        }
        else {
            ErrorReporter::reportSimple("ArgumentError", "unknown parameter '" + in + "'");
            return 1;
        }
    }
    
    if (isOutInfo)
        std::cout << "指令解析完成" << std::endl;
    if (v1 == 2) {
        fz(encrypt_key);
    }
    else if (v1 == 3) {
        fz1(encrypt_key);
    }

    if (!filename.empty()) {
        std::string fullCode = read_file(filename);
        ErrorReporter::setSource(filename, fullCode);
        RegFunc(); // 初始化系统库，以便 import 能正确识别
        interpreter.parseCode(fullCode, filename);
        if (!interpreter.parse_failed) {
            interpreter.runMainFunc();
        }
    }

    return 0;
}
