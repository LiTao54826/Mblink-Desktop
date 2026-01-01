# Implementation Plan

## 重要：执行环境和日志处理规范

**⚠️ 当前系统环境：Windows (cmd shell)**

**禁止使用 Linux/Unix 命令！** 必须使用 Windows CMD 命令：
- ❌ `rm` → ✅ `del`（删除文件）或 `rmdir /s /q`（删除目录）
- ❌ `cat` → ✅ `type`（查看文件内容）
- ❌ `grep` → ✅ `findstr`（搜索文本）
- ❌ `ls` → ✅ `dir`（列出目录）
- ❌ `mkdir -p` → ✅ `mkdir`（创建目录）
- ❌ `cp` → ✅ `copy`（复制文件）
- ❌ `mv` → ✅ `move`（移动文件）
- ❌ `&&` → ✅ `&`（命令连接符）
- ❌ `/path/to/file` → ✅ `path\to\file`（路径分隔符用反斜杠）

**所有任务在运行测试或执行命令时，必须遵循以下规范：**

1. **禁止直接读取控制台输出** - 大量日志会占用上下文，影响执行效率
2. **输出重定向到文件** - 使用 `>> debuglog.txt` 将输出追加到日志文件
3. **条件筛选获取结果** - 使用 `findstr` 过滤关键信息，不要读取完整日志
4. **及时清理日志文件** - 测试完成后使用 `del debuglog.txt` 删除日志

**标准测试流程（Windows CMD）：**
```cmd
REM 1. 运行测试，输出到文件（添加 -q 5 自动退出）
build\bin\Release\esm_loader.exe tests\js\test_xxx.js -q 5 >> debuglog.txt

REM 2. 筛选检查结果（只看关键信息）
findstr "TEST_PASS TEST_FAIL ERROR" debuglog.txt

REM 3. 清理日志文件
del debuglog.txt
```

**编译检查流程（Windows CMD）：**
```cmd
REM 1. 编译，输出到文件
cmake --build build --config Release --target esm_loader >> buildlog.txt 2>&1

REM 2. 筛选检查错误
findstr /i "error warning" buildlog.txt

REM 3. 清理日志文件
del buildlog.txt
```

---

## Phase 1: 创建 PaintLayer 核心类

- [ ] 1. 创建 PaintLayer 头文件和基础结构
  - [ ] 1.1 创建 `core/render/layer/paint_layer.h`
    - 定义 PaintLayer 类
    - 包含 RenderObject 关联、树结构（parent_、children_）
    - 包含 z-order 列表（pos_z_order_list_、neg_z_order_list_）
    - 包含 compositing 相关成员（compositor_layer_、promotion_reason_）
    - 声明所有公共方法
    - _Requirements: 1.1, 1.2, 2.1_
  - [ ] 1.2 创建 `core/render/layer/paint_layer.cpp` 基础实现
    - 实现构造函数、析构函数
    - 实现树操作方法（AddChild、RemoveChild、InsertBefore）
    - 复用现有 Layer 的树管理逻辑
    - _Requirements: 1.1_
  - [ ]* 1.3 编写 PaintLayer 树结构单元测试
    - 测试 AddChild、RemoveChild、InsertBefore
    - _Requirements: 1.1_

- [ ] 2. 实现 Stacking Context 判断
  - [ ] 2.1 实现 IsStackingContext() 方法
    - 检查 position + z-index 条件
    - 检查 opacity < 1 条件
    - 检查 transform != none 条件
    - 检查 filter != none 条件
    - 检查 will-change 条件
    - _Requirements: 1.1, 3.1_
  - [ ] 2.2 实现 StackingContext() 和 ZIndex() 方法
    - 向上查找最近的 stacking context 祖先
    - 从 RenderObject 的 ComputedStyle 获取 z-index
    - _Requirements: 1.1, 3.1_
  - [ ]* 2.3 编写 Stacking Context 属性测试
    - **Property 2: Stacking Context 创建正确性**
    - **Validates: Requirements 1.1**

- [ ] 3. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 2: 实现 Z-Order 列表管理

