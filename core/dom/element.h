/**
 * @file element.h
 * @brief DOM元素类
 * 
 * 功能：
 * - 表示HTML元素（div, span, button等）
 * - 属性管理（setAttribute, getAttribute）
 * - 样式管理（style, className, classList）
 * - 事件监听器管理
 * - 查询选择器（querySelector, querySelectorAll）
 * 
 * 实现要点：
 * - 支持40+ DOM APIs
 * - 高效的属性和样式存储
 * - 事件监听器列表管理
 * - CSS选择器匹配
 */

#pragma once

#include "node.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>

// 前向声明Lexbor类型
struct lxb_dom_node;
typedef struct lxb_dom_node lxb_dom_node_t;

namespace lightui {

// 前向声明
class Event;
class DOMTokenList;
class CSSStyleDeclaration;
class DOMStringMap;
class Document;
using EventListener = std::function<void(std::shared_ptr<Event>)>;

// EventListener包装器，包含唯一ID、捕获阶段标志和once选项
struct EventListenerEntry {
    uint64_t id;                    // 唯一ID
    EventListener listener;         // 监听器函数
    bool use_capture;               // 是否在捕获阶段触发
    bool once;                      // 是否只执行一次（执行后自动移除）

    EventListenerEntry(uint64_t id_, EventListener listener_, bool use_capture_, bool once_ = false)
        : id(id_), listener(std::move(listener_)), use_capture(use_capture_), once(once_) {}
};

/**
 * @brief DOM元素类
 */
class Element : public Node {
public:
    /**
     * @brief 构造函数
     * @param tag_name 标签名（如"div", "span"）
     */
    explicit Element(const std::string& tag_name);
    
    /**
     * @brief 析构函数
     */
    ~Element() override = default;
    
    /**
     * @brief 获取标签名
     * @return 标签名
     */
    std::string GetTagName() const { return tag_name_; }
    
    // ========== 属性操作 ==========
    
    /**
     * @brief 设置属性
     * @param name 属性名
     * @param value 属性值
     */
    virtual void SetAttribute(const std::string& name, const std::string& value);
    
    /**
     * @brief 获取属性
     * @param name 属性名
     * @return 属性值，如果不存在返回空字符串
     */
    std::string GetAttribute(const std::string& name) const;
    
    /**
     * @brief 检查是否有指定属性
     * @param name 属性名
     * @return true表示存在
     */
    bool HasAttribute(const std::string& name) const;

    /**
     * @brief 获取所有属性
     * @return 属性名到属性值的映射
     */
    const std::unordered_map<std::string, std::string>& GetAllAttributes() const;
    
    /**
     * @brief 移除属性
     * @param name 属性名
     */
    void RemoveAttribute(const std::string& name);
    
    // ========== 样式操作 ==========
    
    /**
     * @brief 获取className
     * @return className字符串
     */
    std::string GetClassName() const;
    
    /**
     * @brief 设置className
     * @param class_name className字符串
     */
    void SetClassName(const std::string& class_name);
    
    /**
     * @brief 添加class
     * @param class_name class名称
     */
    void AddClass(const std::string& class_name);
    
    /**
     * @brief 移除class
     * @param class_name class名称
     */
    void RemoveClass(const std::string& class_name);
    
    /**
     * @brief 切换class
     * @param class_name class名称
     * @return true表示添加，false表示移除
     */
    bool ToggleClass(const std::string& class_name);
    
    /**
     * @brief 检查是否有指定class
     * @param class_name class名称
     * @return true表示存在
     */
    bool HasClass(const std::string& class_name) const;

    /**
     * @brief 获取classList对象
     * @return DOMTokenList对象
     *
     * 符合W3C DOMTokenList接口：
     * - add(token1, token2, ...) - 添加class
     * - remove(token1, token2, ...) - 移除class
     * - toggle(token, force?) - 切换class
     * - contains(token) - 检查是否包含class
     * - item(index) - 获取指定索引的class
     * - length - class数量
     *
     * 示例：
     * element->GetClassList()->Add("active");
     * element->GetClassList()->Toggle("hidden");
     */
    std::shared_ptr<DOMTokenList> GetClassList();

