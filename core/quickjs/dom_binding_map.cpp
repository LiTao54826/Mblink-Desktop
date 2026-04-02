/**
 * @file dom_binding_map.cpp
 * @brief DOM 绑定映射表实现
 */

#include "dom_binding_map.h"

namespace mbink {

DOMBindingMap& DOMBindingMap::GetInstance() {
    static DOMBindingMap instance;
    return instance;
}

JSValue DOMBindingMap::GetJSValue(Node* node) const {
    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        return it->second.value;
    }
    return JS_UNDEFINED;
}

void DOMBindingMap::SetJSValue(Node* node, JSValue value, JSContext* ctx) {
    JSValueEntry old_entry{};
    bool has_old_entry = false;

    // 如果已经存在，先从映射表移除旧值，避免 JS_FreeValue 触发 finalizer 时误删新映射
    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        old_entry = it->second;
        node_to_js_map_.erase(it);
        has_old_entry = true;
    }

    if (has_old_entry) {
        JS_FreeValue(old_entry.ctx, old_entry.value);
    }

    // 保存新值（增加引用计数）
    JSValueEntry entry;
    entry.value = JS_DupValue(ctx, value);
    entry.ctx = ctx;
    node_to_js_map_[node] = entry;
}

void DOMBindingMap::Remove(Node* node) {
    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        JSValueEntry entry = it->second;
        node_to_js_map_.erase(it);
        JS_FreeValue(entry.ctx, entry.value);
    }
}

bool DOMBindingMap::Has(Node* node) const {
    return node_to_js_map_.find(node) != node_to_js_map_.end();
}

void DOMBindingMap::Clear() {
    auto entries = std::move(node_to_js_map_);
    node_to_js_map_.clear();

    for (auto& pair : entries) {
        JS_FreeValue(pair.second.ctx, pair.second.value);
    }
}

void DOMBindingMap::ForEach(const std::function<void(Node*, JSContext*, JSValueConst)>& visitor) const {
    if (!visitor) {
        return;
    }

    for (const auto& [node, entry] : node_to_js_map_) {
        visitor(node, entry.ctx, entry.value);
    }
}

DOMBindingMap::~DOMBindingMap() {
    Clear();
}

} // namespace mbink
