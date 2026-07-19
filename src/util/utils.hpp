#pragma once
#include <string>
#include <vector>

// 读取文件
std::string read_file(const std::string& filename);

// 原有的单字符加密（保留兼容）
std::string xor_encrypt_decrypt(const std::string& input, char key);
std::string xor_encrypt_decrypt_str_key(const std::string& input, const std::string& key);

// 使用字符串密钥的文件加密解密
void encrypt_file_with_key(const std::string& src_path, const std::string& dst_path, const std::string& key);
std::string decrypt_file_with_key(const std::string& fz_path, const std::string& key);

// 旧的单字符函数
void encrypt_file(const std::string& src_path, const std::string& dst_path, char key);
std::string decrypt_file(const std::string& fz_path, char key);