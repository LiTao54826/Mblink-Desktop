# Taffy 布局引擎集成计划

## 📋 概述

将 Taffy CSS 布局引擎集成到 MBink 中，实现完整的 CSS 布局支持（Block + Flexbox + Grid），达到与 Chrome 一致的渲染效果。

---

## 🎯 目标

1. ✅ **完整的 CSS 布局支持**
   - Block Layout（块级布局）
   - Flexbox Layout（弹性盒布局）
   - CSS Grid Layout（网格布局）

2. ✅ **W3C 规范兼容**
   - 严格遵循 W3C CSS 规范
   - 与 Chrome 渲染效果一致

3. ✅ **无 Rust 依赖**（对开发者）
   - 使用预编译库
   - 开发者无需安装 Rust

---

## 🏗️ 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    MBink Application                     │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                   StyleResolver                          │
│  - 解析 CSS 属性                                         │
│  - 构建 ComputedStyle                                    │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                   LayoutEngine                           │
│  - 管理 Taffy 布局树                                     │
│  - 同步 DOM 树到 Taffy 树                                │
│  - 调用 Taffy 计算布局                                   │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│                   Taffy C API                            │
│  - TaffyTree (布局树)                                    │
│  - TaffyStyle (样式)                                     │
│  - TaffyLayout (布局结果)                                │
└─────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────┐
│              Taffy Core (Rust, 预编译)                   │
│  - Block 布局算法                                        │
│  - Flexbox 布局算法                                      │
│  - Grid 布局算法                                         │
└─────────────────────────────────────────────────────────┘
```

### 核心类设计

#### 1. LayoutEngine (新增)

```cpp
// core/layout/layout_engine.h
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();
    
    // 从 DOM 树构建布局树
    void BuildLayoutTree(Element* root);
    
    // 计算布局
    void ComputeLayout(float available_width, float available_height);
    
    // 获取布局结果
    LayoutInfo GetLayoutInfo(Element* element);
    
    // 同步样式更新
    void UpdateStyle(Element* element, const ComputedStyle& style);
    
private:
    TaffyTree* taffy_tree_;
    std::unordered_map<Element*, TaffyNodeId> element_to_node_;
    std::unordered_map<TaffyNodeId, Element*> node_to_element_;
    
    // 辅助方法
    TaffyNodeId CreateNode(Element* element);
    void ApplyStyle(TaffyNodeId node, const ComputedStyle& style);
    void SyncChildren(Element* element, TaffyNodeId node);
};
```

#### 2. ComputedStyle (扩展)

```cpp
// core/render/computed_style.h
struct ComputedStyle {
    // Display
    Display display = Display::Block;  // Block, Flex, Grid, Inline, None
    
    // Flexbox
    FlexDirection flex_direction = FlexDirection::Row;
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    JustifyContent justify_content = JustifyContent::FlexStart;
    AlignItems align_items = AlignItems::Stretch;
    AlignContent align_content = AlignContent::Stretch;
    AlignSelf align_self = AlignSelf::Auto;
    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    CSSLength flex_basis = CSSLength::Auto();
    
    // Grid
    GridAutoFlow grid_auto_flow = GridAutoFlow::Row;
    std::vector<GridTrack> grid_template_rows;
    std::vector<GridTrack> grid_template_columns;
    CSSLength gap = CSSLength::Zero();
    CSSLength row_gap = CSSLength::Zero();
    CSSLength column_gap = CSSLength::Zero();
    
    // Positioning
    Position position = Position::Static;  // Static, Relative, Absolute, Fixed
    CSSLength top = CSSLength::Auto();
    CSSLength right = CSSLength::Auto();
    CSSLength bottom = CSSLength::Auto();
    CSSLength left = CSSLength::Auto();
    
    // Box Model
    CSSLength width = CSSLength::Auto();
    CSSLength height = CSSLength::Auto();
    CSSLength min_width = CSSLength::Auto();
    CSSLength min_height = CSSLength::Auto();
    CSSLength max_width = CSSLength::Auto();
    CSSLength max_height = CSSLength::Auto();
    
    CSSEdges margin;
    CSSEdges padding;
    CSSEdges border_width;
    
    // ... 其他属性
};
```

#### 3. RenderObject (简化)

```cpp
// core/render/render_object.h
class RenderObject {
public:
    // 移除 Yoga 相关代码
    // 布局由 LayoutEngine 统一管理
    
