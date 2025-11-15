# 下一步工作指南

> **最后更新**: 2025-11-14  
> **当前进度**: Phase 2 已完成，准备开始 Phase 3  
> **下一个任务**: Task 3.1 - Transition 数据结构

---

## 🎯 当前状态

### ✅ 已完成
- ✅ Phase 1: 阴影和渐变 (100%)
  - Box Shadow, Text Shadow
  - Linear Gradient, Radial Gradient
  - 完整测试覆盖 (50个测试用例)
  
- ✅ Phase 2: CSS Transform (100%)
  - translate, rotate, scale, skew, matrix
  - transform-origin 支持
  - 完整测试覆盖 (19个测试用例)

### 🔜 下一步
- Phase 5: CSS 变量和滤镜 (0%)
- Phase 6: 性能优化 (0%)

---

## 📋 Phase 3: Transition 任务清单

### Task 3.1: Transition 数据结构 (1天)

**目标**: 定义 Transition 相关的数据结构

**需要创建的文件**:
```
core/render/transition.h
core/render/transition.cpp
tests/render/test_transition.cpp
```

**需要实现的结构**:

```cpp
// core/render/transition.h

namespace lightui {

/**
 * @brief CSS transition-timing-function 类型
 */
enum class TimingFunction {
    LINEAR,
    EASE,
    EASE_IN,
    EASE_OUT,
    EASE_IN_OUT,
    CUBIC_BEZIER
};

/**
 * @brief 贝塞尔曲线参数
 */
struct CubicBezier {
    float x1, y1, x2, y2;
    
    CubicBezier() : x1(0), y1(0), x2(1), y2(1) {}
    CubicBezier(float x1, float y1, float x2, float y2)
        : x1(x1), y1(y1), x2(x2), y2(y2) {}
    
    // 计算给定时间点的值
    float Evaluate(float t) const;
};

/**
 * @brief CSS transition 属性
 */
struct CSSTransition {
    std::string property;           // 过渡属性名
    float duration;                 // 持续时间 (秒)
    TimingFunction timing_function; // 缓动函数
    CubicBezier bezier;            // 自定义贝塞尔曲线
    float delay;                    // 延迟时间 (秒)
    
    CSSTransition()
        : property("all")
        , duration(0)
        , timing_function(TimingFunction::EASE)
        , delay(0) {}
    
    /**
     * @brief 解析 transition 属性
     * @param str CSS transition 字符串
     * @return 解析后的 transition 列表
     */
    static std::vector<CSSTransition> Parse(const std::string& str);
    
    /**
     * @brief 解析 transition-property
     */
    static std::vector<std::string> ParseProperty(const std::string& str);
    
    /**
     * @brief 解析 transition-duration
     */
    static std::vector<float> ParseDuration(const std::string& str);
    
    /**
     * @brief 解析 transition-timing-function
     */
    static std::vector<std::pair<TimingFunction, CubicBezier>> ParseTimingFunction(const std::string& str);
    
    /**
     * @brief 解析 transition-delay
     */
    static std::vector<float> ParseDelay(const std::string& str);
};

} // namespace lightui
```

**验收标准**:
- [ ] 定义所有必要的数据结构
- [ ] 实现 Parse 函数
- [ ] 编写 10个单元测试
- [ ] 所有测试通过

**预计时间**: 1天

---

### Task 3.2: 缓动函数实现 (1天)

**目标**: 实现各种缓动函数

**需要创建的文件**:
```
core/render/easing_functions.h
core/render/easing_functions.cpp
tests/render/test_easing_functions.cpp
```

**需要实现的函数**:

```cpp
// core/render/easing_functions.h

namespace lightui {

/**
 * @brief 缓动函数类
 */
class EasingFunctions {
public:
    // 线性
    static float Linear(float t);
    
    // 标准缓动
    static float Ease(float t);
    static float EaseIn(float t);
    static float EaseOut(float t);
    static float EaseInOut(float t);
    
    // 自定义贝塞尔曲线
    static float CubicBezier(float t, float x1, float y1, float x2, float y2);
    
private:
    // 贝塞尔曲线辅助函数
    static float SampleCurveX(float t, float x1, float x2);
    static float SampleCurveY(float t, float y1, float y2);
    static float SampleCurveDerivativeX(float t, float x1, float x2);
    static float SolveCurveX(float x, float x1, float x2);
};

} // namespace lightui
```

**验收标准**:
- [ ] 实现所有标准缓动函数
- [ ] 实现贝塞尔曲线计算
- [ ] 编写 15个单元测试
- [ ] 性能测试 (10000次计算 < 10ms)

**预计时间**: 1天

---

### Task 3.3: 动画时间轴管理 (2天)

**目标**: 实现动画时间轴和状态管理

**需要创建的文件**:
```
core/render/animation_timeline.h
core/render/animation_timeline.cpp
tests/render/test_animation_timeline.cpp
```

**需要实现的类**:

