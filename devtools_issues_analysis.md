# DevTools 问题分析

## 问题 1：body元素高亮位置计算不对

### 根本原因

在 `element_highlighter.cpp` 的 `RenderBoxModelHighlight` 中，计算绝对位置的逻辑有问题：

```cpp
// 第184-206行的 find_render_object lambda
float current_x = offset_x + layout.x;
float current_y = offset_y + layout.y;

// 子元素偏移
float child_offset_x = current_x - obj->GetScrollX();
float child_offset_y = current_y - obj->GetScrollY();
```

**问题**：
- Body 元素本身作为滚动容器，它的布局位置(layout.x, layout.y)是相对于视口的
- 但在递归查找时，`offset_x` 和 `offset_y` 初始值是 0
- 当 body 滚动时，body内的子元素位置受 `scroll_y_` 影响
- 但 body 元素自身的高亮矩形应该始终在视口位置，不应受滚动影响

### 修复方案

需要区分两种情况：
1. **Body元素自身的高亮**：使用 layout 位置，不考虑滚动
2. **Body子元素的高亮**：需要考虑body的滚动偏移

```cpp
// 伪代码修复
if (element == body_element) {
    // Body自身不需要考虑滚动偏移
    float x = layout.x;  // 通常是8 (margin)
    float y = layout.y;  // 通常是8 (margin)
} else {
    // 子元素需要考虑祖先的滚动
    float x = abs_x - parent_scroll_x;
    float y = abs_y - parent_scroll_y;
}
```

## 问题 2：元素选择器无法选中动态添加的元素

### 根本原因

`element_picker.cpp` 第131-138行：

```cpp
std::shared_ptr<Element> ElementPicker::HitTest(int x, int y) {
    if (!document_) return nullptr;
    
    // TODO: 实现真正的命中测试
    // 暂时返回 body
    return document_->GetBody();
}
```

**问题**：
- `HitTest` 方法只是返回body，完全没有实现
- 无论点击哪里，都只能选中body
- 动态添加的元素无法被选中

### 修复方案

需要实现真正的命中测试：
1. 从渲染树遍历所有可见元素
2. 检查鼠标坐标是否在元素的边界框内
3. 返回最深层（z-index最高）的命中元素

```cpp
std::shared_ptr<Element> ElementPicker::HitTest(int x, int y) {
    // 获取渲染树
    auto& wm = WindowManager::Instance();
    auto windows = wm.GetAllWindows();
    if (windows.empty()) return nullptr;
    
    auto window = windows[0];
    auto root_render = window->GetCachedRenderTree();
    if (!root_render) return nullptr;
    
    // 递归查找命中的元素（从后向前，优先命中上层元素）
    std::shared_ptr<Element> hit_element;
    std::function<void(std::shared_ptr<RenderObject>, float, float)> traverse;
    traverse = [&](std::shared_ptr<RenderObject> obj, float offset_x, float offset_y) {
        if (!obj) return;
        
        const auto& layout = obj->GetLayoutInfo();
        float abs_x = offset_x + layout.x;
        float abs_y = offset_y + layout.y;
        
        // 检查是否命中当前元素
        SkRect bounds = SkRect::MakeXYWH(abs_x, abs_y, layout.width, layout.height);
        if (bounds.contains(x, y)) {
            auto node = obj->GetNode();
            if (node && node->GetNodeType() == NodeType::ELEMENT_NODE) {
                hit_element = std::static_pointer_cast<Element>(node);
            }
        }
        
        // 递归检查子元素（考虑滚动）
        float child_offset_x = abs_x - obj->GetScrollX();
        float child_offset_y = abs_y - obj->GetScrollY();
        
        for (const auto& child : obj->GetChildren()) {
            traverse(child, child_offset_x, child_offset_y);
        }
    };
    
    traverse(root_render, 0, 0);
    return hit_element;
}
```

## 下一步行动

1. 修复 `element_highlighter.cpp` 中的高亮位置计算
2. 实现 `element_picker.cpp` 中的真正命中测试
3. 测试验证两个问题都已解决