- [ ] 4. 实现 Z-Order 列表
  - [ ] 4.1 实现 UpdateZOrderLists() 方法
    - 收集所有需要参与排序的子层
    - 分离正负 z-index 到两个列表
    - 复用现有 Layer::EnsureSorted 的排序逻辑
    - _Requirements: 1.2_
  - [ ] 4.2 实现 CollectZOrderLayers() 私有方法
    - 递归收集子层
    - 跳过自身是 stacking context 的子层（它们有自己的排序）
    - _Requirements: 1.2_
  - [ ] 4.3 实现 SortZOrderLists() 私有方法
    - 按 z-index 升序排序
    - 相同 z-index 按文档顺序
    - _Requirements: 1.2, 4.4_
  - [ ]* 4.4 编写 Z-Order 列表属性测试
    - **Property 3: Z-order 列表维护正确性**
    - **Validates: Requirements 1.2**

- [ ] 5. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 3: 实现绘制功能

- [ ] 6. 实现 Paint 方法
  - [ ] 6.1 实现 Paint() 主方法
    - 按 CSS stacking context 规则绘制
    - 顺序：背景 → 负 z-index → 内容 → 正 z-index
    - 复用现有 Layer::Paint 的绘制逻辑
    - _Requirements: 1.3_
  - [ ] 6.2 实现 PaintContents() 方法
    - 调用 RenderObject::Paint()
    - 处理 CompositorLayer 的情况
    - _Requirements: 1.3, 2.3_
  - [ ] 6.3 实现 PaintNegativeZOrderChildren() 和 PaintPositiveZOrderChildren()
    - 遍历 z-order 列表绘制子层
    - _Requirements: 1.3_
  - [ ]* 6.4 编写绘制顺序属性测试
    - **Property 4: 绘制顺序正确性**
    - **Validates: Requirements 1.3**

- [ ] 7. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 4: 实现 Hit Testing

- [ ] 8. 实现 HitTest 方法
  - [ ] 8.1 实现 HitTest() 主方法
    - 按 z-order 逆序测试（从高到低）
    - 先测试正 z-index 子层，再测试自身，最后测试负 z-index 子层
    - 复用现有 Layer::HitTest 的核心逻辑
    - _Requirements: 4.1, 4.2_
  - [ ] 8.2 实现 HitTestChildren() 私有方法
    - 递归测试子层
    - 处理滚动偏移坐标变换
    - 复用现有 Layer::HitTestRenderObject 逻辑
    - _Requirements: 4.3, 4.4_
  - [ ] 8.3 实现 HandleWheel() 方法
    - 处理滚轮事件
    - 复用现有 Layer::HandleWheel 逻辑
    - _Requirements: 4.3_
  - [ ]* 8.4 编写 Hit Testing 属性测试
    - **Property 7: Hit Testing 逆序遍历**
    - **Property 8: 滚动偏移坐标变换**
    - **Property 9: 同 z-index 文档顺序**
    - **Validates: Requirements 4.1, 4.2, 4.3, 4.4**

- [ ] 9. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 5: 实现 Compositing 功能

- [ ] 10. 实现 Compositing 判断和关联
  - [ ] 10.1 实现 NeedsCompositing() 方法
    - 检查 will-change: transform/opacity
    - 检查 position: fixed
    - 检查活动的 transform/opacity 动画
    - 检查可滚动容器
    - 复用现有 LayerTreeBuilder::ShouldPromote 逻辑
    - _Requirements: 6.1, 6.2, 6.3, 6.4_
  - [ ] 10.2 实现 GetPromotionReason() 方法
    - 返回具体的提升原因
    - 复用现有 LayerPromotionReason 枚举
    - _Requirements: 6.5_
  - [ ] 10.3 实现 EnsureCompositorLayer() 方法
    - 按需创建 CompositorLayer
    - 设置 CompositorLayer 的属性
    - _Requirements: 2.1, 2.2_
  - [ ]* 10.4 编写 Compositing 属性测试
    - **Property 5: CompositorLayer 不影响绘制顺序**
    - **Property 10: Compositing 提升规则**
    - **Validates: Requirements 2.1, 2.2, 2.3, 6.1, 6.2, 6.3, 6.4**

- [ ] 11. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 6: 集成到渲染管线

- [ ] 12. 修改 RenderObject 支持 PaintLayer
  - [ ] 12.1 在 RenderObject 中添加 paint_layer_ 成员
    - 添加 GetPaintLayer()、EnsurePaintLayer()、NeedsPaintLayer() 方法
    - 在需要时懒创建 PaintLayer
    - _Requirements: 1.1_
  - [ ] 12.2 实现 PaintLayer 树同步构建
    - 在 RenderObject 树变化时同步更新 PaintLayer 树
    - 在 AppendChild/RemoveChild 时更新 PaintLayer 树
    - _Requirements: 7.2_

