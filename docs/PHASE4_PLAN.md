# Phase 4: CSS Animation 开发计划

> **开始日期**: 2025-11-15 (预计)
> **预计完成**: 2025-11-26 (12天)
> **状态**: ⏳ 准备开始

---

## 📋 任务概览

Phase 4 将实现完整的 CSS Animation 功能，包括 @keyframes 规则解析、动画引擎、关键帧插值和动画事件。

### 任务列表

| 任务 | 预计时间 | 优先级 | 状态 |
|------|---------|--------|------|
| 4.1 @keyframes 解析 | 2天 | P0 | ⏳ 未开始 |
| 4.2 Animation 数据结构 | 1天 | P0 | ⏳ 未开始 |
| 4.3 AnimationController 实现 | 4天 | P0 | ⏳ 未开始 |
| 4.4 关键帧插值 | 2天 | P0 | ⏳ 未开始 |
| 4.5 Animation 事件 | 1天 | P1 | ⏳ 未开始 |
| 4.6 集成到渲染管线 | 1天 | P0 | ⏳ 未开始 |
| 4.7 文档和示例 | 1天 | P0 | ⏳ 未开始 |

---

## 🎯 Task 4.1: @keyframes 解析 (2天)

### 目标
实现 CSS @keyframes 规则的解析，支持关键帧定义和属性值。

### 需要创建的文件
```
core/render/keyframes.h
core/render/keyframes.cpp
tests/render/test_keyframes.cpp
```

### 数据结构

```cpp
namespace lightui {

/**
 * @brief 关键帧
 */
struct Keyframe {
    float offset;  // 0.0 到 1.0 (0% 到 100%)
    std::map<std::string, std::string> properties;  // 属性名 -> 属性值
    
    Keyframe() : offset(0.0f) {}
    Keyframe(float offset) : offset(offset) {}
};

/**
 * @brief @keyframes 规则
 */
struct KeyframesRule {
    std::string name;
    std::vector<Keyframe> keyframes;
    
    KeyframesRule() = default;
    KeyframesRule(const std::string& name) : name(name) {}
    
    /**
     * @brief 解析 @keyframes 规则
     * @param css CSS 字符串
     * @return 解析后的 keyframes 规则
     */
    static KeyframesRule Parse(const std::string& css);
    
    /**
     * @brief 获取指定进度的关键帧
     * @param progress 动画进度 (0.0 到 1.0)
     * @return 前后两个关键帧和插值因子
     */
    std::tuple<const Keyframe*, const Keyframe*, float> 
        GetKeyframesAt(float progress) const;
};

} // namespace lightui
```

### 支持的语法

```css
@keyframes slide-in {
    from {
        transform: translateX(-100%);
        opacity: 0;
    }
    to {
        transform: translateX(0);
        opacity: 1;
    }
}

@keyframes bounce {
    0% {
        transform: translateY(0);
    }
    50% {
        transform: translateY(-20px);
    }
    100% {
        transform: translateY(0);
    }
}

@keyframes fade-in-out {
    0%, 100% {
        opacity: 0;
    }
    50% {
        opacity: 1;
    }
}
```

### 测试用例 (15个)
- ✅ 解析简单的 from-to 关键帧
- ✅ 解析百分比关键帧
- ✅ 解析多个关键帧
- ✅ 解析多个属性
- ✅ 解析复合关键帧 (0%, 100%)
- ✅ GetKeyframesAt 测试
- ✅ 边界情况测试

---

## 🎯 Task 4.2: Animation 数据结构 (1天)

### 目标
定义 CSS animation 属性的数据结构。

### 需要创建的文件
```
core/render/animation.h
core/render/animation.cpp
tests/render/test_animation.cpp
```

### 数据结构

```cpp
namespace lightui {

/**
 * @brief 动画方向
 */
enum class AnimationDirection {
    NORMAL,          // 正常播放
    REVERSE,         // 反向播放
    ALTERNATE,       // 交替播放
    ALTERNATE_REVERSE  // 反向交替播放
};

/**
 * @brief 动画填充模式
 */
enum class AnimationFillMode {
    NONE,      // 不填充
    FORWARDS,  // 保持最后一帧
    BACKWARDS, // 应用第一帧
    BOTH       // 两者都应用
};

/**
 * @brief CSS animation 属性
 */
struct CSSAnimation {
    std::string name;                   // 动画名称
    float duration;                     // 持续时间 (秒)
    TimingFunction timing_function;     // 缓动函数
    CubicBezier bezier;                // 自定义贝塞尔曲线
    float delay;                        // 延迟时间 (秒)
    int iteration_count;                // 迭代次数 (-1 表示 infinite)
    AnimationDirection direction;       // 播放方向
    AnimationFillMode fill_mode;        // 填充模式
    bool paused;                        // 是否暂停
    
    CSSAnimation()
        : name("")
        , duration(0)
        , timing_function(TimingFunction::EASE)
        , delay(0)
        , iteration_count(1)
        , direction(AnimationDirection::NORMAL)
        , fill_mode(AnimationFillMode::NONE)
        , paused(false) {}
    
    /**
     * @brief 解析 animation 属性
     * @param str CSS animation 字符串
     * @return 解析后的 animation 列表
     */
    static std::vector<CSSAnimation> Parse(const std::string& str);
};

} // namespace lightui
```

