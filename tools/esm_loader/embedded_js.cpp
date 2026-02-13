/**
 * @file embedded_js.cpp
 * @brief 嵌入式 JavaScript 资源实现
 * 
 * 此文件包含构建时生成的 JS 代码数据
 */

#include "embedded_js.h"

// 包含构建时生成的 JS 数据文件
#include "generated/preact_js.inc"
#include "generated/hooks_js.inc"
#include "generated/dom_polyfills_js.inc"

namespace lightui {
namespace embedded {

std::string_view GetPreactJS() {
    return std::string_view(reinterpret_cast<const char*>(preact_js_data), preact_js_size);
}

std::string_view GetHooksJS() {
    return std::string_view(reinterpret_cast<const char*>(hooks_js_data), hooks_js_size);
}

std::string_view GetDomPolyfillsJS() {
    return std::string_view(reinterpret_cast<const char*>(dom_polyfills_js_data), dom_polyfills_js_size);
}

bool HasEmbeddedJS() {
    return true;
}

}  // namespace embedded
}  // namespace lightui