- [ ] 13. 修改 Window 使用 PaintLayer
  - [ ] 13.1 修改 Window::Paint() 使用 PaintLayer
    - 从根 PaintLayer 开始绘制
    - 移除对 LayerManager::BeginFrame() 的调用
    - 移除对 LayerManager::PaintLayers() 的调用
    - _Requirements: 1.3_
  - [ ] 13.2 修改 hit_testing.cpp 使用 PaintLayer
    - 从根 PaintLayer 开始 hit testing
    - 移除对 LayerManager::HitTest() 的调用
    - _Requirements: 4.1, 4.2_
  - [ ] 13.3 修改 mouse_event_dispatcher.cpp 使用 PaintLayer
    - 移除对 LayerManager::HitTest() 的调用
    - _Requirements: 4.1, 4.2_

- [ ] 14. 修改 RenderBlock 和 RenderInline
  - [ ] 14.1 修改 render_block.cpp
    - 移除 LayerManager::ShouldCollect() 和 Collect() 调用
    - 使用 PaintLayer 树结构
    - _Requirements: 1.1, 1.3_
  - [ ] 14.2 修改 render_inline.cpp
    - 移除 LayerManager::ShouldCollect() 和 Collect() 调用
    - 使用 PaintLayer 树结构
    - _Requirements: 1.1, 1.3_

- [ ] 15. 处理 position:fixed 元素
  - [ ] 15.1 实现 fixed 元素的特殊处理
    - fixed 元素的 PaintLayer 参与根 stacking context
    - 实现 IsFixedPositioned() 和 GetAbsolutePosition() 方法
    - 复用现有 Layer::AddItem 中的 fixed 处理逻辑
    - _Requirements: 3.1, 3.2, 3.3, 3.4_
  - [ ]* 15.2 编写 fixed 元素属性测试
    - **Property 6: Fixed 元素 stacking context 归属**
    - **Validates: Requirements 3.1, 3.4**

- [ ] 16. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 7: 增量更新和调试

- [ ] 17. 实现增量更新
  - [ ] 17.1 实现样式变化时的增量更新
    - 只更新受影响的 PaintLayer
    - 使用 DirtyZOrderLists() 标记
    - _Requirements: 7.1, 7.3_
  - [ ]* 17.2 编写增量更新属性测试
    - **Property 11: 增量更新范围**
    - **Validates: Requirements 7.1, 7.2, 7.3**

- [ ] 18. 实现调试支持
  - [ ] 18.1 实现 ToDebugString() 和 DumpTree() 方法
    - 输出层级结构、z-index、stacking context 状态
    - 输出 compositing 状态和原因
    - _Requirements: 8.3, 8.4_
  - [ ]* 18.2 编写层检查信息属性测试
    - **Property 12: 层检查信息完整性**
    - **Validates: Requirements 8.3**

- [ ] 19. 实现序列化 round-trip（可选）
  - [ ]* 19.1 实现 PaintLayer 树序列化和解析
    - **Property 1: PaintLayer 树 round-trip 一致性**
    - **Validates: Requirements 1.4, 1.5**

- [ ] 20. Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 8: 清理旧代码

- [ ] 21. 删除 LayerManager 系统
  - [ ] 21.1 删除旧文件
    - 删除 `core/render/layer/layer_manager.h`
    - 删除 `core/render/layer/layer_manager.cpp`
    - 删除 `core/render/layer/layer.h`
    - 删除 `core/render/layer/layer.cpp`
    - _Requirements: 5.1_
  - [ ] 21.2 更新 CMakeLists.txt 和引用
    - 更新 `core/render/layer/CMakeLists.txt`
    - 更新所有 include 语句
    - _Requirements: 5.1_
  - [ ] 21.3 更新测试文件
    - 更新 `tests/property/render/test_layer_system_properties.cpp`
    - 更新或删除 `tests/js/test_layer_manager.js`
    - _Requirements: 5.1_
  - [ ] 21.4 更新文档
    - 更新 `core/render/layer/README.md`
    - 更新 `docs/LAYER_SYSTEM_DESIGN.md`
    - _Requirements: 8.4_