### 支持的语法

```css
/* 简写属性 */
animation: slide-in 0.5s ease-in-out;
animation: fade-in 1s ease 0.5s infinite alternate;
animation: bounce 0.3s ease-in-out 3;

/* 分解属性 */
animation-name: slide-in;
animation-duration: 0.5s;
animation-timing-function: ease-in-out;
animation-delay: 0.1s;
animation-iteration-count: infinite;
animation-direction: alternate;
animation-fill-mode: forwards;
animation-play-state: paused;
```

### 测试用例 (20个)
- ✅ 解析简单动画
- ✅ 解析带延迟的动画
- ✅ 解析 infinite 动画
- ✅ 解析 alternate 方向
- ✅ 解析 fill-mode
- ✅ 解析多个动画
- ✅ 性能测试

---

## 🎯 Task 4.3: AnimationController 实现 (4天)

### 目标
实现动画控制器，管理所有运行中的动画。

### 需要创建的文件
```
core/render/animation_controller.h
core/render/animation_controller.cpp
tests/render/test_animation_controller.cpp
```

### 核心类

```cpp
namespace lightui {

/**
 * @brief 动画状态
 */
enum class AnimationState {
    IDLE,      // 未开始
    DELAYED,   // 延迟中
    RUNNING,   // 运行中
    PAUSED,    // 暂停
    FINISHED   // 已完成
};

/**
 * @brief 运行中的动画
 */
struct RunningAnimation {
    RenderObject* object;
    CSSAnimation config;
    const KeyframesRule* keyframes;
    AnimationState state;
    double start_time;
    double current_time;
    int current_iteration;
    
    RunningAnimation()
        : object(nullptr)
        , keyframes(nullptr)
        , state(AnimationState::IDLE)
        , start_time(0)
        , current_time(0)
        , current_iteration(0) {}
};

/**
 * @brief 动画控制器
 */
class AnimationController {
public:
    AnimationController();
    ~AnimationController();
    
    /**
     * @brief 注册 @keyframes 规则
     */
    void RegisterKeyframes(const KeyframesRule& rule);
    
    /**
     * @brief 启动动画
     */
    void StartAnimation(RenderObject* object, const CSSAnimation& animation);
    
    /**
     * @brief 停止动画
     */
    void StopAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 停止所有动画
     */
    void StopAllAnimations(RenderObject* object);
    
    /**
     * @brief 暂停动画
     */
    void PauseAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 恢复动画
     */
    void ResumeAnimation(RenderObject* object, const std::string& name);
    
    /**
     * @brief 更新所有动画 (每帧调用)
     */
    void Update(double current_time);
    
    /**
     * @brief 获取当前动画属性值
     */
    std::optional<std::map<std::string, std::string>> 
        GetCurrentProperties(RenderObject* object, const std::string& name) const;
    
private:
    std::map<std::string, KeyframesRule> keyframes_rules_;
    std::vector<RunningAnimation> running_animations_;
    
    /**
     * @brief 计算当前帧的属性值
     */
    std::map<std::string, std::string> ComputeCurrentFrame(
        const RunningAnimation& anim, float progress);
};

} // namespace lightui
```

### 测试用例 (25个)
- ✅ 注册 keyframes
- ✅ 启动动画
- ✅ 停止动画
- ✅ 暂停/恢复动画
- ✅ 更新动画进度
- ✅ 处理延迟
- ✅ 处理迭代
- ✅ 处理方向
- ✅ 处理填充模式
- ✅ 性能测试

---

## 📈 性能目标

- **@keyframes 解析**: 1000个规则 < 100ms
- **动画更新**: 1000个并发动画 < 50ms
- **关键帧插值**: 10000次插值 < 20ms
- **内存占用**: 每个动画 < 1KB

---

## 📚 参考资料

- [W3C CSS Animations Specification](https://www.w3.org/TR/css-animations-1/)
- [MDN CSS Animations](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Animations)
- [CSS Animation Performance](https://web.dev/animations/)

---

## ✅ 验收标准

1. **功能完整性**
   - ✅ 支持所有 animation 属性
   - ✅ 支持 @keyframes 规则
   - ✅ 支持多个并发动画
   - ✅ 支持动画控制 (暂停/恢复/停止)

2. **代码质量**
   - ✅ 编译无错误无警告
   - ✅ 所有测试通过 (60+个)
   - ✅ 无内存泄漏
   - ✅ 遵循项目编码规范

3. **性能要求**
   - ✅ 所有性能指标达标
   - ✅ 60 FPS 流畅动画

4. **文档完整性**
   - ✅ API 文档完整
   - ✅ 使用指南详细
   - ✅ 示例代码可运行
   - ✅ CHANGELOG 更新

