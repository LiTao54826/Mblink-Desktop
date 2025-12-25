# 滚动条视口宽度计算问题修复总结

## 🔍 问题分析

### 症状
在 `sidebar_app.js` 示例的 Layout Tests 页面中：
1. **内容区域没有减去滚动条的宽度** - 导致内容的右侧被滚动条遮挡
2. **滚动功能异常** - 滚动范围计算不正确

### 根本原因

在分层渲染架构中，`ScrollLayerManager` 负责管理滚动容器的视口和滚动范围。问题出现在两个关键函数中：

**位置**: `core/compositor/scroll_layer_manager.cpp`

```cpp
// ❌ 错误的实现（修复前）
bool ScrollLayerManager::RegisterScrollContainer(RenderObject* container) {
    // ...
    const auto& layout = container->GetLayoutInfo();
    info.viewport_width = layout.width;      // 直接使用布局宽度
    info.viewport_height = layout.height;    // 直接使用布局高度
    // ...
}
```

**问题**: 
- `layout.width` 和 `layout.height` 是元素的总宽度/高度
- **没有减去 border 宽度**
- **没有减去滚动条宽度（12px）**
- 导致视口尺寸比实际可滚动区域大

### 对比: RenderObject::Paint 的正确实现

在 `RenderObject::Paint` 函数中（第1900-1929行），有正确的滚动条宽度计算逻辑：

```cpp
// ✅ 正确的实现（Paint阶段）
float visible_width = effective_width - box.border_left_width - box.border_right_width;
float visible_height = effective_height - box.border_top_width - box.border_bottom_width;

bool needs_v_scroll = allow_v_scroll && (content_height > visible_height || overflow_y == "scroll");

float content_area_width = visible_width;
if (needs_v_scroll) {
    content_area_width -= scrollbar_width;  // ✅ 正确减去滚动条宽度
}
```

### 不一致性

- **Layout阶段**: `layout.width` / `layout.height` 不包含滚动条扣除
- **Paint阶段**: 动态计算 `content_area_width` / `content_area_height`，**正确减去滚动条宽度**
- **Compositor阶段**: `ScrollLayerManager` 使用 layout 阶段的尺寸，**没有减去滚动条宽度** ❌

## ✅ 解决方案

### 修复内容

修改了 `ScrollLayerManager` 中的两个关键函数，使其与 `RenderObject::Paint` 的逻辑保持一致：

#### 1. `RegisterScrollContainer` (第43-108行)

```cpp
// ✅ 修复后的实现
bool ScrollLayerManager::RegisterScrollContainer(RenderObject* container) {
    // ...
    
    // 获取布局和样式信息
    const auto& layout = container->GetLayoutInfo();
    const auto& style = container->GetComputedStyle();
    
    // 获取 border 宽度（从ComputedStyle）
    float border_left = style.border_left_width > 0 ? style.border_left_width : style.border.width.ToPx();
    float border_right = style.border_right_width > 0 ? style.border_right_width : style.border.width.ToPx();
    float border_top = style.border_top_width > 0 ? style.border_top_width : style.border.width.ToPx();
    float border_bottom = style.border_bottom_width > 0 ? style.border_bottom_width : style.border.width.ToPx();
    
    // 计算可见区域（减去border）
    float visible_width = layout.width - border_left - border_right;
    float visible_height = layout.height - border_top - border_bottom;
    
    // 获取overflow设置
    std::string overflow_x = !style.overflow_x.empty() ? style.overflow_x : style.overflow;
    std::string overflow_y = !style.overflow_y.empty() ? style.overflow_y : style.overflow;
    
    // 判断是否允许滚动
    bool allow_v_scroll = (overflow_y == "scroll" || overflow_y == "auto");
    bool allow_h_scroll = (overflow_x == "scroll" || overflow_x == "auto");
    
    // 计算内容尺寸
    info.content_width = container->GetContentWidth();
    info.content_height = container->GetContentHeight();
    
    // 计算是否需要滚动条（与Paint逻辑一致）
    const float scrollbar_width = 12.0f;
    bool needs_v_scroll = allow_v_scroll && (info.content_height > visible_height || overflow_y == "scroll");
    
    float content_area_width = visible_width;
    if (needs_v_scroll) {
        content_area_width -= scrollbar_width;  // ✅ 减去垂直滚动条宽度
    }
    
    bool needs_h_scroll = allow_h_scroll && (info.content_width > content_area_width || overflow_x == "scroll");
    
    float content_area_height = visible_height;
    if (needs_h_scroll) {
        content_area_height -= scrollbar_width;  // ✅ 减去水平滚动条高度
        // 重新检查是否需要垂直滚动条（水平滚动条可能导致需要垂直滚动条）
        if (allow_v_scroll && !needs_v_scroll && info.content_height > content_area_height) {
            needs_v_scroll = true;
            content_area_width = visible_width - scrollbar_width;
            // 重新检查水平滚动条
            needs_h_scroll = allow_h_scroll && info.content_width > content_area_width;
        }
    }
    
    // ✅ 使用计算后的尺寸作为视口尺寸
    info.viewport_width = content_area_width;
    info.viewport_height = content_area_height;
    
    // ...
}
```

