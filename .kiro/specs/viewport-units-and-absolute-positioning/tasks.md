# 实现计划

- [x] 1. 调查和诊断当前视口单位处理问题






  - [x] 1.1 添加调试日志追踪视口单位解析路径

    - 在 `CSSLength::ToPx()` 中添加 vh/vw 单位使用时的日志
    - 在 `ViewportSize::Set()` 中添加日志确认被调用
    - 在 native_layout_engine.cpp 的 `ConvertLength()` 中添加日志
    - _需求: 1.1, 2.1_

  - [x] 1.2 创建最小测试用例复现视口单位问题

    - 创建使用 `height: 100vh` 的测试 HTML/JS 文件
    - 在 DevTools 中验证计算后的高度值
    - _需求: 1.1_



- [x] 2. 修复原生布局引擎中的视口单位解析



  - [x] 2.1 更新 ConvertLength 处理视口单位


    - 在 ConvertLength() 中添加 CSSUnit::VH, VW, VMIN, VMAX 的处理
    - 使用 ViewportSize::GetWidth()/GetHeight() 进行解析
    - _需求: 1.1, 2.1, 3.1, 3.2_
  - [x] 2.2 编写视口高度单位解析的属性测试


    - **属性 1: 视口高度单位解析**
    - **验证: 需求 1.1, 1.2**

  - [x] 2.3 编写视口宽度单位解析的属性测试
    - **属性 2: 视口宽度单位解析**
    - **验证: 需求 2.1, 2.2**
  - [x] 2.4 确保 ViewportSize 在布局前初始化


    - 验证 window.cpp 中的调用顺序
    - 如果视口大小未设置，添加断言或回退值

    - _需求: 1.3, 2.3_
  - [x] 2.5 编写视口单位在窗口调整大小时重新计算的属性测试
    - **属性 3: 视口单位在调整大小时重新计算**
    - **验证: 需求 1.3, 2.3**

- [x] 3. 检查点 - 验证视口单位正常工作





  - 确保所有测试通过，如有问题询问用户。

- [x] 4. 修复 vmin/vmax 单位解析
  - [x] 4.1 在 ConvertLength 中添加 vmin/vmax 处理
    - 计算视口尺寸的最小/最大值
    - 应用百分比计算
    - _需求: 3.1, 3.2_
    - ✅ 已在 CSSLength::ToPx() 中实现 VMIN/VMAX 处理
  - [x] 4.2 编写 vmin/vmax 解析的属性测试
    - **属性 5: vmin/vmax 解析**
    - **验证: 需求 3.1, 3.2**
    - ✅ 5 个属性测试通过: VminResolution, VmaxResolution, Vmin100EqualsMinDimension, Vmax100EqualsMaxDimension, VminVmaxOrientationChange

- [x] 5. 调查绝对定位的 bottom/right 问题






  - [x] 5.1 在绝对定位代码中添加调试日志

    - 在 PerformAbsoluteLayoutOnAbsoluteChildren 中记录 inset 值
    - 记录 area_size 和 area_offset 值
    - 记录计算后的位置值
    - _需求: 4.1, 4.2_

  - [x] 5.2 创建绝对定位 bottom/right 的测试用例

    - 创建使用 `position: absolute; bottom: 0; right: 0` 的测试
    - 在 DevTools 中验证元素位置
    - _需求: 4.1, 4.2_


- [-] 6. 修复绝对定位 inset 计算


  - [ ] 6.1 验证绝对元素的包含块计算


    - 检查 area_size 是否从包含块正确计算
    - 处理包含块是视口的情况（没有定位祖先）
    - _需求: 4.1, 4.2, 6.1_
  - [ ] 6.2 编写绝对定位 bottom inset 的属性测试
    - **属性 6: 绝对定位的 bottom inset**
    - **验证: 需求 4.1, 4.5**
  - [ ] 6.3 编写绝对定位 right inset 的属性测试
    - **属性 7: 绝对定位的 right inset**
    - **验证: 需求 4.2**
  - [ ] 6.4 修复百分比值的 inset 解析
    - 确保百分比 inset 相对于包含块解析
    - _需求: 6.3_



- [ ] 7. 检查点 - 验证基本绝对定位正常工作









  - 确保所有测试通过，如有问题询问用户。

- [ ] 8. 修复绝对定位拉伸行为







  - [x] 8.1 实现 top+bottom 的高度拉伸


    - 当同时设置 top 和 bottom 但没有显式高度时
    - 计算高度为 container_height - top - bottom - margins
    - _需求: 4.3_
  - [x] 8.2 编写 top+bottom 拉伸的属性测试


    - **属性 8: 绝对定位的 top+bottom 拉伸**
    - **验证: 需求 4.3**
  - [x] 8.3 实现 left+right 的宽度拉伸

    - 当同时设置 left 和 right 但没有显式宽度时
    - 计算宽度为 container_width - left - right - margins
    - _需求: 4.4_
  - [x] 8.4 编写 left+right 拉伸的属性测试

    - **属性 9: 绝对定位的 left+right 拉伸**
    - **验证: 需求 4.4**

- [x] 9. 验证 flex 容器中的绝对定位





  - [x] 9.1 测试 flex 容器中的绝对定位


    - 验证 flex_layout.cpp 正确处理 insets
    - 与 block 布局行为对比
    - _需求: 6.2_
  - [x] 9.2 编写跨布局模式一致性的属性测试

    - **属性 10: 跨布局模式的绝对定位一致性**
    - **验证: 需求 6.1, 6.2**


- [x] 10. 集成测试和清理




  - [x] 10.1 使用视口单位测试 sidebar_app.js


    - 修改 sidebar_app.js 使用 `height: 100vh`
    - 验证全高布局正常工作
    - _需求: 1.1, 5.1_
  - [x] 10.2 测试绝对定位演示


    - 创建包含各种绝对定位场景的演示
    - 验证所有 inset 组合正常工作
    - _需求: 4.1, 4.2, 4.3, 4.4_
  - [x] 10.3 移除调试日志


    - 移除步骤 1.1 和 5.1 中添加的临时日志
    - _需求: 全部_


- [x] 11. 最终检查点 - 确保所有测试通过













  - 确保所有测试通过，如有问题询问用户。
