/**
 * @file mutation_observer.h
 * @brief DOM MutationObserver API 实现
 *
 * 功能：
 * - 监听 DOM 树的变化
 * - 支持子节点、属性、文本内容变化的观察
 * - 支持 subtree 递归观察
 * - 异步批量回调
 *
 * 参考：https://dom.spec.whatwg.org/#interface-mutationobserver
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <set>

namespace mbink {

// 前向声明
class Node;
class Element;
class MutationObserver;

/**
 * @brief 变化记录
 *
 * 记录单个 DOM 变化的详细信息
 */
struct MutationRecord {
    /// 变化类型："childList", "attributes", "characterData"
    std::string type;

    /// 发生变化的目标节点
    std::weak_ptr<Node> target;

    /// 添加的节点列表（仅 childList 类型）
    std::vector<std::shared_ptr<Node>> added_nodes;

    /// 移除的节点列表（仅 childList 类型）
    std::vector<std::shared_ptr<Node>> removed_nodes;

    /// 前一个兄弟节点（仅 childList 类型）
    std::weak_ptr<Node> previous_sibling;

    /// 后一个兄弟节点（仅 childList 类型）
    std::weak_ptr<Node> next_sibling;

    /// 变化的属性名（仅 attributes 类型）
    std::string attribute_name;

    /// 属性命名空间（仅 attributes 类型）
    std::string attribute_namespace;

    /// 变化前的值（需要启用 oldValue 选项）
    std::string old_value;
};

/**
 * @brief 观察选项
 *
 * 配置 MutationObserver 要观察的变化类型
 */
struct MutationObserverInit {
    /// 观察子节点的添加和移除
    bool child_list = false;

    /// 观察属性变化
    bool attributes = false;

    /// 观察文本内容变化
    bool character_data = false;

    /// 观察目标节点的所有后代
    bool subtree = false;

    /// 记录属性变化前的值
    bool attribute_old_value = false;

    /// 记录文本变化前的值
    bool character_data_old_value = false;

    /// 只观察指定的属性（空表示观察所有属性）
    std::vector<std::string> attribute_filter;

    /**
     * @brief 检查选项是否有效
     * @return true 如果至少启用了一种观察类型
     */
    bool IsValid() const {
        return child_list || attributes || character_data;
    }
};

/**
 * @brief MutationObserver 回调类型
 */
using MutationCallback = std::function<void(
    const std::vector<MutationRecord>&,
    MutationObserver*)>;

/**
 * @brief DOM MutationObserver 类
 *
 * 提供监听 DOM 变化的能力，当观察的节点发生变化时，
 * 会异步调用回调函数并传递变化记录。
 */
class MutationObserver : public std::enable_shared_from_this<MutationObserver> {
public:
    /**
     * @brief 构造函数
     * @param callback 变化发生时调用的回调函数
     */
    explicit MutationObserver(MutationCallback callback);

    /**
     * @brief 析构函数
     */
    ~MutationObserver();

    // ========== 观察控制 ==========

    /**
     * @brief 开始观察目标节点
     * @param target 要观察的节点
     * @param options 观察选项
     * @throws TypeError 如果 target 为 null 或 options 无效
     */
    void Observe(std::shared_ptr<Node> target, const MutationObserverInit& options);

    /**
     * @brief 停止观察所有目标
     */
    void Disconnect();

    /**
     * @brief 获取并清空待处理的变化记录
     * @return 待处理的变化记录列表
     */
    std::vector<MutationRecord> TakeRecords();

    // ========== 内部接口（由 DOM 操作调用）==========

    /**
     * @brief 通知节点添加
     * @param node 添加的节点
     * @param parent 父节点
     * @param previous_sibling 前一个兄弟节点
     * @param next_sibling 后一个兄弟节点
     */
    void NotifyNodeAdded(
        std::shared_ptr<Node> node,
        std::shared_ptr<Node> parent,
        std::shared_ptr<Node> previous_sibling,
        std::shared_ptr<Node> next_sibling);

