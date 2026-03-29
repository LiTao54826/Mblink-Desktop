/**
 * @file event_types.h
 * @brief 事件类型定义（参考RmlUi设计）
 * 
 * 设计理念：
 * - 使用EventId枚举而不是字符串，提升性能
 * - 支持字符串<->EventId双向转换
 * - 完整的事件类型定义（鼠标、键盘、焦点、拖拽）
 * 
 * 参考：ReferenceProject/RmlUi/Include/RmlUi/Core/ID.h
 */

#pragma once

#include <string>
#include <unordered_map>

namespace mbink {

/**
 * @brief 事件ID枚举
 * 
 * 使用枚举而不是字符串的优势：
 * 1. 性能：枚举比较比字符串比较快10-100倍
 * 2. 类型安全：编译期检查
 * 3. 内存：枚举占用4字节，字符串占用更多
 */
enum class EventId : uint32_t {
    Invalid = 0,
    
    // ========== 鼠标事件 ==========
    Click,              // 单击
    Dblclick,           // 双击
    Mousedown,          // 鼠标按下
    Mouseup,            // 鼠标释放
    Mousemove,          // 鼠标移动
    Mouseover,          // 鼠标进入（冒泡）
    Mouseout,           // 鼠标离开（冒泡）
    Mouseenter,         // 鼠标进入（不冒泡）
    Mouseleave,         // 鼠标离开（不冒泡）
    Contextmenu,        // 右键菜单
    
    // ========== 键盘事件 ==========
    Keydown,            // 键盘按下
    Keyup,              // 键盘释放
    Keypress,           // 键盘按键（已废弃，但保留兼容）
    Textinput,          // 文本输入
    
    // ========== 焦点事件 ==========
    Focus,              // 获得焦点（不冒泡）
    Blur,               // 失去焦点（不冒泡）
    Focusin,            // 获得焦点（冒泡）
    Focusout,           // 失去焦点（冒泡）
    
    // ========== 拖拽事件 ==========
    Dragstart,          // 开始拖拽
    Drag,               // 拖拽中
    Dragend,            // 拖拽结束
    Dragover,           // 拖拽经过目标
    Dragenter,          // 拖拽进入目标
    Dragleave,          // 拖拽离开目标
    Drop,               // 放置到目标
    
    // ========== 表单事件 ==========
    Submit,             // 表单提交
    Change,             // 值改变
    Input,              // 输入
    
    // ========== 窗口事件 ==========
    Resize,             // 窗口大小改变
    Scroll,             // 滚动
    Load,               // 加载完成
    Unload,             // 卸载
    
    // ========== 自定义事件 ==========
    Custom,             // 自定义事件起始ID
    
    // 总数（用于数组大小）
    MaxNumIds = 256
};

/**
 * @brief 事件规范（描述事件的属性）
 */
struct EventSpecification {
    EventId id;                 // 事件ID
    std::string type;           // 事件类型字符串
    bool bubbles;               // 是否冒泡
    bool cancelable;            // 是否可取消
    bool interruptible;         // 是否可中断传播
    
    EventSpecification(EventId id, const std::string& type, 
                      bool bubbles = true, 
                      bool cancelable = true,
                      bool interruptible = true)
        : id(id), type(type), bubbles(bubbles), 
          cancelable(cancelable), interruptible(interruptible) {}
};

/**
 * @brief 事件类型注册表（单例）
 * 
 * 功能：
 * - 管理EventId <-> 字符串的映射
 * - 提供事件规范查询
 * - 支持自定义事件注册
 * 
 * 参考：RmlUi的EventSpecificationInterface
 */
class EventTypeRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static EventTypeRegistry& GetInstance();
    
    /**
     * @brief 根据字符串获取EventId
     * @param type 事件类型字符串
     * @return EventId，如果不存在返回EventId::Invalid
     */
    EventId GetId(const std::string& type) const;
    
    /**
     * @brief 根据字符串获取或插入EventId
     * @param type 事件类型字符串
     * @return EventId
     */
    EventId GetIdOrInsert(const std::string& type);
    
    /**
     * @brief 根据EventId获取字符串
     * @param id 事件ID
     * @return 事件类型字符串
     */
    std::string GetType(EventId id) const;
    
    /**
     * @brief 获取事件规范
     * @param id 事件ID
     * @return 事件规范指针，如果不存在返回nullptr
     */
    const EventSpecification* GetSpecification(EventId id) const;
    
    /**
     * @brief 注册自定义事件
     * @param type 事件类型字符串
     * @param bubbles 是否冒泡
     * @param cancelable 是否可取消
     * @return 分配的EventId
     */
    EventId RegisterCustomEvent(const std::string& type, 
                                bool bubbles = true, 
                                bool cancelable = true);
    
private:
    EventTypeRegistry();
    ~EventTypeRegistry() = default;
    
    // 禁止拷贝和移动
    EventTypeRegistry(const EventTypeRegistry&) = delete;
    EventTypeRegistry& operator=(const EventTypeRegistry&) = delete;
    
    /**
     * @brief 注册内置事件
     */
    void RegisterBuiltinEvents();
    
    /**
     * @brief 注册单个事件
     */
    void RegisterEvent(EventId id, const std::string& type, 
                      bool bubbles = true, 
                      bool cancelable = true,
                      bool interruptible = true);
    
    // 事件规范数组（按EventId索引）
    EventSpecification* specifications_[static_cast<size_t>(EventId::MaxNumIds)];
    
    // 字符串 -> EventId 映射
    std::unordered_map<std::string, EventId> type_to_id_;
    
    // 下一个自定义事件ID
    uint32_t next_custom_id_;
};

/**
 * @brief 辅助函数：字符串转EventId
 */
inline EventId StringToEventId(const std::string& type) {
    return EventTypeRegistry::GetInstance().GetId(type);
}

/**
 * @brief 辅助函数：EventId转字符串
 */
inline std::string EventIdToString(EventId id) {
    return EventTypeRegistry::GetInstance().GetType(id);
}

/**
 * @brief 辅助函数：获取事件规范
 */
inline const EventSpecification* GetEventSpecification(EventId id) {
    return EventTypeRegistry::GetInstance().GetSpecification(id);
}

/**
 * @brief 辅助函数：检查事件是否冒泡
 */
inline bool IsEventBubbles(EventId id) {
    auto spec = GetEventSpecification(id);
    return spec ? spec->bubbles : true;
}

/**
 * @brief 辅助函数：检查事件是否可取消
 */
inline bool IsEventCancelable(EventId id) {
    auto spec = GetEventSpecification(id);
    return spec ? spec->cancelable : true;
}

} // namespace mbink

