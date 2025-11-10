# 🔧 LightUI 渲染 API 重构计划

**日期**: 2025-11-10  
**目标**: 添加高层统一渲染 API，支持 JavaScript 绑定  
**预计时间**: 2.5-3 小时  
**影响范围**: 新增代码，不破坏现有实现

---

## 📋 重构概述

### 核心思路

**保留低层 API + 新增高层 API**

```
低层 API (已有)          高层 API (新增)           JavaScript 绑定
┌─────────────┐         ┌──────────────────┐      ┌─────────────┐
│  Shapes     │    ┌───→│ UnifiedRenderer  │─────→│ JS Bindings │
│TextRenderer │────┤    │  (状态管理)      │      └─────────────┘
│ImageRenderer│    └───→│  (简化接口)      │
└─────────────┘         └──────────────────┘
     ↑                           ↑
     │                           │
  C++ 内部使用              JS 绑定使用
```

### 设计原则

1. ✅ **不破坏现有代码** - 保留所有现有类和测试
2. ✅ **复用现有实现** - UnifiedRenderer 内部调用 Shapes 等
3. ✅ **状态管理** - 内置 Paint、Font 状态
4. ✅ **简化接口** - 不需要每次传递 Paint 参数
5. ✅ **易于绑定** - 适合 JavaScript 调用

---

## 🎯 重构任务清单

### Phase 1: 设计和准备 (30 分钟)

#### Task 1.1: 设计 UnifiedRenderer API ✅
- [x] 确定需要的状态（Paint、Font）
- [x] 设计绘制方法签名
- [x] 设计辅助方法（SaveToFile 等）
- [x] 设计状态设置方法

#### Task 1.2: 创建文件结构
- [ ] 创建 `core/render/unified_renderer.h`
- [ ] 创建 `core/render/unified_renderer.cpp`
- [ ] 更新 `core/render/CMakeLists.txt`

---

### Phase 2: 实现 UnifiedRenderer (1.5 小时)

#### Task 2.1: 实现头文件 (30 分钟)
**文件**: `core/render/unified_renderer.h`

**内容**:
```cpp
class UnifiedRenderer {
private:
    // 核心对象
    sk_sp<SkSurface> surface_;
    SkCanvas* canvas_;
    bool owns_surface_;
    
    // 状态管理
    Paint fill_paint_;
    Paint stroke_paint_;
    SkFont current_font_;
    std::shared_ptr<FontManager> font_manager_;
    
    // 辅助对象（延迟创建）
    std::unique_ptr<Shapes> shapes_;
    std::unique_ptr<TextRenderer> text_renderer_;
    std::unique_ptr<ImageRenderer> image_renderer_;
    
public:
    // 构造函数
    UnifiedRenderer(int width, int height);
    UnifiedRenderer(sk_sp<SkSurface> surface);
    UnifiedRenderer(SkCanvas* canvas);
    
    // 状态设置 (10 个方法)
    void SetFillColor(const Color& color);
    void SetFillColor(const std::string& color_str);
    void SetStrokeColor(const Color& color);
    void SetStrokeColor(const std::string& color_str);
    void SetLineWidth(float width);
    void SetFont(const std::string& family, float size, bool bold = false, bool italic = false);
    void SetFontSize(float size);
    void SetOpacity(float alpha);
    
    // 基础绘制 (10 个方法)
    void FillRect(float x, float y, float width, float height);
    void DrawRect(float x, float y, float width, float height);
    void FillCircle(float cx, float cy, float radius);
    void DrawCircle(float cx, float cy, float radius);
    void DrawLine(float x1, float y1, float x2, float y2);
    void FillRoundRect(float x, float y, float width, float height, float radius);
    void DrawRoundRect(float x, float y, float width, float height, float radius);
    
    // 文本绘制 (3 个方法)
    void DrawText(const std::string& text, float x, float y);
    TextMetrics MeasureText(const std::string& text);
    
    // 图片绘制 (3 个方法)
    sk_sp<SkImage> LoadImage(const std::string& path);
    void DrawImage(sk_sp<SkImage> image, float x, float y);
    void DrawImage(sk_sp<SkImage> image, float x, float y, float width, float height);
    
    // 渲染控制 (4 个方法)
    void Clear(const Color& color = Color::White());
    void Flush();
    bool SaveToFile(const std::string& path);
    
    // 状态管理 (4 个方法)
    void Save();
    void Restore();
    void Translate(float dx, float dy);
    void Scale(float sx, float sy);
    void Rotate(float degrees);
    
    // 获取器
    SkCanvas* GetCanvas() const { return canvas_; }
    sk_sp<SkSurface> GetSurface() const { return surface_; }
};
```

