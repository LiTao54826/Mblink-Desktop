# Design Document: Animation Element Association

## Overview

本设计文档描述了将动画系统从 `RenderObject*` 关联改为 `Element` 关联的重构方案。核心思想是参考 Blink 的实现，将动画的生命周期与 DOM Element 绑定，而不是与可能被销毁重建的 RenderObject 绑定。

### 问题背景

当前实现中，动画通过 `RenderObject*` 指针关联：
- `RunningAnimation.object` 存储 `RenderObject*`
- `RunningTransition.object` 存储 `RenderObject*`
- `AnimationDirtyTracker` 使用 `RenderObject*` 作为键

当 DOM 发生变化（如添加 Modal）时，`InvalidateRenderTree()` 被调用，导致：
1. 旧的 RenderObject 被销毁
2. 新的 RenderObject 被创建
3. 动画系统中的 `RenderObject*` 指针变成悬空指针
4. 动画被中断或丢失

### 解决方案

将动画关联从 `RenderObject*` 改为 `Element*`（或 `std::weak_ptr<Element>`）：
- Element 在 DOM 变化时保持稳定
- 通过 `Element::GetRenderObject()` 获取当前的 RenderObject
- 渲染树重建不影响动画的持续运行

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         DOM Layer                                │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                         │
│  │ Element │  │ Element │  │ Element │  (稳定，不随渲染树重建)   │
│  └────┬────┘  └────┬────┘  └────┬────┘                         │
│       │            │            │                               │
│       ▼            ▼            ▼                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Animation System (重构后)                    │   │
│  │  ┌─────────────────┐  ┌─────────────────┐               │   │
│  │  │AnimationController│  │AnimationTimeline│               │   │
│  │  │ (Element 引用)   │  │ (Element 引用)  │               │   │
│  │  └────────┬────────┘  └────────┬────────┘               │   │
│  │           │                    │                         │   │
│  │           ▼                    ▼                         │   │
│  │  ┌─────────────────────────────────────────────────┐    │   │
│  │  │           AnimationApplicator                    │    │   │
│  │  │  (通过 Element 获取当前 RenderObject)            │    │   │
│  │  └─────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Render Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ RenderObject │  │ RenderObject │  │ RenderObject │          │
│  │ (可能重建)   │  │ (可能重建)   │  │ (可能重建)   │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

## Components and Interfaces

### 1. RunningAnimation 结构体修改

```cpp
struct RunningAnimation {
    // 旧: RenderObject* object;
    // 新: 使用 weak_ptr 避免循环引用和悬空指针
    std::weak_ptr<Element> element;     ///< 关联的 DOM 元素
    
    CSSAnimation config;                 ///< 动画配置
    const KeyframesRule* keyframes;      ///< 关键帧规则
    CSSAnimationState state;             ///< 当前状态
    double start_time;                   ///< 开始时间
    double current_time;                 ///< 当前时间
    int current_iteration;               ///< 当前迭代次数
    int last_iteration;                  ///< 上一次迭代次数
    bool initialized;                    ///< 是否已初始化
    bool start_event_fired;              ///< 是否已触发 start 事件
    bool end_event_fired;                ///< 是否已触发 end 事件
    
    /**
     * @brief 获取当前关联的 RenderObject
     * @return RenderObject 指针，如果 Element 已销毁或无 RenderObject 则返回 nullptr
     */
    RenderObject* GetRenderObject() const {
        if (auto elem = element.lock()) {
            return elem->GetRenderObject();
        }
        return nullptr;
    }
    
    /**
     * @brief 检查动画是否仍然有效
     */
    bool IsValid() const {
        return !element.expired();
    }
};
```

### 2. RunningTransition 结构体修改

```cpp
struct RunningTransition {
    // 旧: RenderObject* object;
    std::weak_ptr<Element> element;      ///< 关联的 DOM 元素
    
    std::string property;                ///< 属性名
    CSSTransition transition;            ///< 过渡配置
    AnimationState state;                ///< 当前状态
    double start_time;                   ///< 开始时间
    double current_time;                 ///< 当前时间
    TransitionValue start_value;         ///< 起始值
    TransitionValue end_value;           ///< 目标值
    
    /**
     * @brief 获取当前关联的 RenderObject
     */
    RenderObject* GetRenderObject() const {
        if (auto elem = element.lock()) {
            return elem->GetRenderObject();
        }
        return nullptr;
    }
    
    /**
     * @brief 检查过渡是否仍然有效
     */
    bool IsValid() const {
        return !element.expired();
    }
};
```

