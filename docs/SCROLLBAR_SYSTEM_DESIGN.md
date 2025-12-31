# 滚动条系统统一设计

## 当前问题

滚动条处理逻辑分散在多个地方，导致：
1. body 的滚动条处理与普通元素不一致
2. 布局时滚动条宽度计算重复
3. 绘制时滚动条逻辑重复
4. 水平滚动条在有垂直滚动条时错误出现

## 当前代码分布

### 布局相关
- `native_layout_engine.cpp::ComputeLayoutInternal` - body 滚动条检测（特殊处理）
- `native_layout_engine.cpp::ComputeNodeLayout` - 普通元素 overflow:auto 滚动条检测
- `block_layout.cpp::ComputeScrollbarGutter` - 计算滚动条占用空间
- `native_layout_engine.cpp::ConvertStyle` - overflow:scroll 时设置 scrollbar_width

### 绘制相关
- `scrollbar_painter.cpp` - 通用滚动条绘制器
- `render_block.cpp::Paint` - block 元素滚动条绘制
- `render_inline_block.cpp::PaintTextarea` - textarea 滚动条绘制

### 滚动计算
- `render_object.cpp::GetMaxScrollX/Y` - 计算最大滚动范围
- `render_object.cpp::HitTestScrollbar` - 滚动条点击检测
- `scrollbar_controller.cpp` - 滚动条拖动控制

## 统一设计方案

### 核心原则（参考 Blink）
1. 滚动条在元素内部显示（Chrome 行为）
2. 元素宽度 = 视口/容器宽度
3. 内容区域宽度 = 元素宽度 - padding - border - scrollbar_gutter
4. 所有元素（包括 body）使用相同的滚动条处理逻辑

### 开发计划

#### Phase 1: 统一滚动条检测逻辑
**目标**: 移除 body 的特殊处理，让所有元素使用相同的滚动条检测流程

**修改文件**:
- `native_layout_engine.cpp::ComputeLayoutInternal` - 移除 body 特殊滚动条处理
- `native_layout_engine.cpp::ComputeNodeLayout` - 增强滚动条检测，支持 root 节点

**关键改动**:
1. `ComputeNodeLayout` 中的滚动条检测不再跳过 root_node_
2. 使用 `available_space.height` 作为容器高度判断依据
3. 确保 `scrollbar_width` 正确传递给 `ComputeBlockLayout`

#### Phase 2: 修复 block_layout 中的 scrollbar_gutter 计算
**目标**: 确保 scrollbar_gutter 正确减少内容区域宽度

**修改文件**:
- `block_layout.cpp::ComputeScrollbarGutter` - 验证逻辑正确性
- `block_layout.cpp::ComputeBlockLayoutInner` - 确保 content_box_inset 正确应用

#### Phase 3: 清理调试代码
**目标**: 移除所有调试 printf 语句

**修改文件**:
- `native_layout_engine.cpp` - 移除 printf/fflush

#### Phase 4: 添加 JS API 绑定
**目标**: 提供 scrollWidth, scrollHeight, scrollTop, scrollLeft 等 API

**新增文件**:
- `core/api/scroll_api.cpp` - 滚动相关 API 实现

#### Phase 5: 测试验证
**目标**: 确保滚动条功能正常

**测试用例**:
1. body overflow:auto 垂直滚动条
2. body overflow:auto 无水平滚动条（内容宽度 < 视口宽度 - 滚动条宽度）
3. 普通 div overflow:auto 滚动条
4. overflow:scroll 强制显示滚动条

## 实现细节

### Phase 1 详细设计

当前 `ComputeLayoutInternal` 中的 body 滚动条检测流程：
```
1. 第一次布局：scrollbar_width = 0
2. 检查 content_height > available_height
3. 如果需要滚动条：设置 scrollbar_width，清除缓存，重新布局
```

问题：这个逻辑与 `ComputeNodeLayout` 中的逻辑重复，且 root 节点被跳过。

解决方案：
1. 移除 `ComputeLayoutInternal` 中的特殊处理
2. 修改 `ComputeNodeLayout` 中的条件 `node_id != root_node_` 为允许 root 节点
3. 使用 `inputs.available_space.height` 作为容器高度判断

### 滚动条检测统一逻辑

```cpp
// 在 ComputeNodeLayout 中，对所有 overflow:auto/scroll 元素：
if (overflow_y == "auto" || overflow_x == "auto") {
    // 获取容器高度：优先使用 available_space，其次使用 output.size
    float container_height = inputs.available_space.height.IsDefinite() 
        ? inputs.available_space.height.value 
        : output.size.height;
    
    // 检查是否需要垂直滚动条
    if (overflow_y == "auto" && output.content_size.height > container_height) {
        node->style.scrollbar_width = scrollbar_width;
        needs_relayout = true;
    }
    
    // 重新布局
    if (needs_relayout) {
        node->cache.Clear();
        output = ComputeBlockLayout(node_id, inputs);
    }
}
```

## 验收标准

1. body 设置 overflow:auto 时，垂直滚动条正常显示
2. 内容宽度不超过 (视口宽度 - 滚动条宽度) 时，不显示水平滚动条
3. 所有元素的滚动条行为一致
4. 无调试日志输出
5. JS API 可正常获取/设置滚动位置
