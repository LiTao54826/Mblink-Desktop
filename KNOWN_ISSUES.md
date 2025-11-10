# LightUI 已知问题跟踪

**最后更新**: 2025-11-10  
**版本**: 0.2.0-alpha

---

## 🔴 高优先级问题

### Issue #1: 窗口顶部出现黑色区域

**状态**: 🔍 调查中  
**优先级**: 🔴 高  
**影响**: 所有示例应用的文字内容不可见  
**发现日期**: 2025-11-10

#### 问题描述

窗口顶部出现约 100-120px 的黑色区域，DOM 文字被渲染在这个区域内，导致不可见。

#### 症状

1. 窗口显示两条黑色横条（约 y=0-110 和 y=110-230）
2. DOM 文字被渲染在 y=0, y=17.875, y=35.75 位置（在黑色区域内）
3. 使用 Skia 直接绘制的图形和文字可以正常显示
4. 黑色区域的高度似乎与添加的 body_top_offset (120px) 相关

#### 已验证的事实

- ✅ Skia 图形渲染正常（矩形可以显示）
- ✅ Skia 文字渲染正常（使用 FontManager 的文字可以显示）
- ✅ 布局计算正确（位置坐标正确）
- ✅ Paint 调用正确（translate 正确）
- ✅ 窗口大小、客户区大小、Surface 大小都是 800x600（匹配）
- ✅ SDL Renderer 和 Texture 创建成功
- ✅ SDL3 API 使用正确（SDL_UpdateTexture 返回 true）

#### 可能的原因

1. **RenderBlock 背景色问题**
   - RenderBlock::Paint 调用 RenderBackgroundAdvanced 绘制背景
   - 默认背景色可能是黑色
   - body 元素的背景色覆盖了文字

2. **SDL Renderer 视口问题**
   - SDL Renderer 的视口或裁剪区域设置不正确
   - 导致部分区域被遮挡

3. **Z-order 问题**
   - 背景层在文字层之上
   - 绘制顺序不正确

4. **坐标系转换问题**
   - SDL 和 Skia 的坐标系不一致
   - translate 累积导致位置偏移

#### 已尝试的解决方案

1. ❌ 给 body 添加 120px top offset - 导致更多黑色条纹
2. ❌ 设置 body style="background-color: white;" - 无效（inline style 不生效）
3. ✅ 直接在 Skia canvas 绘制测试内容 - 可以显示

#### 下一步调查方向

1. 检查 RenderBlock::Paint 中的背景渲染逻辑
2. 检查 BoxRenderer::RenderBackgroundAdvanced 的实现
3. 验证 ComputedStyle 的默认背景色
4. 测试 GPU 模式是否有同样问题
5. 检查 SDL Renderer 的 viewport 和 clip rect
6. 添加更多调试输出，追踪背景绘制

#### 相关代码

- `core/window/window.cpp::RenderDocument()` - 渲染入口
- `core/render/render_object.cpp::RenderBlock::Paint()` - 背景渲染
- `core/render/box_renderer.cpp::RenderBackgroundAdvanced()` - 背景绘制
- `core/window/window.cpp::SwapBuffers()` - 缓冲区交换

---

### Issue #2: Inline Style 不生效

**状态**: ⏳ 待修复  
**优先级**: 🟡 中  
**影响**: 无法通过 inline style 设置元素样式  
**发现日期**: 2025-11-10

#### 问题描述

通过 `element->SetAttribute("style", "...")` 设置的 inline style 不生效。

#### 症状

```cpp
body->SetAttribute("style", "background-color: white;");
// 背景色不变，仍然是默认颜色
```

#### 可能的原因

1. StyleResolver 未实现 inline style 解析
2. SetAttribute 未触发样式重新计算
3. style 属性未被正确解析

#### 解决方案

1. 在 StyleResolver::ResolveStyle 中添加 inline style 解析
2. 解析 style 属性的值（CSS 声明列表）
3. 将 inline style 应用到 ComputedStyle（最高优先级）

---

### Issue #3: CSS Padding 属性不生效