**检查点**:
- [ ] 所有方法签名定义完整
- [ ] 文档注释完整
- [ ] 包含必要的头文件

#### Task 2.2: 实现构造函数和析构函数 (15 分钟)
**文件**: `core/render/unified_renderer.cpp`

**内容**:
- [ ] 实现 3 个构造函数
- [ ] 初始化 Paint 状态（fill/stroke）
- [ ] 初始化 Font 状态
- [ ] 创建 FontManager

**检查点**:
- [ ] 构造函数正确初始化所有成员
- [ ] Paint 默认状态正确（fill: black, stroke: black, width: 1）
- [ ] Font 默认状态正确（Arial, 14px）

#### Task 2.3: 实现状态设置方法 (15 分钟)
**方法列表**:
- [ ] `SetFillColor(const Color&)`
- [ ] `SetFillColor(const std::string&)` - 解析颜色字符串
- [ ] `SetStrokeColor(const Color&)`
- [ ] `SetStrokeColor(const std::string&)`
- [ ] `SetLineWidth(float)`
- [ ] `SetFont(...)` - 创建 SkFont
- [ ] `SetFontSize(float)`
- [ ] `SetOpacity(float)`

**检查点**:
- [ ] 颜色字符串解析正确（#RGB, #RRGGBB, rgb(), rgba()）
- [ ] Paint 状态正确更新
- [ ] Font 状态正确更新

#### Task 2.4: 实现绘制方法 (30 分钟)
**基础图形** (7 个方法):
- [ ] `FillRect` - 调用 `shapes_->FillRect(..., fill_paint_)`
- [ ] `DrawRect` - 调用 `shapes_->StrokeRect(..., stroke_paint_)`
- [ ] `FillCircle`
- [ ] `DrawCircle`
- [ ] `DrawLine`
- [ ] `FillRoundRect`
- [ ] `DrawRoundRect`

**文本绘制** (2 个方法):
- [ ] `DrawText` - 调用 `text_renderer_->DrawText(..., current_font_, fill_paint_)`
- [ ] `MeasureText` - 调用 `text_renderer_->MeasureText(..., current_font_)`

**图片绘制** (3 个方法):
- [ ] `LoadImage` - 调用 `ImageLoader::LoadFromFile`
- [ ] `DrawImage(image, x, y)` - 调用 `image_renderer_->DrawImage`
- [ ] `DrawImage(image, x, y, w, h)` - 调用 `image_renderer_->DrawImage`

**检查点**:
- [ ] 延迟创建辅助对象（shapes_, text_renderer_, image_renderer_）
- [ ] 正确传递状态（fill_paint_, stroke_paint_, current_font_）
- [ ] 所有方法都能正确调用

#### Task 2.5: 实现辅助方法 (15 分钟)
**渲染控制** (3 个方法):
- [ ] `Clear` - 调用 `canvas_->clear()`
- [ ] `Flush` - 调用 `surface_->flush()`
- [ ] `SaveToFile` - 编码为 PNG 并保存

**状态管理** (5 个方法):
- [ ] `Save` - 调用 `canvas_->save()`
- [ ] `Restore` - 调用 `canvas_->restore()`
- [ ] `Translate` - 调用 `canvas_->translate()`
- [ ] `Scale` - 调用 `canvas_->scale()`
- [ ] `Rotate` - 调用 `canvas_->rotate()`

