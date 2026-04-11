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
    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        JSValueEntry old_entry = it->second;
        node_to_js_map_.erase(it);
        JS_FreeValue(old_entry.ctx, old_entry.value);
    }

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

size_t DOMBindingMap::Size() const {
    return node_to_js_map_.size();
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