    /**
     * @brief 通知节点移除
     * @param node 移除的节点
     * @param parent 原父节点
     * @param previous_sibling 前一个兄弟节点
     * @param next_sibling 后一个兄弟节点
     */
    void NotifyNodeRemoved(
        std::shared_ptr<Node> node,
        std::shared_ptr<Node> parent,
        std::shared_ptr<Node> previous_sibling,
        std::shared_ptr<Node> next_sibling);

    /**
     * @brief 通知属性变化
     * @param element 发生变化的元素
     * @param name 属性名
     * @param old_value 变化前的值
     * @param new_value 变化后的值
     * @param namespace_uri 属性命名空间
     */
    void NotifyAttributeChanged(
        std::shared_ptr<Element> element,
        const std::string& name,
        const std::string& old_value,
        const std::string& new_value,
        const std::string& namespace_uri = "");

    /**
     * @brief 通知文本内容变化
     * @param node 发生变化的文本节点
     * @param old_text 变化前的文本
     * @param new_text 变化后的文本
     */
    void NotifyTextChanged(
        std::shared_ptr<Node> node,
        const std::string& old_text,
        const std::string& new_text);

private:
    /**
     * @brief 观察目标信息
     */
    struct ObservationTarget {
        std::weak_ptr<Node> target;
        MutationObserverInit options;
    };

    /**
     * @brief 检查节点是否在观察范围内
     * @param node 要检查的节点
     * @param type 变化类型
     * @param attribute_name 属性名（仅 attributes 类型）
     * @return 匹配的观察选项，如果不在范围内返回 nullptr
     */
    const MutationObserverInit* ShouldObserve(
        std::shared_ptr<Node> node,
        const std::string& type,
        const std::string& attribute_name = "") const;

    /**
     * @brief 检查节点是否是观察目标的后代
     * @param node 要检查的节点
     * @param target 观察目标
     * @return true 如果是后代
     */
    bool IsDescendantOf(std::shared_ptr<Node> node, std::shared_ptr<Node> target) const;

    /**
     * @brief 调度回调执行
     */
    void ScheduleCallback();

    /**
     * @brief 执行回调并清空记录
     */
    void FlushRecords();

    /**
     * @brief 添加变化记录
     * @param record 变化记录
     */
    void QueueRecord(MutationRecord record);

private:
    /// 回调函数
    MutationCallback callback_;

    /// 待处理的变化记录
    std::vector<MutationRecord> pending_records_;

    /// 观察目标列表
    std::vector<ObservationTarget> targets_;

    /// 是否已调度回调
    bool is_scheduled_ = false;
};

/**
 * @brief 全局 MutationObserver 注册表
 *
 * 管理所有活动的 MutationObserver，用于在 DOM 变化时通知它们
 */
class MutationObserverRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static MutationObserverRegistry& Instance();

    /**
     * @brief 注册观察者
     * @param observer 要注册的观察者
     */
    void Register(std::shared_ptr<MutationObserver> observer);

    /**
     * @brief 注销观察者
     * @param observer 要注销的观察者
     */
    void Unregister(MutationObserver* observer);

    /**
     * @brief 通知所有观察者节点添加
     */
    void NotifyNodeAdded(
        std::shared_ptr<Node> node,
        std::shared_ptr<Node> parent,
        std::shared_ptr<Node> previous_sibling,
        std::shared_ptr<Node> next_sibling);

    /**
     * @brief 通知所有观察者节点移除
     */
    void NotifyNodeRemoved(
        std::shared_ptr<Node> node,
        std::shared_ptr<Node> parent,
        std::shared_ptr<Node> previous_sibling,
        std::shared_ptr<Node> next_sibling);

    /**
     * @brief 通知所有观察者属性变化
     */
    void NotifyAttributeChanged(
        std::shared_ptr<Element> element,
        const std::string& name,
        const std::string& old_value,
        const std::string& new_value,
        const std::string& namespace_uri = "");

    /**
     * @brief 通知所有观察者文本变化
     */
    void NotifyTextChanged(
        std::shared_ptr<Node> node,
        const std::string& old_text,
        const std::string& new_text);

private:
    MutationObserverRegistry() = default;

    /// 活动的观察者列表
    std::set<std::shared_ptr<MutationObserver>> observers_;
};

} // namespace mbink