**SaveToFile 实现**:
```cpp
bool UnifiedRenderer::SaveToFile(const std::string& path) {
    if (!surface_) return false;
    
    auto image = surface_->makeImageSnapshot();
    if (!image) return false;
    
    auto data = image->encodeToData(SkEncodedImageFormat::kPNG, 100);
    if (!data) return false;
    
    SkFILEWStream stream(path.c_str());
    return stream.write(data->data(), data->size());
}
```

**检查点**:
- [ ] SaveToFile 能正确保存 PNG
- [ ] 状态管理方法正确调用 Canvas API

---

### Phase 3: 测试验证 (30 分钟)

#### Task 3.1: 创建单元测试 (20 分钟)
**文件**: `tests/test_unified_renderer.cpp`

**测试内容**:
```cpp
// 1. 构造函数测试
void TestConstructor();

// 2. 状态设置测试
void TestSetFillColor();
void TestSetStrokeColor();
void TestSetLineWidth();
void TestSetFont();

// 3. 基础绘制测试
void TestFillRect();
void TestDrawCircle();
void TestDrawLine();

// 4. 文本绘制测试
void TestDrawText();
void TestMeasureText();

// 5. 图片绘制测试
void TestDrawImage();

// 6. 辅助方法测试
void TestClear();
void TestSaveToFile();

// 7. 状态管理测试
void TestSaveRestore();
void TestTransform();

// 8. 集成测试
void TestCompleteScene();
```

**检查点**:
- [ ] 所有测试编译通过
- [ ] 所有测试运行通过
- [ ] 生成的图片正确

#### Task 3.2: 编译和运行测试 (10 分钟)
```bash
# 编译
cmake --build build --target test_unified_renderer --config Debug -j 8

# 运行
.\build\bin\Debug\test_unified_renderer.exe
```

**检查点**:
- [ ] 编译无错误
- [ ] 所有测试通过
- [ ] 无内存泄漏

---

### Phase 4: JavaScript 绑定 (1 小时)

#### Task 4.1: 实现 QuickJS 绑定 (40 分钟)
**文件**: `core/render/unified_renderer_bindings.h/cpp`

**绑定方法**:
```cpp
class UnifiedRendererBindings {
public:
    static void Register(JSContext* ctx, JSValue global_obj);
    
private:
    // 构造函数
    static JSValue CreateRenderer(JSContext* ctx, JSValueConst this_val, 
                                   int argc, JSValueConst* argv);
    
    // 状态设置
    static JSValue SetFillColor(JSContext* ctx, JSValueConst this_val, 
                                int argc, JSValueConst* argv);
    static JSValue SetStrokeColor(...);
    static JSValue SetLineWidth(...);
    static JSValue SetFont(...);
    
    // 绘制方法
    static JSValue FillRect(...);
    static JSValue DrawCircle(...);
    static JSValue DrawText(...);
    static JSValue DrawImage(...);
    
    // 辅助方法
    static JSValue Clear(...);
    static JSValue SaveToFile(...);
    
    // 辅助函数
    static UnifiedRenderer* GetRenderer(JSContext* ctx, JSValueConst obj);
    static void RendererFinalizer(JSRuntime* rt, JSValue val);
};
```

**检查点**:
- [ ] 所有方法正确绑定
- [ ] 参数转换正确
- [ ] 错误处理完善
- [ ] 内存管理正确

#### Task 4.2: 创建 JavaScript 示例 (10 分钟)
**文件**: `examples/unified_renderer_example.js`

**示例内容**:
```javascript
// 创建渲染器
const renderer = createRenderer(800, 600);

// 设置样式
renderer.setFillColor("#3498db");
renderer.setStrokeColor("#e74c3c");
renderer.setLineWidth(3);

// 绘制图形
renderer.fillRect(50, 50, 200, 150);
renderer.drawCircle(400, 300, 75);

// 绘制文本
renderer.setFont("Arial", 24);
renderer.setFillColor("#000000");
renderer.drawText("Hello, LightUI!", 50, 500);

// 保存
renderer.saveToFile("output.png");
```

