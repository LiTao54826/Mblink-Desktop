/**
 * @file embedded_js.h
 * @brief 嵌入式 JavaScript 资源管理
 * 
 * 提供编译时嵌入的 JS 库（Preact、Hooks、Polyfills 等）
 * 这样生成的 exe 可以独立运行，不需要外部 JS 文件
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace mbink {
namespace embedded {

std::string_view GetEmbeddedJS(std::string_view path);
std::vector<std::string_view> ListEmbeddedJSPaths();

/**
 * @brief 获取嵌入的 Preact 库代码
 */
std::string_view GetPreactJS();

/**
 * @brief 获取嵌入的 Hooks 库代码
 */
std::string_view GetHooksJS();

/**
 * @brief 获取嵌入的 DOM Polyfills 代码
 */
std::string_view GetDomPolyfillsJS();

/**
 * @brief 获取嵌入的 runtime bootstrap 代码
 */
std::string_view GetBootstrapJS();

/**
 * @brief 检查是否有嵌入的 JS 资源
 */
bool HasEmbeddedJS();

}  // namespace embedded
}  // namespace mbink