**状态**: ⏳ 待修复  
**优先级**: 🟡 中  
**影响**: 无法通过 CSS 控制元素内边距  
**发现日期**: 2025-11-10

#### 问题描述

CSS padding 属性在布局中不生效。

#### 可能的原因

1. RenderBlock::Layout 未正确应用 padding
2. ComputedStyle 的 padding 值未正确计算
3. 布局算法中 padding 被忽略

#### 解决方案

1. 检查 RenderBlock::Layout 中的 padding 计算
2. 确保 padding 被正确应用到子元素位置
3. 添加测试用例验证 padding 功能

---

## 🟡 中优先级问题

### Issue #4: 每次渲染都重建渲染树

**状态**: ⏳ 待优化  
**优先级**: 🟡 中  
**影响**: 性能较低，不适合频繁更新的场景  
**发现日期**: 2025-11-10

#### 问题描述

`Window::RenderDocument()` 每次都重新构建整个渲染树，即使 DOM 没有变化。

#### 影响

- 性能开销大
- 不适合动画和频繁更新
- CPU 使用率高

#### 解决方案

1. 实现渲染树缓存
2. 只在 DOM 变化时重建渲染树
3. 实现增量更新机制
4. 添加脏标记（dirty flag）

---

### Issue #5: 调试代码过多

**状态**: ⏳ 待清理  
**优先级**: 🟢 低  
**影响**: 代码可读性差，日志输出混乱  
**发现日期**: 2025-11-10

#### 问题描述

代码中有大量 `std::cout` 调试输出，需要清理或改为日志系统。

#### 解决方案

1. 实现统一的日志系统（Logger）
2. 使用日志级别（DEBUG, INFO, WARN, ERROR）
3. 清理或注释掉调试代码
4. 添加编译选项控制日志输出

---

## 🟢 低优先级问题

### Issue #6: 缺少错误处理

**状态**: ⏳ 待改进  
**优先级**: 🟢 低  
**影响**: 异常情况下可能崩溃  

#### 问题描述

部分代码缺少错误处理和边界检查。

#### 解决方案

1. 添加空指针检查
2. 添加边界检查
3. 使用异常或错误码
4. 添加断言（assert）

---

### Issue #7: 代码注释不足

**状态**: ⏳ 待改进  
**优先级**: 🟢 低  
**影响**: 代码可维护性差  

#### 问题描述

部分复杂逻辑缺少注释，难以理解。

#### 解决方案

1. 添加函数注释（Doxygen 格式）
2. 添加复杂算法的说明
3. 添加示例代码
4. 完善 API 文档

---

## 📋 问题统计

- **总问题数**: 7
- **高优先级**: 3 🔴
- **中优先级**: 2 🟡
- **低优先级**: 2 🟢
- **已修复**: 0 ✅
- **进行中**: 1 🔍
- **待修复**: 6 ⏳

---

## 🔧 技术债务

### 架构改进

1. **渲染树缓存** - 避免每次重建
2. **样式缓存** - 缓存样式计算结果
3. **事件系统优化** - 减少事件传递开销
4. **内存管理** - 使用智能指针，避免内存泄漏

### 测试覆盖

1. **渲染测试** - 添加视觉回归测试
2. **性能测试** - 添加性能基准测试
3. **集成测试** - 更多端到端测试
4. **压力测试** - 测试极限情况

### 文档完善

1. **API 文档** - 完整的 API 参考
2. **架构文档** - 详细的架构设计
3. **开发指南** - 贡献者指南
4. **故障排除** - 常见问题解答

---

## 📝 问题报告模板

```markdown
### Issue #X: 问题标题

**状态**: 🔍 调查中 / ⏳ 待修复 / ✅ 已修复  
**优先级**: 🔴 高 / 🟡 中 / 🟢 低  
**影响**: 问题影响范围  
**发现日期**: YYYY-MM-DD

#### 问题描述

详细描述问题...

#### 症状

1. 症状1
2. 症状2

#### 可能的原因

1. 原因1
2. 原因2

#### 解决方案

1. 方案1
2. 方案2

#### 相关代码

- 文件路径::函数名
```

---

**维护者**: AI Assistant  
**最后审查**: 2025-11-10