**检查点**:
- [ ] 示例代码简洁易懂
- [ ] 演示所有主要功能
- [ ] 能正确运行

#### Task 4.3: 创建绑定测试 (10 分钟)
**文件**: `tests/test_unified_renderer_bindings.cpp`

**测试内容**:
- [ ] 测试 JavaScript 调用所有方法
- [ ] 测试参数验证
- [ ] 测试错误处理
- [ ] 测试内存管理

**检查点**:
- [ ] 所有测试通过
- [ ] 无内存泄漏
- [ ] 生成的图片正确

---

### Phase 5: 文档更新 (20 分钟)

#### Task 5.1: 更新 PHASE_2_3_PLAN.md
- [ ] 标记 Task 9 为完成 ✅
- [ ] 更新总体进度 (78.4% → 90%+)
- [ ] 添加 UnifiedRenderer 说明

#### Task 5.2: 更新 README.md
- [ ] 更新进度徽章
- [ ] 添加 UnifiedRenderer 到特性列表

#### Task 5.3: 创建重构报告
**文件**: `UNIFIED_RENDERER_REFACTORING_REPORT.md`

**内容**:
- [ ] 重构原因
- [ ] 重构方案
- [ ] API 对比
- [ ] 使用示例
- [ ] 测试结果

---

## 📊 进度跟踪

### 时间分配

| Phase | 任务 | 预计时间 | 实际时间 | 状态 |
|-------|------|---------|---------|------|
| 1 | 设计和准备 | 30 分钟 | - | ⏳ |
| 2 | 实现 UnifiedRenderer | 1.5 小时 | - | ⏳ |
| 3 | 测试验证 | 30 分钟 | - | ⏳ |
| 4 | JavaScript 绑定 | 1 小时 | - | ⏳ |
| 5 | 文档更新 | 20 分钟 | - | ⏳ |
| **总计** | | **3.5 小时** | - | ⏳ |

### 里程碑

- [ ] **M1**: UnifiedRenderer 实现完成
- [ ] **M2**: 单元测试全部通过
- [ ] **M3**: JavaScript 绑定完成
- [ ] **M4**: 示例运行成功
- [ ] **M5**: 文档更新完成

---

## 🎯 成功标准

### 功能完整性
- [ ] UnifiedRenderer 支持所有基础绘制功能
- [ ] 状态管理正确（Paint、Font）
- [ ] SaveToFile 能正确保存 PNG
- [ ] JavaScript 绑定完整

### 代码质量
- [ ] 所有代码编译无警告
- [ ] 所有测试通过
- [ ] 无内存泄漏
- [ ] 代码风格统一

### 文档完整性
- [ ] API 文档完整
- [ ] 使用示例清晰
- [ ] 重构报告详细

---

## 🚀 执行建议

### 执行顺序
1. **先实现核心** - Phase 1-2（UnifiedRenderer 实现）
2. **验证功能** - Phase 3（测试）
3. **完成绑定** - Phase 4（JavaScript）
4. **更新文档** - Phase 5

### 风险控制
- ✅ **不破坏现有代码** - 只新增，不修改
- ✅ **增量开发** - 每个 Phase 独立验证
- ✅ **完整测试** - 每步都有测试覆盖

### 回滚方案
如果重构失败：
1. 删除新增的文件
2. 恢复 CMakeLists.txt
3. Phase 2.3 仍然是 78.4% 完成

---

## 📝 下一步行动

**准备好开始了吗？**

我建议按以下顺序执行：

1. ✅ **确认计划** - 你同意这个重构计划吗？
2. 🚀 **开始 Phase 1** - 创建文件结构
3. 💻 **实现 Phase 2** - 编写 UnifiedRenderer
4. ✅ **验证 Phase 3** - 运行测试
5. 🔗 **完成 Phase 4** - JavaScript 绑定
6. 📚 **更新 Phase 5** - 文档

**要现在开始吗？** 我可以立即开始实现！
