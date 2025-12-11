/**
 * @file encoding_utils.cpp
 * @brief 编码转换工具函数实现
 */

#include "encoding_utils.h"

#ifdef _WIN32
    #include <windows.h>
#endif

namespace lightui {
namespace utils {

std::string LocalToUTF8(const std::string& local_str) {
#ifdef _WIN32
    if (local_str.empty()) {
        return "";
    }

    // 步骤1: 将GBK转换为宽字符(UTF-16)
    int wlen = MultiByteToWideChar(CP_ACP, 0, local_str.c_str(), -1, nullptr, 0);
    if (wlen == 0) {
        return local_str;  // 转换失败，返回原字符串
    }

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, local_str.c_str(), -1, &wstr[0], wlen);

    // 步骤2: 将宽字符(UTF-16)转换为UTF-8
    int utf8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8len == 0) {
        return local_str;  // 转换失败，返回原字符串
    }

    std::string utf8_str(utf8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8len, nullptr, nullptr);

    // 移除末尾的null字符
    if (!utf8_str.empty() && utf8_str.back() == '\0') {
        utf8_str.pop_back();
    }

    return utf8_str;
#else
    // 非Windows平台，假设已经是UTF-8
    return local_str;
#endif
}

std::string UTF8ToLocal(const std::string& utf8_str) {
#ifdef _WIN32
    if (utf8_str.empty()) {
        return "";
    }

    // 步骤1: 将UTF-8转换为宽字符(UTF-16)
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wlen == 0) {
        return utf8_str;  // 转换失败，返回原字符串
    }

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], wlen);

    // 步骤2: 将宽字符(UTF-16)转换为GBK
    int local_len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (local_len == 0) {
        return utf8_str;  // 转换失败，返回原字符串
    }

    std::string local_str(local_len, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &local_str[0], local_len, nullptr, nullptr);

    // 移除末尾的null字符
    if (!local_str.empty() && local_str.back() == '\0') {
        local_str.pop_back();
    }

    return local_str;
#else
    // 非Windows平台，假设已经是UTF-8
    return utf8_str;
#endif
}

}  // namespace utils
}  // namespace lightui
