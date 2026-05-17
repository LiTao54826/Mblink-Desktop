/**
 * @file dom_binding_map.cpp
 * @brief DOM 绑定映射表实现
 */

#include "dom_binding_map.h"
#include <vector>

namespace mbink {

namespace {
bool g_dom_binding_map_clearing = false;
}

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

JSValue DOMBindingMap::GetJSValueWithContext(Node* node, JSContext** ctx_out) const {
    if (ctx_out) {
        *ctx_out = nullptr;
    }

    auto it = node_to_js_map_.find(node);
    if (it != node_to_js_map_.end()) {
        if (ctx_out) {
            *ctx_out = it->second.ctx;
        }
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
    if (g_dom_binding_map_clearing) {
        return;
    }

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

    g_dom_binding_map_clearing = true;
    for (auto& pair : entries) {
        JS_FreeValue(pair.second.ctx, pair.second.value);
    }
    g_dom_binding_map_clearing = false;
}

size_t DOMBindingMap::Size() const {
    return node_to_js_map_.size();
}

void DOMBindingMap::ForEach(const std::function<void(Node*, JSContext*, JSValueConst)>& visitor) const {
    if (!visitor) {
        return;
    }

    std::vector<std::pair<Node*, JSValueEntry>> entries;
    entries.reserve(node_to_js_map_.size());
    for (const auto& [node, entry] : node_to_js_map_) {
        JSValueEntry snapshot_entry;
        snapshot_entry.ctx = entry.ctx;
        snapshot_entry.value = entry.ctx ? JS_DupValue(entry.ctx, entry.value) : JS_UNDEFINED;
        entries.emplace_back(node, snapshot_entry);
    }

    for (const auto& [node, entry] : entries) {
        visitor(node, entry.ctx, entry.value);
    }

    for (auto& [_, entry] : entries) {
        if (entry.ctx) {
            JS_FreeValue(entry.ctx, entry.value);
        }
    }
}

DOMBindingMap::~DOMBindingMap() {
    Clear();
}

} // namespace mbink