### 3. AnimationController 接口修改

```cpp
class AnimationController {
public:
    /**
     * @brief 启动动画 (新接口，推荐使用)
     * @param element DOM 元素
     * @param animation 动画配置
     */
    void StartAnimation(std::shared_ptr<Element> element, const CSSAnimation& animation);
    
    /**
     * @brief 启动动画 (兼容接口，内部提取 Element)
     * @param object 渲染对象
     * @param animation 动画配置
     * @deprecated 推荐使用 Element 版本
     */
    void StartAnimation(RenderObject* object, const CSSAnimation& animation);
    
    /**
     * @brief 停止动画
     * @param element DOM 元素
     * @param name 动画名称
     */
    void StopAnimation(std::shared_ptr<Element> element, const std::string& name);
    
    /**
     * @brief 更新所有动画
     * 
     * 遍历所有运行中的动画，通过 Element 获取当前 RenderObject，
     * 如果 RenderObject 为 null，跳过应用但保留动画状态。
     */
    void Update(double current_time);
    
    /**
     * @brief 获取当前动画属性值
     * @param element DOM 元素
     * @param name 动画名称
     */
    std::optional<std::map<std::string, std::string>> 
        GetCurrentProperties(std::shared_ptr<Element> element, const std::string& name) const;
    
private:
    /**
     * @brief 从 RenderObject 提取 Element
     */
    std::shared_ptr<Element> ExtractElement(RenderObject* object);
    
    /**
     * @brief 清理无效动画（Element 已销毁）
     */
    void CleanupInvalidAnimations();
};
```

### 4. AnimationTimeline 接口修改

```cpp
class AnimationTimeline {
public:
    /**
     * @brief 启动过渡 (新接口)
     */
    void StartTransition(std::shared_ptr<Element> element,
                        const std::string& property,
                        const CSSTransition& transition,
                        const TransitionValue& start_value,
                        const TransitionValue& end_value);
    
    /**
     * @brief 启动过渡 (兼容接口)
     */
    void StartTransition(RenderObject* object,
                        const std::string& property,
                        const CSSTransition& transition,
                        const TransitionValue& start_value,
                        const TransitionValue& end_value);
    
    /**
     * @brief 获取当前值
     */
    std::optional<TransitionValue> GetCurrentValue(std::shared_ptr<Element> element,
                                                    const std::string& property) const;
    
private:
    /**
     * @brief 清理无效过渡
     */
    void CleanupInvalidTransitions();
};
```

### 5. AnimationDirtyTracker 修改

```cpp
class AnimationDirtyTracker {
public:
    /**
     * @brief 标记动画为脏 (新接口)
     */
    void MarkDirty(std::weak_ptr<Element> element, const std::string& animation_name);
    
    /**
     * @brief 检查动画是否为脏
     */
    bool IsDirty(std::weak_ptr<Element> element, const std::string& animation_name) const;
    
    /**
     * @brief 清除脏标记
     */
    void ClearDirty(std::weak_ptr<Element> element, const std::string& animation_name);
    
private:
    // 使用 Element 指针地址作为键（Element 生命周期稳定）
    std::unordered_map<Element*, std::unordered_set<std::string>> dirty_flags_;
};
```

### 6. AnimationLayerBridge 修改

```cpp
class AnimationLayerBridge {
public:
    /**
     * @brief 通知动画开始 (新接口)
     */
    void OnAnimationStart(std::shared_ptr<Element> element,
                          const std::string& animation_name,
                          const std::vector<std::string>& properties);
    
    /**
     * @brief 应用动画属性值
     */
    AnimationUpdateType ApplyAnimationProperty(std::shared_ptr<Element> element,
                                                const std::string& property,
                                                const std::string& value);
    
private:
    // 使用 Element* 作为键
    std::unordered_map<Element*, AnimationState> animation_states_;
    std::unordered_set<Element*> animation_promoted_objects_;
};
```

## Data Models

### Element 与 RenderObject 的关系

```cpp
class Element {
public:
    /**
     * @brief 获取关联的 RenderObject
     * @return RenderObject 指针，可能为 nullptr（如 display:none 或渲染树重建中）
     */
    RenderObject* GetRenderObject() const { return render_object_; }
    
    /**
     * @brief 设置关联的 RenderObject
     * 在渲染树构建时调用
     */
    void SetRenderObject(RenderObject* object) { render_object_ = object; }
    
private:
    RenderObject* render_object_ = nullptr;
};
```

