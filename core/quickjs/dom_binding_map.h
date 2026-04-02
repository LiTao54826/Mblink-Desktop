/**
 * @file dom_binding_map.h
 * @brief DOM 绑定映射表 - 确保 C++ Node 与 JS 对象的引用一致性
 * 
 * 功能：
 * - 维护 C++ Node* 到 JSValue 的全局映射
 * - 确保同一个 C++ Node 始终对应同一个 JS 对象
 * - 支持 Node 生命周期管理
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include "quickjs.h"
#include "core/dom/node.h"

namespace mbink {

/**
 * @brief DOM 绑定映射表（单例）
 * 
 * 确保引用相等性：
 * - 同一个 C++ Node* 总是映射到同一个 JSValue
 * - JS 侧的对象 === 比较能够正常工作
 * - 避免重复包装造成的内存浪费
 */
class DOMBindingMap {
public:
    /**
     * @brief 获取单例实例
     */
    static DOMBindingMap& GetInstance();

    /**
     * @brief 获取 Node 对应的 JSValue
     * @param node C++ Node 指针
     * @return JSValue，如果不存在返回 JS_UNDEFINED
     */
    JSValue GetJSValue(Node* node) const;

    /**
     * @brief 设置 Node 到 JSValue 的映射
     * @param node C++ Node 指针
     * @param value JSValue（会被 DupValue）
     * @param ctx QuickJS 上下文
     */
    void SetJSValue(Node* node, JSValue value, JSContext* ctx);

    /**
     * @brief 移除 Node 的映射
     * @param node C++ Node 指针
     */
    void Remove(Node* node);

    /**
     * @brief 检查 Node 是否已有映射
     * @param node C++ Node 指针
     * @return true 表示已有映射
     */
    bool Has(Node* node) const;

    /**
     * @brief 清空所有映射
     */
    void Clear();

    /**
     * @brief 遍历所有映射项
     * @param visitor 访问回调，参数为 Node* / JSContext* / JSValue
     */
    void ForEach(const std::function<void(Node*, JSContext*, JSValueConst)>& visitor) const;

private:
    DOMBindingMap() = default;
    ~DOMBindingMap();

    // 禁止拷贝和移动
    DOMBindingMap(const DOMBindingMap&) = delete;
    DOMBindingMap& operator=(const DOMBindingMap&) = delete;

    struct JSValueEntry {
        JSValue value;
        JSContext* ctx;
    };

    std::unordered_map<Node*, JSValueEntry> node_to_js_map_;
};

} // namespace mbink
