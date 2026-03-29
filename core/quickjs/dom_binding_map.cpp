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
    // 如果已经存在，先释放旧值
    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        JS_FreeValue(it->second.ctx, it->second.value);
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
        JS_FreeValue(it->second.ctx, it->second.value);
        node_to_js_map_.erase(it);
    }
}

bool DOMBindingMap::Has(Node* node) const {
    return node_to_js_map_.find(node) != node_to_js_map_.end();
}

void DOMBindingMap::Clear() {
    for (auto& pair : node_to_js_map_) {
        JS_FreeValue(pair.second.ctx, pair.second.value);
    }
    node_to_js_map_.clear();
}

DOMBindingMap::~DOMBindingMap() {
    Clear();
}

} // namespace mbink
