/**
 * @file mutation_observer.cpp
 * @brief DOM MutationObserver API 实现
 */

#include "mutation_observer.h"
#include "node.h"
#include "element.h"
#include "core/event/loop/task_scheduler.h"
#include <algorithm>
#include <stdexcept>

namespace lightui {

// ========== MutationObserver 实现 ==========

MutationObserver::MutationObserver(MutationCallback callback)
    : callback_(std::move(callback)) {
}

MutationObserver::~MutationObserver() {
    Disconnect();
}

void MutationObserver::Observe(std::shared_ptr<Node> target, const MutationObserverInit& options) {
    if (!target) {
        throw std::invalid_argument("Target cannot be null");
    }

    if (!options.IsValid()) {
        throw std::invalid_argument("At least one of childList, attributes, or characterData must be true");
    }

    // 检查是否已经在观察这个目标
    for (auto& obs : targets_) {
        auto existing_target = obs.target.lock();
        if (existing_target == target) {
            // 更新选项
            obs.options = options;
            return;
        }
    }

    // 添加新的观察目标
    ObservationTarget obs;
    obs.target = target;
    obs.options = options;
    targets_.push_back(obs);

    // 注册到全局注册表
    MutationObserverRegistry::Instance().Register(shared_from_this());
}

void MutationObserver::Disconnect() {
    targets_.clear();
    pending_records_.clear();
    is_scheduled_ = false;

    // 从全局注册表注销
    MutationObserverRegistry::Instance().Unregister(this);
}

std::vector<MutationRecord> MutationObserver::TakeRecords() {
    std::vector<MutationRecord> records = std::move(pending_records_);
    pending_records_.clear();
    is_scheduled_ = false;
    return records;
}

void MutationObserver::NotifyNodeAdded(
    std::shared_ptr<Node> node,
    std::shared_ptr<Node> parent,
    std::shared_ptr<Node> previous_sibling,
    std::shared_ptr<Node> next_sibling) {

    const MutationObserverInit* options = ShouldObserve(parent, "childList");
    if (!options || !options->child_list) {
        return;
    }

    MutationRecord record;
    record.type = "childList";
    record.target = parent;
    record.added_nodes.push_back(node);
    record.previous_sibling = previous_sibling;
    record.next_sibling = next_sibling;

    QueueRecord(std::move(record));
}

void MutationObserver::NotifyNodeRemoved(
    std::shared_ptr<Node> node,
    std::shared_ptr<Node> parent,
    std::shared_ptr<Node> previous_sibling,
    std::shared_ptr<Node> next_sibling) {

    const MutationObserverInit* options = ShouldObserve(parent, "childList");
    if (!options || !options->child_list) {
        return;
    }

    MutationRecord record;
    record.type = "childList";
    record.target = parent;
    record.removed_nodes.push_back(node);
    record.previous_sibling = previous_sibling;
    record.next_sibling = next_sibling;

    QueueRecord(std::move(record));
}

void MutationObserver::NotifyAttributeChanged(
    std::shared_ptr<Element> element,
    const std::string& name,
    const std::string& old_value,
    const std::string& new_value,
    const std::string& namespace_uri) {

    const MutationObserverInit* options = ShouldObserve(element, "attributes", name);
    if (!options || !options->attributes) {
        return;
    }

    MutationRecord record;
    record.type = "attributes";
    record.target = element;
    record.attribute_name = name;
    record.attribute_namespace = namespace_uri;

    if (options->attribute_old_value) {
        record.old_value = old_value;
    }

    QueueRecord(std::move(record));
}

void MutationObserver::NotifyTextChanged(
    std::shared_ptr<Node> node,
    const std::string& old_text,
    const std::string& new_text) {

    const MutationObserverInit* options = ShouldObserve(node, "characterData");
    if (!options || !options->character_data) {
        return;
    }

    MutationRecord record;
    record.type = "characterData";
    record.target = node;

    if (options->character_data_old_value) {
        record.old_value = old_text;
    }

    QueueRecord(std::move(record));
}

const MutationObserverInit* MutationObserver::ShouldObserve(
    std::shared_ptr<Node> node,
    const std::string& type,
    const std::string& attribute_name) const {

    if (!node) {
        return nullptr;
    }

    for (const auto& obs : targets_) {
        auto target = obs.target.lock();
        if (!target) {
            continue;
        }

        // 检查是否是目标节点本身
        bool is_target = (node == target);

        // 检查是否是目标的后代（如果启用了 subtree）
        bool is_descendant = obs.options.subtree && IsDescendantOf(node, target);

        if (!is_target && !is_descendant) {
            continue;
        }

        // 检查变化类型
        if (type == "childList" && obs.options.child_list) {
            return &obs.options;
        }

        if (type == "attributes" && obs.options.attributes) {
            // 检查属性过滤器
            if (!obs.options.attribute_filter.empty()) {
                auto it = std::find(
                    obs.options.attribute_filter.begin(),
                    obs.options.attribute_filter.end(),
                    attribute_name);
                if (it == obs.options.attribute_filter.end()) {
                    continue;
                }
            }
            return &obs.options;
        }

        if (type == "characterData" && obs.options.character_data) {
            return &obs.options;
        }
    }

    return nullptr;
}

bool MutationObserver::IsDescendantOf(std::shared_ptr<Node> node, std::shared_ptr<Node> target) const {
    if (!node || !target) {
        return false;
    }

    auto current = node->GetParentNode();
    while (current) {
        if (current == target) {
            return true;
        }
        current = current->GetParentNode();
    }

    return false;
}

void MutationObserver::ScheduleCallback() {
    if (is_scheduled_) {
        return;
    }

    is_scheduled_ = true;

    // 使用微任务调度回调
    // 这确保回调在当前脚本执行完成后异步执行
    auto self = shared_from_this();
    TaskScheduler::Instance().PostMicrotask([self]() {
        self->FlushRecords();
    });
}

void MutationObserver::FlushRecords() {
    if (pending_records_.empty()) {
        is_scheduled_ = false;
        return;
    }

    // 取出所有记录
    std::vector<MutationRecord> records = std::move(pending_records_);
    pending_records_.clear();
    is_scheduled_ = false;

    // 调用回调
    if (callback_) {
        callback_(records, this);
    }
}

void MutationObserver::QueueRecord(MutationRecord record) {
    pending_records_.push_back(std::move(record));
    ScheduleCallback();
}

// ========== MutationObserverRegistry 实现 ==========

MutationObserverRegistry& MutationObserverRegistry::Instance() {
    static MutationObserverRegistry instance;
    return instance;
}

void MutationObserverRegistry::Register(std::shared_ptr<MutationObserver> observer) {
    if (observer) {
        observers_.insert(observer);
    }
}

void MutationObserverRegistry::Unregister(MutationObserver* observer) {
    for (auto it = observers_.begin(); it != observers_.end(); ) {
        if (it->get() == observer) {
            it = observers_.erase(it);
        } else {
            ++it;
        }
    }
}

void MutationObserverRegistry::NotifyNodeAdded(
    std::shared_ptr<Node> node,
    std::shared_ptr<Node> parent,
    std::shared_ptr<Node> previous_sibling,
    std::shared_ptr<Node> next_sibling) {

    for (const auto& observer : observers_) {
        observer->NotifyNodeAdded(node, parent, previous_sibling, next_sibling);
    }
}

void MutationObserverRegistry::NotifyNodeRemoved(
    std::shared_ptr<Node> node,
    std::shared_ptr<Node> parent,
    std::shared_ptr<Node> previous_sibling,
    std::shared_ptr<Node> next_sibling) {

    for (const auto& observer : observers_) {
        observer->NotifyNodeRemoved(node, parent, previous_sibling, next_sibling);
    }
}

void MutationObserverRegistry::NotifyAttributeChanged(
    std::shared_ptr<Element> element,
    const std::string& name,
    const std::string& old_value,
    const std::string& new_value,
    const std::string& namespace_uri) {

    for (const auto& observer : observers_) {
        observer->NotifyAttributeChanged(element, name, old_value, new_value, namespace_uri);
    }
}

void MutationObserverRegistry::NotifyTextChanged(
    std::shared_ptr<Node> node,
    const std::string& old_text,
    const std::string& new_text) {

    for (const auto& observer : observers_) {
        observer->NotifyTextChanged(node, old_text, new_text);
    }
}

} // namespace lightui