- [ ] 22. Final Checkpoint - 确保所有测试通过
  - Ensure all tests pass, ask the user if questions arise.

## Phase 9: UI 集成测试

- [ ] 23. 编写并运行 UI 集成测试
  - [ ] 23.1 创建 `tests/js/test_paint_layer.js` 测试文件
    - 测试 PaintLayer 基础功能
    - 测试 stacking context 创建
    - 测试 z-index 排序
    - 使用 `[TEST_PASS]`/`[TEST_FAIL]` 标记输出
    - _Requirements: 1.1, 1.2, 1.3_
  - [ ] 23.2 创建 `tests/js/test_hit_testing_layers.js` 测试文件
    - 测试 hit testing 逆序遍历
    - 测试滚动容器内的 hit testing
    - 测试同 z-index 文档顺序
    - _Requirements: 4.1, 4.2, 4.3, 4.4_
  - [ ] 23.3 创建 `tests/js/test_fixed_position_layer.js` 测试文件
    - 测试 position:fixed 元素的 stacking context 归属
    - 测试 fixed 元素的 hit testing
    - _Requirements: 3.1, 3.2, 3.3, 3.4_
  - [ ] 23.4 运行所有 UI 测试并验证通过
    - 使用 `build\bin\Release\esm_loader.exe tests\js\test_paint_layer.js -q 5 >> debuglog.txt`
    - 使用 `build\bin\Release\esm_loader.exe tests\js\test_hit_testing_layers.js -q 5 >> debuglog.txt`
    - 使用 `build\bin\Release\esm_loader.exe tests\js\test_fixed_position_layer.js -q 5 >> debuglog.txt`
    - 使用 `findstr "TEST_FAIL TEST_PASS" debuglog.txt` 检查结果
    - 确保所有测试通过后删除 `debuglog.txt`
    - _Requirements: 1.1, 1.2, 1.3, 3.1, 3.2, 3.3, 3.4, 4.1, 4.2, 4.3, 4.4_

- [ ] 24. 创建高 z-index 元素测试（Toast、Modal、Overlay）
  - [ ] 24.1 创建 `tests/js/test_toast_modal_overlay.js` 测试文件
    - 测试 Toast 元素（z-index >= 100）的绘制顺序和 hit testing
    - 测试 Modal 元素（z-index >= 1000）的绘制顺序和 hit testing
    - 测试 Overlay 元素覆盖普通内容
    - 测试多个高 z-index 元素之间的层叠顺序
    - 使用 `[TEST_PASS]`/`[TEST_FAIL]` 标记输出
    - _Requirements: 5.2, 5.3, 5.4_
  - [ ] 24.2 测试 Toast 场景
    - 创建普通内容 + Toast 通知
    - 验证 Toast 始终显示在普通内容之上
    - 验证点击 Toast 区域能正确响应
    - 验证点击 Toast 外区域能穿透到下层
    - _Requirements: 5.2, 4.1, 4.2_
  - [ ] 24.3 测试 Modal 场景
    - 创建普通内容 + Modal 对话框 + 遮罩层
    - 验证 Modal 显示在所有普通内容之上
    - 验证遮罩层阻止下层元素的点击
    - 验证 Modal 内部元素的 hit testing 正确
    - _Requirements: 5.2, 5.3, 4.1, 4.2_
  - [ ] 24.4 测试多层叠加场景
    - 创建 普通内容 + Overlay + Toast + Modal 的复杂场景
    - 验证绘制顺序：普通内容 → Overlay → Toast → Modal
    - 验证 hit testing 顺序：Modal → Toast → Overlay → 普通内容
    - _Requirements: 1.3, 4.1, 4.2, 5.2, 5.3, 5.4_
  - [ ] 24.5 运行 Toast/Modal/Overlay 测试
    - 使用 `build\bin\Release\esm_loader.exe tests\js\test_toast_modal_overlay.js -q 5 >> debuglog.txt`
    - 使用 `findstr "TEST_FAIL TEST_PASS" debuglog.txt` 检查结果
    - 确保所有测试通过后删除 `debuglog.txt`
    - _Requirements: 5.2, 5.3, 5.4_

- [ ] 25. Final UI Test Checkpoint
  - Ensure all UI tests pass with `[TEST_PASS]` markers
  - No `[TEST_FAIL]` markers in output
  - Ask the user if questions arise
