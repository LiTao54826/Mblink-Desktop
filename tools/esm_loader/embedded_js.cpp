/**
 * @file embedded_js.cpp
 * @brief 嵌入式 JavaScript 资源实现
 *
 * 此文件包含构建时生成的 JS 代码数据
 */

#include "embedded_js.h"

namespace mblink {
namespace embedded {
namespace {

struct EmbeddedJSEntry {
    const char* path;
    const unsigned char* data;
    size_t size;
};

#include "generated/embedded_js_registry.inc"

}  // namespace

std::string_view GetEmbeddedJS(std::string_view path) {
    for (const auto& entry : kEmbeddedJSEntries) {
        if (path == entry.path) {
            return std::string_view(reinterpret_cast<const char*>(entry.data), entry.size);
        }
    }
    return {};
}

std::vector<std::string_view> ListEmbeddedJSPaths() {
    std::vector<std::string_view> paths;
    paths.reserve(sizeof(kEmbeddedJSEntries) / sizeof(kEmbeddedJSEntries[0]));
    for (const auto& entry : kEmbeddedJSEntries) {
        paths.emplace_back(entry.path);
    }
    return paths;
}

std::string_view GetDomPolyfillsJS() {
    return GetEmbeddedJS("polyfills/dom.js");
}

std::string_view GetBootstrapJS() {
    return GetEmbeddedJS("runtime/bootstrap.js");
}

bool HasEmbeddedJS() {
    return sizeof(kEmbeddedJSEntries) / sizeof(kEmbeddedJSEntries[0]) > 0;
}

}  // namespace embedded
}  // namespace mblink

