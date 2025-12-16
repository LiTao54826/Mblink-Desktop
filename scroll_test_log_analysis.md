## 滚动条计算问题 - 测试日志分析

基于实际运行日志，我发现了关键信息：

### 初始状态日志

```
[Paint Update #1] content_height_: 0 -> 650.203

[GetMaxScrollY #1] 
  effective_width=400, effective_height=300
  borders: L=0, R=0, T=0, B=0
  visible_width=400, visible_height=300
  content_width_=352, content_height_=650.203
  using_cached=1, content_height=650.203
  needs_v_scroll=1, needs_h_scroll=0
  available_height=300, max_scroll=350.203
```

### 关键观察

1. **content_height_ 在 Paint时更新为 650.203**
2. **effective_height = 300** (这可能是问题所在！)
3. **body CSS设置的是 `height: 400px`，但effective_height却是300**
4. **content_height使用的是缓存值 (using_cached=1)**
5. **max_scroll = 350.203** (650.203 - 300 = 350.203)

### 疑问

为什么 `effective_height` 是 300 而不是 400？

这需要进一步查看 `GetEffectiveVisibleHeight()` 的实现。

### 下一步

需要您手动操作窗口来触发问题：
1. 启动程序
2. 调整窗口大小  
3. 滚动鼠标
4. 观察日志中 `effective_height` 和 `content_height_` 的变化

如果能看到：
- resize后 effective_height 变化
- 但 content_height_ 没有立即更新
- 滚动时使用旧的 content_height_

那就证实了问题所在。
