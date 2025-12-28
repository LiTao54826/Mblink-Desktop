# Property Tree Nodes

属性树节点类型定义。

## 文件说明

| 文件 | 职责 |
|------|------|
| `property_tree_node.h` | 属性树节点基类模板（CRTP） |
| `transform_tree_node.*` | 变换属性节点（位移、旋转、缩放） |
| `clip_tree_node.*` | 裁剪属性节点 |
| `effect_tree_node.*` | 效果属性节点（透明度、滤镜、混合模式） |
| `scroll_tree_node.*` | 滚动属性节点 |

## 设计原理

每种属性类型形成独立的树结构，渲染对象通过索引引用节点，实现：
- 属性继承的高效计算
- 增量更新
- 内存共享
