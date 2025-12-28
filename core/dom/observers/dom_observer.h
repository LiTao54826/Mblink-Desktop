/**
 * @file dom_observer.h
 * @brief DOM 观察者接口
 * 
 * 功能：
 * - 监听 DOM 树的变化
 * - 通知观察者节点的添加、删除、属性变化等
 * - 支持多个观察者
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lightui {

// 前向声明
class Node;
class Element;

/**
 * @brief DOM 观察者接口
 * 
 * 实现此接口以接收 DOM 变化通知
 */
class DOMObserver {
public:
    virtual ~DOMObserver() = default;
    
    /**
     * @brief 节点被添加时调用
     * @param node 被添加的节点
     * @param parent 父节点
     */
    virtual void OnNodeAdded(Node* node, Node* parent) {}
    
    /**
     * @brief 节点被移除时调用
     * @param node 被移除的节点
     * @param parent 原父节点
     */
    virtual void OnNodeRemoved(Node* node, Node* parent) {}
    
    /**
     * @brief 元素属性被修改时调用
     * @param element 元素
     * @param name 属性名
     * @param old_value 旧值
     * @param new_value 新值
     */
    virtual void OnAttributeChanged(Element* element, 
                                   const std::string& name,
                                   const std::string& old_value,
                                   const std::string& new_value) {}
    
    /**
     * @brief 元素样式被修改时调用
     * @param element 元素
     * @param property 样式属性名
     * @param old_value 旧值
     * @param new_value 新值
     */
    virtual void OnStyleChanged(Element* element,
                               const std::string& property,
                               const std::string& old_value,
                               const std::string& new_value) {}
    
    /**
     * @brief 文本内容被修改时调用
     * @param node 节点
     * @param old_text 旧文本
     * @param new_text 新文本
     */
    virtual void OnTextChanged(Node* node,
                              const std::string& old_text,
                              const std::string& new_text) {}

    /**
     * @brief CSS伪类状态被修改时调用
     * @param element 元素
     * @param pseudo_class 伪类名称（如"hover", "active", "focus"）
     * @param activate true表示激活，false表示移除
     */
    virtual void OnPseudoClassChanged(std::shared_ptr<Element> element,
                                     const std::string& pseudo_class,
                                     bool activate) {}

    /**
     * @brief 子树被修改时调用（批量变化）
     * @param root 子树根节点
     */
    virtual void OnSubtreeModified(Node* root) {}
};

/**
 * @brief DOM 观察者管理器
 * 
 * 管理多个观察者，并分发 DOM 变化通知
 */
class DOMObserverManager {
public:
    /**
     * @brief 添加观察者
     * @param observer 观察者指针
     */
    void AddObserver(DOMObserver* observer);
    
    /**
     * @brief 移除观察者
     * @param observer 观察者指针
     */
    void RemoveObserver(DOMObserver* observer);
    
    /**
     * @brief 移除所有观察者
     */
    void RemoveAllObservers();
    
    /**
     * @brief 通知节点被添加
     */
    void NotifyNodeAdded(Node* node, Node* parent);
    
    /**
     * @brief 通知节点被移除
     */
    void NotifyNodeRemoved(Node* node, Node* parent);
    
    /**
     * @brief 通知属性被修改
     */
    void NotifyAttributeChanged(Element* element,
                               const std::string& name,
                               const std::string& old_value,
                               const std::string& new_value);
    
    /**
     * @brief 通知样式被修改
     */
    void NotifyStyleChanged(Element* element,
                           const std::string& property,
                           const std::string& old_value,
                           const std::string& new_value);
    
    /**
     * @brief 通知文本被修改
     */
    void NotifyTextChanged(Node* node,
                          const std::string& old_text,
                          const std::string& new_text);
    
    /**
     * @brief 通知子树被修改
     */
    void NotifySubtreeModified(Node* root);

    /**
     * @brief 通知伪类状态变化
     */
    void NotifyPseudoClassChanged(std::shared_ptr<Element> element,
                                 const std::string& pseudo_class,
                                 bool activate);

    /**
     * @brief 获取观察者数量
     */
    size_t GetObserverCount() const { return observers_.size(); }

private:
    std::vector<DOMObserver*> observers_;
};

} // namespace lightui