    /**
     * @brief 设置内联样式
     * @param property 样式属性名
     * @param value 样式值
     */
    void SetStyle(const std::string& property, const std::string& value);

    /**
     * @brief 获取内联样式
     * @param property 样式属性名
     * @return 样式值
     */
    std::string GetStyle(const std::string& property) const;

    /**
     * @brief 获取style对象
     * @return CSSStyleDeclaration对象
     *
     * 符合W3C CSSStyleDeclaration接口：
     * - setProperty(property, value, priority?) - 设置样式属性
     * - getPropertyValue(property) - 获取样式属性值
     * - removeProperty(property) - 移除样式属性
     * - getPropertyPriority(property) - 获取优先级（!important）
     * - cssText - 完整的样式文本
     * - length - 样式属性数量
     *
     * 示例：
     * element->GetStyleDeclaration()->SetProperty("color", "red");
     * element->GetStyleDeclaration()->SetCssText("color: red; font-size: 16px;");
     */
    std::shared_ptr<CSSStyleDeclaration> GetStyleDeclaration();

    /**
     * @brief 获取dataset对象
     * @return DOMStringMap对象
     *
     * 符合W3C DOMStringMap接口：
     * - Get(name) - 获取data-*属性值
     * - Set(name, value) - 设置data-*属性值
     * - Remove(name) - 删除data-*属性
     * - Has(name) - 检查是否存在data-*属性
     * - GetAll() - 获取所有data-*属性
     *
     * 命名转换：
     * - HTML: data-user-id
     * - JavaScript: userId
     *
     * 示例：
     * element->GetDataset()->Set("userId", "123");  // 设置data-user-id="123"
     * std::string id = element->GetDataset()->Get("userId");
     */
    std::shared_ptr<DOMStringMap> GetDataset();

    // ========== CSS伪类支持（参考RmlUi） ==========

    /**
     * @brief 设置或移除CSS伪类
     * @param pseudo_class 伪类名称（如"hover", "active", "focus"）
     * @param activate true表示设置，false表示移除
     *
     * 参考：RmlUi/Source/Core/Element.cpp - SetPseudoClass
     *
     * 支持的伪类：
     * - :hover - 鼠标悬停
     * - :active - 鼠标按下
     * - :focus - 获得焦点
     * - :focus-visible - 键盘导航焦点
     * - :drag - 拖拽中
     * - :disabled - 禁用状态
     * - :checked - 选中状态（checkbox/radio）
     */
    void SetPseudoClass(const std::string& pseudo_class, bool activate);

    /**
     * @brief 检查是否有指定伪类
     * @param pseudo_class 伪类名称
     * @return true表示存在
     */
    bool HasPseudoClass(const std::string& pseudo_class) const;

    /**
     * @brief 获取所有激活的伪类
     * @return 伪类名称列表
     */
    std::vector<std::string> GetActivePseudoClasses() const;
    
    // ========== 事件监听 ==========
    
    /**
     * @brief 添加事件监听器
     * @param type 事件类型（如"click", "mousemove"）
     * @param listener 监听器函数
     * @param use_capture 是否在捕获阶段触发（默认false，在冒泡阶段触发）
     * @param once 是否只执行一次（执行后自动移除，默认false）
     * @return 监听器ID，用于后续移除
     *
     * 符合W3C EventTarget接口：
     * - addEventListener(type, listener, {capture: false, once: false})
     *
     * 示例：
     * element->AddEventListener("click", [](auto e) { ... }, false, true);  // 只执行一次
     */
    uint64_t AddEventListener(const std::string& type, EventListener listener, bool use_capture = false, bool once = false);

