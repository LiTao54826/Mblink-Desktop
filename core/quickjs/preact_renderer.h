/**
 * @file preact_renderer.h
 * @brief Preact渲染器 - Virtual DOM到MBink DOM的映射
 * 
 * 功能：
 * - 将Preact Virtual DOM渲染为MBink DOM
 * - 处理组件更新和diff算法
 * - 管理组件生命周期和Hooks
 */

#pragma once

#include "quickjs_runtime.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace lightui {

/**
 * @brief Preact渲染器类
 * 
 * 负责将Preact的Virtual DOM转换为MBink的真实DOM
 */
class PreactRenderer {
public:
    /**
     * @brief 构造函数
     * @param runtime QuickJS运行时
     * @param document 目标文档
     */
    PreactRenderer(QuickJSRuntime* runtime, std::shared_ptr<Document> document);
    
    /**
     * @brief 析构函数
     */
    ~PreactRenderer();
    
    /**
     * @brief 渲染VNode到容器
     * @param vnode_val VNode的JSValue
     * @param container_val 容器元素的JSValue
     * @return 是否成功
     */
    bool Render(JSValue vnode_val, JSValue container_val);
    
    /**
     * @brief 从VNode创建DOM元素
     * @param vnode_val VNode的JSValue
     * @return DOM元素
     */
    std::shared_ptr<Node> CreateDOMFromVNode(JSValue vnode_val);
    
    /**
     * @brief 更新现有DOM元素
     * @param element 现有DOM元素
     * @param vnode_val 新的VNode
     */
    void UpdateElement(std::shared_ptr<Element> element, JSValue vnode_val);
    
    /**
     * @brief 应用VNode的属性到DOM元素
     * @param element DOM元素
     * @param props_val 属性对象
     */
    void ApplyProps(std::shared_ptr<Element> element, JSValue props_val);
    
    /**
     * @brief 应用VNode的子节点到DOM元素
     * @param element DOM元素
     * @param children_val 子节点数组
     */
    void ApplyChildren(std::shared_ptr<Element> element, JSValue children_val);
    
    /**
     * @brief 渲染函数组件
     * @param component_func 组件函数
     * @param props_val 属性对象
     * @return 渲染结果VNode
     */
    JSValue RenderComponent(JSValue component_func, JSValue props_val);
    
    /**
     * @brief 设置当前组件上下文（用于Hooks）
     * @param component 组件对象
     */
    void SetCurrentComponent(JSValue component);
    
    /**
     * @brief 触发组件重新渲染
     * @param component 组件对象
     */
    void TriggerRerender(JSValue component);
    
    /**
     * @brief 获取QuickJS上下文
     */
    JSContext* GetContext() const { return ctx_; }
    
    /**
     * @brief 获取文档
     */
    std::shared_ptr<Document> GetDocument() const { return document_; }

private:
    /**
     * @brief 从JSValue获取字符串
     */
    std::string GetString(JSValue val);
    
    /**
     * @brief 从JSValue获取数字
     */
    double GetNumber(JSValue val);
    
    /**
     * @brief 从JSValue获取布尔值
     */
    bool GetBool(JSValue val);
    
    /**
     * @brief 检查JSValue是否为数组
     */
    bool IsArray(JSValue val);
    
    /**
     * @brief 获取数组长度
     */
    int GetArrayLength(JSValue val);
    
    /**
     * @brief 获取数组元素
     */
    JSValue GetArrayElement(JSValue val, int index);
    
    /**
     * @brief 获取对象属性
     */
    JSValue GetProperty(JSValue val, const char* name);
    
    /**
     * @brief 设置对象属性
     */
    void SetProperty(JSValue val, const char* name, JSValue prop_val);
    
    /**
     * @brief 检查是否为VNode
     */
    bool IsVNode(JSValue val);
    
    /**
     * @brief 检查是否为函数组件
     */
    bool IsComponentFunction(JSValue val);
    
    /**
     * @brief 创建文本节点
     */
    std::shared_ptr<Node> CreateTextNode(const std::string& text);
    
    /**
     * @brief 创建元素节点
     */
    std::shared_ptr<Element> CreateElementNode(const std::string& tag_name);
    
    /**
     * @brief 添加事件监听器
     */
    void AddEventListener(std::shared_ptr<Element> element, 
                          const std::string& event_name, 
                          JSValue handler);
    
    /**
     * @brief 清理组件资源
     */
    void CleanupComponent(JSValue component);

private:
    QuickJSRuntime* runtime_;           ///< QuickJS运行时
    JSContext* ctx_;                    ///< QuickJS上下文
    std::shared_ptr<Document> document_; ///< 目标文档
    
    // 组件管理
    JSValue current_component_;         ///< 当前渲染的组件
    int current_hook_index_;            ///< 当前Hook索引

    // 事件处理器映射（防止被GC回收）
    std::unordered_map<std::shared_ptr<Element>, std::vector<JSValue>> event_handlers_;
};

/**
 * @brief 注册Preact渲染器到QuickJS
 * @param runtime QuickJS运行时
 * @param document 文档对象
 */
void RegisterPreactRenderer(QuickJSRuntime* runtime, std::shared_ptr<Document> document);

} // namespace lightui

