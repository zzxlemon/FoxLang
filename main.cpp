#include "./common.hpp"
#include "./interpreter.hpp"
#include "utils.hpp"
#include <iostream>
#include <cstdio>
#include <fstream>  

std::string fz_name_file = "";
std::string fz_name_file_v = "";
const char XOR_KEY = 0x2A;
std::string encrypt_key = "";

static void CreateFile(const std::string& source_code) {
    std::ofstream outfile(fz_name_file_v, std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(source_code.c_str(), source_code.size());
        outfile.close();
        std::cout << "文件" << fz_name_file_v << "创建成功。" << std::endl;
    }
    else {
        std::cerr << "文件" << fz_name_file_v << "打开/创建失败。" << std::endl;
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
                std::cerr << "错误：'-f' 参数后必须指定文件名\n";
                return 1;
            }
            filename = argv[i + 1];
            i++;
        }
        else if (in == "OutInfo=true") {
            isOutInfo = true;
        }
        else if (in == "pack=true") {
            v1 = 2;
            if (i + 3 >= argc) {
                std::cerr << "错误：pack=true 需要三个参数：输入文件名(不含扩展名) 输出文件名(不含扩展名) 密钥" << std::endl;
                return 1;
            }
            fz_name_file = argv[i + 1];
            fz_name_file += ".fox";
            fz_name_file_v = argv[i + 2];
            fz_name_file_v += ".fz";
            encrypt_key = argv[i + 3];
            i += 3;
        }
        else if (in == "pack=false") {
            v1 = 3;
            if (i + 3 >= argc) {
                std::cerr << "错误：pack=false 需要三个参数：输入文件名(不含扩展名) 输出文件名(不含扩展名) 密钥" << std::endl;
                return 1;
            }
            fz_name_file = argv[i + 1];
            fz_name_file += ".fz";
            fz_name_file_v = argv[i + 2];
            fz_name_file_v += ".fox";
            encrypt_key = argv[i + 3];
            i += 3;
        }
        else if (in == "-help" || in == "-h") {
            std::cout << "FoxLang 解释器使用说明:" << std::endl;
            std::cout << "  -f <filename>       指定要执行的 FoxLang 文件" << std::endl;
            std::cout << "  version             显示版本信息" << std::endl;
            std::cout << "  OutInfo=true        输出详细信息" << std::endl;
            std::cout << "  pack=true           将 .fox 文件加密为 .fz 文件" << std::endl;
            std::cout << "  pack=false          将 .fz 文件解密为 .fox 文件" << std::endl;
            return 0;
        }
        else {
            std::cerr << "错误：未知参数 '" << in << "'\n";
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
        interpreter.parseCode(fullCode);
        interpreter.runMainFunc();
    }

    return 0;
}
