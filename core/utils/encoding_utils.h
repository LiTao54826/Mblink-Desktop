/**
 * @file encoding_utils.h
 * @brief 编码转换工具函数
 */

#pragma once

#include <string>

namespace lightui {
namespace utils {

/**
 * @brief 将系统本地编码String转换为UTF-8编码
 * @param local_str 本地编码的字符串（Windows下为GBK）
 * @return UTF-8编码的字符串
 * 
 * 在Windows上，C++源代码默认使用GBK编码，而SDL需要UTF-8编码
 * 此函数用于在设置窗口标题等场景下进行编码转换
 */
std::string LocalToUTF8(const std::string& local_str);

/**
 * @brief 将UTF-8编码字符串转换为系统本地编码
 * @param utf8_str UTF-8编码的字符串
 * @return 本地编码的字符串（Windows下为GBK）
 */
std::string UTF8ToLocal(const std::string& utf8_str);

}  // namespace utils
}  // namespace lightui