```cpp
// core/render/animation_timeline.h

namespace lightui {

/**
 * @brief 动画状态
 */
enum class AnimationState {
    IDLE,       // 空闲
    DELAYED,    // 延迟中
    RUNNING,    // 运行中
    FINISHED    // 已完成
};

/**
 * @brief 运行中的过渡动画
 */
struct RunningTransition {
    std::string property;           // 属性名
    CSSTransition transition;       // 过渡配置
    AnimationState state;           // 当前状态
    double start_time;              // 开始时间 (毫秒)
    double current_time;            // 当前时间 (毫秒)
    
    // 起始值和目标值 (根据属性类型存储)
    std::variant<float, SkColor, CSSTransform> start_value;
    std::variant<float, SkColor, CSSTransform> end_value;
    
    // 计算当前插值
    template<typename T>
    T GetCurrentValue() const;
};

/**
 * @brief 动画时间轴管理器
 */
class AnimationTimeline {
public:
    AnimationTimeline();
    ~AnimationTimeline();
    
    /**
     * @brief 启动一个过渡动画
     */
    void StartTransition(const std::string& property,
                        const CSSTransition& transition,
                        const std::variant<float, SkColor, CSSTransform>& start_value,
                        const std::variant<float, SkColor, CSSTransform>& end_value);
    
    /**
     * @brief 更新时间轴 (每帧调用)
     * @param current_time 当前时间 (毫秒)
     */
    void Update(double current_time);
    
    /**
     * @brief 获取属性的当前值
     */
    template<typename T>
    std::optional<T> GetCurrentValue(const std::string& property) const;
    
    /**
     * @brief 停止指定属性的过渡
     */
    void StopTransition(const std::string& property);
    
    /**
     * @brief 停止所有过渡
     */
    void StopAll();
    
    /**
     * @brief 是否有运行中的过渡
     */
    bool HasRunningTransitions() const;
    
private:
    std::vector<RunningTransition> running_transitions_;
    
    // 移除已完成的过渡
    void RemoveFinishedTransitions();
};

} // namespace lightui
```

**验收标准**:
- [ ] 实现时间轴管理
- [ ] 支持多个过渡同时运行
- [ ] 正确处理延迟
- [ ] 编写 20个单元测试

**预计时间**: 2天

---

### Task 3.4: 属性插值系统 (2天)

**目标**: 实现各种 CSS 属性的插值

**需要创建的文件**:
```
core/render/property_interpolation.h
core/render/property_interpolation.cpp
tests/render/test_property_interpolation.cpp
```

**需要实现的插值**:
- [ ] 数值插值 (width, height, margin, padding, etc.)
- [ ] 颜色插值 (color, background-color, border-color, etc.)
- [ ] Transform 插值
- [ ] Shadow 插值

**验收标准**:
- [ ] 实现所有常用属性的插值
- [ ] 编写 25个单元测试
- [ ] 性能测试

**预计时间**: 2天

---

### Task 3.5: 集成到渲染循环 (1天)

**目标**: 将 Transition 集成到现有渲染系统

**需要修改的文件**:
- `core/render/computed_style.h` - 添加 transition 属性
- `core/render/style_resolver.cpp` - 解析 transition 属性
- `core/render/render_object.h` - 添加动画时间轴
- `core/render/render_object.cpp` - 在渲染时应用过渡

**验收标准**:
- [ ] Transition 正确集成
- [ ] 样式变化时自动触发过渡
- [ ] 编写集成测试

**预计时间**: 1天

---

### Task 3.6: 文档和示例 (1天)

**目标**: 编写文档和示例

**需要创建的文件**:
- `docs/CSS_TRANSITION_API.md` - API 文档
- `docs/CSS_TRANSITION_GUIDE.md` - 使用指南
- `examples/css_transition.html` - 示例页面

**预计时间**: 1天

---

## 🚀 快速开始

### 1. 创建 Task 3.1 的文件

```bash
# 创建源文件
touch core/render/transition.h
touch core/render/transition.cpp

# 创建测试文件
touch tests/render/test_transition.cpp

# 更新 CMakeLists.txt
# 在 core/render/CMakeLists.txt 中添加 transition.cpp
# 在 tests/CMakeLists.txt 中添加 test_transition
```

### 2. 实现基本结构

从 `transition.h` 开始，定义数据结构，然后实现解析函数。

### 3. 编写测试

边写代码边写测试，确保每个功能都有测试覆盖。

### 4. 运行测试

```bash
cmake --build build --target test_transition -j8
./build/bin/Debug/test_transition.exe
```

---

## 📚 参考资料

### CSS Transition 规范
- [CSS Transitions Level 1](https://www.w3.org/TR/css-transitions-1/)
- [MDN: CSS Transitions](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions)

### 缓动函数
- [Easing Functions Cheat Sheet](https://easings.net/)
- [Cubic Bezier](https://cubic-bezier.com/)

### 已完成的相关代码
- `core/render/transform.h` - Transform 实现参考
- `core/render/shadow_renderer.cpp` - 渲染器实现参考
- `core/render/style_resolver.cpp` - CSS 解析参考

---

## ⚠️ 注意事项

1. **性能优化**
   - 过渡动画需要每帧更新，注意性能
   - 使用高效的数据结构
   - 避免不必要的计算

2. **内存管理**
   - 及时清理已完成的过渡
   - 避免内存泄漏

3. **测试覆盖**
   - 确保每个功能都有测试
   - 包含边界情况测试
   - 性能测试

4. **代码质量**
   - 遵循项目编码规范
   - 添加详细注释
   - 保持代码简洁

---

## 📞 需要帮助？

查看以下文档：
- [CSS_FEATURES_TASK_TRACKER.md](CSS_FEATURES_TASK_TRACKER.md) - 详细任务清单
- [CSS_ADVANCED_FEATURES_PLAN.md](CSS_ADVANCED_FEATURES_PLAN.md) - 完整开发计划
- [PROGRESS_SUMMARY.md](PROGRESS_SUMMARY.md) - 进度总结

---

**准备好了吗？让我们开始 Phase 3！** 🚀