### 动画查找键

```cpp
// 使用 Element 指针和动画名称作为复合键
struct AnimationKey {
    Element* element;
    std::string animation_name;
    
    bool operator==(const AnimationKey& other) const {
        return element == other.element && animation_name == other.animation_name;
    }
};

// 哈希函数
struct AnimationKeyHash {
    size_t operator()(const AnimationKey& key) const {
        return std::hash<Element*>()(key.element) ^ 
               std::hash<std::string>()(key.animation_name);
    }
};
```

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

### Property 1: Animation State Preservation Across Render Tree Rebuilds

*For any* running animation on an Element, when the render tree is rebuilt (e.g., due to DOM changes like adding a modal), the animation state (progress, iteration count, timing) SHALL be preserved.

**Validates: Requirements 1.1, 1.2**

### Property 2: Animation Re-association After Rebuild

*For any* Element with running animations, when a new RenderObject is created for that Element after a render tree rebuild, the animation values SHALL be correctly applied to the new RenderObject.

**Validates: Requirements 1.3, 2.3, 3.3**

### Property 3: Animation Cleanup on Element Removal

*For any* Element with running animations, when the Element is removed from the DOM, all associated animations SHALL be stopped and cleaned up (no memory leaks, no dangling references).

**Validates: Requirements 1.4**

### Property 4: RenderObject Lookup from Element

*For any* animation update operation, the system SHALL obtain the current RenderObject by calling `Element::GetRenderObject()` rather than using a stored RenderObject pointer.

**Validates: Requirements 2.2, 3.2, 4.1, 5.3**

### Property 5: Graceful Handling of Null RenderObject

*For any* animation whose Element has a null RenderObject (e.g., during render tree rebuild or display:none), the animation state SHALL be preserved but value application SHALL be skipped.

**Validates: Requirements 2.4, 3.4, 4.3**

### Property 6: InvalidateRenderTree Does Not Clear Animations

*For any* call to `InvalidateRenderTree()`, the running animations in AnimationController and transitions in AnimationTimeline SHALL NOT be cleared.

**Validates: Requirements 6.1, 6.2, 6.3**

## Error Handling

### 1. Element 已销毁

当 `weak_ptr<Element>` 过期时：
- `RunningAnimation::IsValid()` 返回 false
- 在 `Update()` 中清理无效动画
- 不触发任何事件

### 2. RenderObject 为 null

当 `Element::GetRenderObject()` 返回 nullptr 时：
- 保留动画状态（继续计时）
- 跳过属性值应用
- 下一帧重试

### 3. 类型转换失败

当 `Node` 不是 `Element` 类型时：
- `StartAnimation(RenderObject*)` 返回而不启动动画
- 记录警告日志

## Testing Strategy

### 单元测试

1. **RunningAnimation 结构体测试**
   - 测试 `GetRenderObject()` 在各种状态下的行为
   - 测试 `IsValid()` 的正确性

2. **AnimationController 测试**
   - 测试 `StartAnimation(Element)` 正确存储 Element 引用
   - 测试 `Update()` 正确处理 null RenderObject
   - 测试动画清理逻辑

3. **AnimationTimeline 测试**
   - 测试过渡的 Element 关联
   - 测试过渡状态保持

### 属性测试

使用 [rapidcheck](https://github.com/emil-e/rapidcheck) 进行属性测试。

**测试框架配置**：
- 每个属性测试运行 100 次迭代
- 使用随机生成的 Element 和动画配置

**属性测试注释格式**：
```cpp
/**
 * Property Test: Animation State Preservation
 * 
 * Feature: animation-element-association, Property 1: Animation State Preservation
 * Validates: Requirements 1.1, 1.2
 */
RC_GTEST_PROP(AnimationController, AnimationStatePersistsAcrossRebuild, ()) {
    // 测试实现
}
```

### 集成测试

1. **Modal 动画测试** (`test_modal_animation.js`)
   - 启动 spinner 动画
   - 打开 Modal
   - 验证 spinner 动画继续运行

2. **DOM 变化测试**
   - 添加/删除兄弟元素
   - 验证动画不受影响

3. **渲染树重建测试**
   - 触发完整渲染树重建
   - 验证所有动画状态保持