#### 2. `UpdateContentSize` (第224-276行)

应用与 `RegisterScrollContainer` 完全相同的逻辑。

#### 3. `EventLoop::HandleMouseWheelEventForDOM` (核心事件处理)

修复了在分层合成模式下，滚动事件直接调用 `RenderObject::ScrollBy` 而没有更新 `ScrollLayerManager` 的问题。

```cpp
// ✅ 修复后的实现 (event_loop.cpp)
// 关键修复：在分层合成模式下，需要通过compositor处理滚动
// 这样ScrollLayerManager中的层偏移才会正确更新
auto compositor_adapter = window->GetCompositorAdapter();
bool scrolled = false;

if (compositor_adapter) {
    // 分层合成模式：通过compositor处理滚动
    scrolled = compositor_adapter->HandleScroll(render_obj.get(), scroll_delta_x, scroll_delta_y);
} else {
    // 传统模式：直接修改RenderObject
    render_obj->ScrollBy(scroll_delta_x, scroll_delta_y);
    scrolled = true;
}
```

### 关键改进点

1. **减去 border 宽度**: 从 `ComputedStyle` 获取准确的 border 宽度
2. **正确计算滚动条**: 判断是否需要滚动条，并相应减去滚动条宽度（12px）
3. **处理互相影响**: 水平/垂直滚动条可能互相影响，需要重新检查
4. **与Paint逻辑一致**: 确保compositor和render阶段使用相同的计算方法
5. **EventLoop路由修复**: 确保滚动事件正确路由到 compositor，更新层偏移状态

## 📊 修复效果

### 修复前
- ❌ 内容宽度 = layout.width（例如：400px）
- ❌ 实际可见区域 = 400px - 12px = 388px （Compositor使用400px，认为没有滚动条）
- ❌ 滚动操作无效（EventLoop直接修改RenderObject，Compositor不知道）

### 修复后
- ✅ Compositor视口 = 正确计算（388px）
- ✅ 滚动操作有效（EventLoop通过Compositor处理，层偏移更新）
- ⚠️ **遗留问题**: 布局引擎给内容分配了400px宽度（没有减去滚动条），导致最右侧12px内容被滚动条遮挡。这是Flex布局引擎的缺陷，需要单独修复。

## 🧪 测试验证

### 测试用例
```bash
build\bin\Release\esm_loader.exe .\examples\preact_demo\sidebar_app.js
```

### 验证步骤
1. 启动 sidebar_app.js
2. 点击 "Layout Tests" 菜单项
3. 检查内容区域是否被滚动条遮挡
4. 测试滚动功能是否正常
5. 测试不同 overflow 设置（auto, scroll）

##影响范围

在启用分层渲染后，所有具有 `overflow: auto` 或 `overflow: scroll` 的元素都受此修复影响：

- ✅ **修复前（非分层模式）**: 正确，因为在Paint时动态计算
- ✅ **修复后（分层模式）**: 正确，现在与Paint逻辑一致

## 📝 技术细节

### 为什么需要从 ComputedStyle 获取 border？

最初尝试使用 `container->GetBox()` 获取 border 宽度，但发现：
- `RenderObject` 没有 `GetBox()` 公共方法
- 必须从 `ComputedStyle` 获取 border 信息
- 使用 `style.border_*_width > 0 ? style.border_*_width : style.border.width.ToPx()` 逻辑

### 滚动条宽度常量

```cpp
const float scrollbar_width = 12.0f;  // 与 RenderObject::Paint 保持一致
```

## 📋 文件修改清单

- `core/compositor/scroll_layer_manager.cpp`
  - `RegisterScrollContainer()` - 修复视口计算
  - `UpdateContentSize()` - 修复视口计算
- `core/event/event_loop.cpp`
  - `HandleMouseWheelEventForDOM()` - 修复滚动事件路由，支持分层合成模式

## ✨ 总结

此修复解决了两部分问题：
1. **视口计算**: ScrollLayerManager现在正确计算扣除滚动条后的视口大小。
2. **滚动交互**: EventLoop现在正确通过Compositor处理滚动事件，使滚动生效。

遗留的布局遮挡问题（内容宽度未减去滚动条宽度）是一个独立的布局引擎缺陷，建议在后续任务中修复。当前修复已确保滚动功能可用。