    /**
     * @brief 移除事件监听器
     * @param type 事件类型
     * @param listener_id 监听器ID（由AddEventListener返回）
     * @return true表示成功移除，false表示未找到
     */
    bool RemoveEventListener(const std::string& type, uint64_t listener_id);
    
    /**
     * @brief 分发事件
     * @param event 事件对象
     * @return true表示事件未被取消
     */
    bool DispatchEvent(std::shared_ptr<Event> event);
    
    // ========== 查询选择器 ==========
    
    /**
     * @brief 查询第一个匹配的元素
     * @param selector CSS选择器
     * @return 匹配的元素，如果没有返回nullptr
     */
    std::shared_ptr<Element> QuerySelector(const std::string& selector);
    
    /**
     * @brief 查询所有匹配的元素
     * @param selector CSS选择器
     * @return 匹配的元素列表
     */
    std::vector<std::shared_ptr<Element>> QuerySelectorAll(const std::string& selector);
    
    /**
     * @brief 检查元素是否匹配选择器
     * @param selector CSS选择器
     * @return true表示匹配
     */
    bool Matches(const std::string& selector) const;
    
    /**
     * @brief 查找最近的匹配祖先元素
     * @param selector CSS选择器
     * @return 匹配的祖先元素，如果没有返回nullptr
     */
    std::shared_ptr<Element> Closest(const std::string& selector);

    // ========== 其他 ==========

    /**
     * @brief 克隆节点
     * @param deep 是否深度克隆
     * @return 克隆的节点
     */
    std::shared_ptr<Node> CloneNode(bool deep) override;
    
    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    std::string GetTextContent() const override;

    /**
     * @brief 设置文本内容
     * @param content 文本内容
     */
    void SetTextContent(const std::string& content) override;

    // ========== HTML内容操作 ==========

    /**
     * @brief 获取innerHTML（元素内部的HTML）
     * @return HTML字符串
     */
    std::string GetInnerHTML() const;

    /**
     * @brief 设置innerHTML（替换元素内部的所有内容）
     * @param html HTML字符串
     */
    void SetInnerHTML(const std::string& html);

    /**
     * @brief 获取outerHTML（包括元素自身的HTML）
     * @return HTML字符串
     */
    std::string GetOuterHTML() const;

    /**
     * @brief 设置outerHTML（替换元素自身）
     * @param html HTML字符串
     */
    void SetOuterHTML(const std::string& html);

    /**
     * @brief 将Lexbor节点转换为我们的Node对象
     * @param lexbor_node Lexbor节点
     * @param doc 所属文档
     * @return 转换后的Node对象
     */
    static std::shared_ptr<Node> ConvertLexborNodeToNode(lxb_dom_node_t* lexbor_node,
                                                          std::shared_ptr<Document> doc);

private:
    /**
     * @brief 处理事件（内部方法）
     * @param event 事件对象
     * @param use_capture 是否使用捕获
     */
    void HandleEvent(std::shared_ptr<Event> event, bool use_capture);

private:
    std::string tag_name_;
    std::unordered_map<std::string, std::string> attributes_;
    std::unordered_map<std::string, std::string> styles_;

    // 事件监听器存储（使用EventListenerEntry支持ID和捕获阶段）
    std::unordered_map<std::string, std::vector<EventListenerEntry>> event_listeners_;

    // 下一个监听器ID（静态，全局唯一）
    static uint64_t next_listener_id_;

    // CSS伪类状态（参考RmlUi设计）
    std::unordered_map<std::string, bool> pseudo_classes_;

    // classList对象（懒加载）
    mutable std::shared_ptr<DOMTokenList> class_list_;

    // style对象（懒加载）
    mutable std::shared_ptr<CSSStyleDeclaration> style_declaration_;

    // dataset对象（懒加载）
    mutable std::shared_ptr<DOMStringMap> dataset_;
};

} // namespace lightui