    void SetComputedStyle(const ComputedStyle& style);
    const ComputedStyle& GetComputedStyle() const;
    
    void SetLayoutInfo(const LayoutInfo& info);
    const LayoutInfo& GetLayoutInfo() const;
    
private:
    ComputedStyle computed_style_;
    LayoutInfo layout_info_;
    // 移除 YGNodeRef yoga_node_;
};
```

---

## 📝 实施步骤

### 阶段 1: 准备工作 ✅

- [x] 创建 `third_party/taffy` 目录结构
- [x] 准备 README 和文档
- [x] 设计架构

### 阶段 2: 编译 Taffy C Bindings

- [ ] 克隆 Taffy 仓库（c-bindings 分支）
- [ ] 编译 Windows 版本
- [ ] 编译 Linux 版本（可选）
- [ ] 编译 macOS 版本（可选）
- [ ] 复制 `taffy.h` 到 `third_party/taffy/include/`
- [ ] 复制编译好的库到 `third_party/taffy/lib/`

### 阶段 3: 实现 LayoutEngine

- [ ] 创建 `core/layout/layout_engine.h`
- [ ] 创建 `core/layout/layout_engine.cpp`
- [ ] 实现 Taffy 树的创建和管理
- [ ] 实现 DOM 树到 Taffy 树的同步
- [ ] 实现布局计算接口

### 阶段 4: 扩展 ComputedStyle

- [ ] 添加所有 Flexbox 属性
- [ ] 添加所有 Grid 属性
- [ ] 添加 Positioning 属性
- [ ] 更新 CSS 枚举定义

### 阶段 5: 更新 StyleResolver

- [ ] 解析 `display` 属性（支持 flex, grid）
- [ ] 解析所有 Flexbox 属性
- [ ] 解析所有 Grid 属性
- [ ] 解析 Positioning 属性

### 阶段 6: 集成到渲染管线

- [ ] 在 `Document` 中集成 `LayoutEngine`
- [ ] 在样式更新时同步到 Taffy
- [ ] 在 DOM 变化时同步到 Taffy
- [ ] 从 Taffy 读取布局结果

### 阶段 7: 更新 CMakeLists.txt

- [ ] 添加 Taffy 头文件路径
- [ ] 链接 Taffy 库
- [ ] 配置不同平台的库路径

### 阶段 8: 测试

- [ ] 测试 Block 布局
- [ ] 测试 Flexbox 布局（修复 todo list）
- [ ] 测试 Grid 布局
- [ ] 测试嵌套布局
- [ ] 测试定位

---

## 🔧 CMakeLists.txt 配置

```cmake
# third_party/taffy/CMakeLists.txt
add_library(taffy STATIC IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(taffy PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/taffy.lib"
    )
elseif(APPLE)
    set_target_properties(taffy PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/libtaffy.a"
    )
else()
    set_target_properties(taffy PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/libtaffy.a"
    )
endif()

target_include_directories(taffy INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

```cmake
# core/layout/CMakeLists.txt
add_library(lightui_layout STATIC
    layout_engine.cpp
)

target_link_libraries(lightui_layout PUBLIC
    taffy
    lightui_dom
)

target_include_directories(lightui_layout PUBLIC
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party/taffy/include
)
```

---

## ✅ 成功标准

1. **Flexbox 正常工作**
   - window_demo 的 todo list 布局正确
   - `display: flex`, `justify-content`, `align-items` 等属性生效

2. **Block 布局正常工作**
   - 普通的块级元素垂直堆叠
   - 内联元素水平排列

3. **Grid 布局正常工作**
   - `display: grid` 生效
   - `grid-template-columns/rows` 正确

4. **与 Chrome 一致**
   - 相同的 HTML/CSS 在 MBink 和 Chrome 中渲染效果一致

---

## 📊 预期影响

### 体积增加
- **Taffy 库**: ~300-500 KB
- **新增代码**: ~2000 行（LayoutEngine + 扩展）

### 性能
- **布局计算**: 与 Yoga 相当或更快
- **内存占用**: 略有增加（维护 Taffy 树）

### 开发体验
- ✅ 开发者无需 Rust
- ✅ 完整的 CSS 支持
- ✅ 与 Chrome 一致的效果

---

## 🚀 下一步

等待 Rust 环境准备完成后：
1. 编译 Taffy C bindings
2. 开始实施阶段 3-8

