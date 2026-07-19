#include "utils.hpp"
#include "token.hpp"
#include "common.hpp"
#include "lexer.hpp"   
#include "error_reporter.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

std::string read_file(const std::string& filename) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) {
        ErrorReporter::reportSimple("FileError", "Cannot open file: " + filename);
        std::exit(1);
    }
    std::string fullCode((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    if (isOutInfo) {
        std::cout << "成功读取文件：" << filename << "，内容长度：" << fullCode.size() << " 字节" << std::endl;
    }
    return fullCode;
}

// 单字符密钥的异或加密/解密
std::string xor_encrypt_decrypt(const std::string& input, char key) {
    std::string output = input;
    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ key; 
    }
    return output;
}

// 字符串密钥的异或加密/解密
std::string xor_encrypt_decrypt_str_key(const std::string& input, const std::string& key) {
    std::string output = input;
    size_t key_len = key.size();
    if (key_len == 0) return input; 

    for (size_t i = 0; i < input.size(); ++i) {
        output[i] = input[i] ^ key[i % key_len]; 
    }
    return output;
}

// 使用字符串密钥加密文件
void encrypt_file_with_key(const std::string& src_path, const std::string& dst_path, const std::string& key) {
    std::ifstream src_file(src_path, std::ios::binary);
    if (!src_file) {
        ErrorReporter::reportSimple("FileError", "Cannot open source file: " + src_path);
        return;
    }
    std::string content((std::istreambuf_iterator<char>(src_file)), std::istreambuf_iterator<char>());
    src_file.close();

    std::string encrypted = xor_encrypt_decrypt_str_key(content, key);

    std::ofstream dst_file(dst_path, std::ios::binary);
    if (!dst_file) {
        ErrorReporter::reportSimple("FileError", "Cannot create encrypted file: " + dst_path);
        return;
    }
    dst_file.write(encrypted.c_str(), encrypted.size());
    dst_file.close();
    std::cout << "加密输出文件：" << dst_path << " (密钥: " << key << ")" << std::endl;
}

// 使用字符串密钥解密文件
std::string decrypt_file_with_key(const std::string& fz_path, const std::string& key) {
    std::ifstream fz_file(fz_path, std::ios::binary);
    if (!fz_file) {
        ErrorReporter::reportSimple("FileError", "Cannot open encrypted file: " + fz_path);
        return "";
    }
    std::string encrypted((std::istreambuf_iterator<char>(fz_file)), std::istreambuf_iterator<char>());
    fz_file.close();
    return xor_encrypt_decrypt_str_key(encrypted, key);
}

// 加密文件
void encrypt_file(const std::string& src_path, const std::string& dst_path, char key) {
    std::ifstream src_file(src_path, std::ios::binary);
    if (!src_file) {
        ErrorReporter::reportSimple("FileError", "Cannot open source file: " + src_path);
        return;
    }
    std::string content((std::istreambuf_iterator<char>(src_file)), std::istreambuf_iterator<char>());
    src_file.close();
    std::string encrypted = xor_encrypt_decrypt(content, key);
    std::ofstream dst_file(dst_path, std::ios::binary);
    if (!dst_file) {
        ErrorReporter::reportSimple("FileError", "Cannot create encrypted file: " + dst_path);
        return;
    }
    dst_file.write(encrypted.c_str(), encrypted.size());
    dst_file.close();

    std::cout << "输出文件：" << dst_path << std::endl;
}

// 解密文件
std::string decrypt_file(const std::string& fz_path, char key) {
    std::ifstream fz_file(fz_path, std::ios::binary);
    if (!fz_file) {
        ErrorReporter::reportSimple("FileError", "Cannot open encrypted file: " + fz_path);
        return "";
    }
    std::string encrypted((std::istreambuf_iterator<char>(fz_file)), std::istreambuf_iterator<char>());
    fz_file.close();
    return xor_encrypt_decrypt(encrypted, key);
}

