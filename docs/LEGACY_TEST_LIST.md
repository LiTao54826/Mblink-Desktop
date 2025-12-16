# 历史测试与示例清单

> 由于项目更新迭代和重构，原有测试和示例存在历史回归问题，已于 2024-12-16 清理。
> 本文档保留原有测试名单，供后续重新编写时参考。

---

## Examples 目录

### C++ 示例 (examples/cpp/)
- `animation_demo.cpp` - 动画演示
- `counter_app.cpp` - 计数器应用
- `dom_example.cpp` / `dom_example.js` - DOM 示例
- `hello_world.cpp` - Hello World
- `html_loading_example.cpp` - HTML 加载示例
- `html_window_example.cpp` - HTML 窗口示例
- `integration_example.cpp` - 集成示例
- `javascript_integration_example.cpp` - JavaScript 集成示例
- `render_example.cpp` - 渲染示例

### Python 示例 (examples/python/)
- `hello_world.py` - Hello World
- `todo_app.py` - Todo 应用

### Preact 示例
- `preact_counter/` - Preact 计数器
- `preact_form_demo/` - Preact 表单演示
- `preact_hello_world/` - Preact Hello World
- `preact_todo_app/` - Preact Todo 应用
- `preact_window_demo/` - Preact 窗口演示

### HTML/JS 演示 (examples/demo_html/)

#### Canvas 测试
- `canvas_advanced_test.js` - Canvas 高级测试
- `canvas_charts_demo.js` - Canvas 图表演示
- `canvas_composite_test.js` - Canvas 合成测试
- `canvas_debug.js` - Canvas 调试
- `canvas_fillrect_test.js` - Canvas fillRect 测试
- `canvas_gradient_test.js` - Canvas 渐变测试
- `canvas_imagedata_test.js` - Canvas ImageData 测试
- `canvas_integration_test.js` - Canvas 集成测试
- `canvas_linestyle_test.js` - Canvas 线条样式测试
- `canvas_minimal.js` - Canvas 最小测试
- `canvas_path_test.js` - Canvas 路径测试
- `canvas_read_test.js` - Canvas 读取测试
- `canvas_test.js` - Canvas 测试
- `canvas_text_test.js` - Canvas 文本测试
- `canvas_transform_test.js` - Canvas 变换测试
- `canvas_ultra_minimal.js` - Canvas 超简测试
- `canvas_write_test.js` - Canvas 写入测试

#### Chart.js 测试
- `chartjs_debug.js`
- `chartjs_detailed_debug.js`
- `chartjs_nomodule_test.js`
- `chartjs_simple_debug.js`
- `chartjs_test_simple.js`
- `chartjs_test.js`

#### Preact 测试
- `preact_counter.js` / `preact_counter_enhanced.js`
- `preact_simple_test.js`
- `preact_test_suite.js`
- `preact_todo_enhanced.js`
- `preact_ui_component_test.js`

#### DOM/事件测试
- `classlist_test.js` - classList 测试
- `dom_traversal_test.js` - DOM 遍历测试
- `query_selector_test.js` - querySelector 测试
- `test_addEventListener.js` - addEventListener 测试
- `test_click_event.js` - 点击事件测试
- `test_createTextNode.js` - createTextNode 测试
- `test_dispatchEvent.js` - dispatchEvent 测试
- `test_setTimeout.js` - setTimeout 测试
- `test_textUpdate.js` - 文本更新测试

#### 布局测试 (demo_html/flexbox_test/)
- `comprehensive_test.html` - 综合测试
- `flexbox_test.html` - Flexbox 测试
- `grid_test.html` - Grid 测试
- `position_test.html` - Position 测试

#### 布局对比测试 (demo_html/layout_compare_test/)
- `app.js` / `app_advanced.js`
- `browser.html` / `browser_advanced.html`
- `collect_browser_data.html`
- `scroll_test.js`

#### 其他测试
- `api_check.js` - API 检查
- `new_apis_test.js` - 新 API 测试
- `pattern_conic_test.js/.html` - 锥形渐变测试
- `roundrect_test.js` - 圆角矩形测试
- `simple_test_app.js` - 简单测试应用
- `test_bindings_app.js` - 绑定测试应用
- `todo_app.js` - Todo 应用

### 模块测试 (examples/module_test/)
- `main.js` - 主入口
- `math.js` - 数学模块
- `advanced-test.js` - 高级测试
- `folder-test.js` - 文件夹测试
- `config/` - 配置模块
- `helpers/` - 辅助模块
- `libs/` - 库模块
- `utils/` - 工具模块

### 综合测试应用 (examples/comprehensive_test_app/)
- `test_framework.js` - 测试框架
- `tests/attribute_tests.js` - 属性测试
- `tests/dom_tests.js` - DOM 测试
- `tests/event_tests.js` - 事件测试
- `tests/form_tests.js` - 表单测试
- `tests/html_tests.js` - HTML 测试
- `tests/query_tests.js` - 查询测试
- `tests/timer_tests.js` - 定时器测试

### 其他示例
- `animation_demo/` - 动画演示 (空)
- `clean_loader_demo/` - 清洁加载器演示
- `desktop_app_example/` - 桌面应用示例
- `element_test/` - 元素测试
- `flexbox_test/` - Flexbox 测试
- `html_tags_test/` - HTML 标签测试
- `input_test_app/` - 输入测试应用
- `layout_compare_test/` - 布局对比测试
- `performance_test/` - 性能测试 (空)
- `sdl_test/` - SDL 测试
- `table_test_app/` - 表格测试应用
- `window_demo/` - 窗口演示
- `incremental_render_test.cpp` - 增量渲染测试

---

## Tests 目录

### 单元测试 (tests/unit/)

#### CSS 相关
- `test_advanced_css.cpp` - 高级 CSS
- `test_cascade_engine.cpp` - 级联引擎
- `test_css_filters.cpp` - CSS 滤镜
- `test_css_rendering.cpp` - CSS 渲染
- `test_css_variables.cpp` - CSS 变量
- `test_css3_pseudo_classes.cpp` - CSS3 伪类
- `test_css3_pseudo_elements.cpp` - CSS3 伪元素
- `test_css3_selectors.cpp` - CSS3 选择器

#### DOM 相关
- `test_dom_document.cpp` - DOM 文档
- `test_dom_event.cpp` - DOM 事件
- `test_dom_html_content.cpp` - DOM HTML 内容
- `test_dom_lexbor_integration.cpp` - DOM Lexbor 集成
- `test_dom_node.cpp` - DOM 节点
- `test_dom_query.cpp` - DOM 查询

#### HTML 元素
- `test_html_anchor_element.cpp` - 锚点元素
- `test_html_button_element.cpp` - 按钮元素
- `test_html_container_elements.cpp` - 容器元素
- `test_html_form_element.cpp` - 表单元素
- `test_html_image_element.cpp` - 图片元素
- `test_html_label_element.cpp` - 标签元素
- `test_html_select_element.cpp` - 选择元素

#### HTML5 相关
- `test_html5_error_handling.cpp` - HTML5 错误处理
- `test_html5_features.cpp` - HTML5 特性
- `test_html5_parser.cpp` - HTML5 解析器
- `test_html5_performance.cpp` - HTML5 性能

#### 表单相关
- `test_form_elements.cpp` - 表单元素
- `test_form_state.cpp` - 表单状态
- `test_form_validation.cpp` - 表单验证

#### 渲染相关
- `test_render_engine.cpp` - 渲染引擎
- `test_render_tree.cpp` - 渲染树
- `test_unified_renderer.cpp` - 统一渲染器
- `test_incremental_rendering.cpp` - 增量渲染

#### QuickJS 相关
- `test_quickjs.cpp` - QuickJS 测试
- `test_quickjs_c.c` - QuickJS C 测试
- `test_quickjs_runtime.cpp` - QuickJS 运行时

#### Lexbor 相关
- `test_lexbor_document.cpp` - Lexbor 文档
- `test_lexbor_stylesheet.cpp` - Lexbor 样式表

#### 其他单元测试
- `test_dirty_marking.cpp` - 脏标记
- `test_event_loop.cpp` - 事件循环
- `test_frame_controller.cpp` - 帧控制器
- `test_ifc.cpp` - IFC 测试
- `test_input_handler.cpp` - 输入处理器
- `test_integration.cpp` - 集成测试
- `test_javascript_bindings.cpp` - JavaScript 绑定
- `test_memory_leak_fix.cpp` - 内存泄漏修复
- `test_object_cache.cpp` - 对象缓存
- `test_performance_optimization.cpp` - 性能优化
- `test_remove_event_listener.cpp` - 移除事件监听器
- `test_skia_text_simple.cpp` - Skia 文本简单测试
- `test_style_cache.cpp` - 样式缓存
- `test_style_manager.cpp` - 样式管理器
- `test_task_scheduler.cpp` - 任务调度器
- `test_window_timeout.cpp` - 窗口超时
- `test_window.cpp` - 窗口测试
- `test_hello.cpp` / `test_minimal.cpp` / `test_simple.cpp` - 基础测试

### 渲染测试 (tests/render/)
- `test_animation_controller.cpp` - 动画控制器
- `test_animation_timeline.cpp` - 动画时间线
- `test_animation.cpp` - 动画
- `test_css_integration.cpp` - CSS 集成
- `test_gradient_renderer.cpp` - 渐变渲染器
- `test_keyframes.cpp` - 关键帧
- `test_property_interpolation.cpp` - 属性插值
- `test_shadow_renderer.cpp` - 阴影渲染器
- `test_text_shadow.cpp` - 文本阴影
- `test_transform.cpp` - 变换
- `test_transition.cpp` - 过渡

### 集成测试 (tests/integration/)
- `test_dom_bindings_integration.cpp` - DOM 绑定集成
- `test_dom_integration.cpp` - DOM 集成
- `test_innerHTML_debug.cpp` - innerHTML 调试
- `test_long_running.cpp` - 长时间运行测试
- `test_preact_basic.cpp` - Preact 基础
- `test_preact_components.cpp` - Preact 组件
- `test_preact_integration.cpp` - Preact 集成
- `test_preact_render.cpp` - Preact 渲染
- `test_real_world_app.cpp` - 真实应用测试
- `test_stress.cpp` - 压力测试

### 性能测试 (tests/performance/)
- `test_dom_performance.cpp` - DOM 性能
- `test_form_performance.cpp` - 表单性能
- `test_layout_performance.cpp` - 布局性能
- `test_memory_stress.cpp` - 内存压力
- `test_parsing_performance.cpp` - 解析性能
- `test_selector_performance.cpp` - 选择器性能
- `performance_utils.h` - 性能工具

### 基准测试 (tests/benchmark/ & tests/benchmarks/)
- `benchmark_css_animations.cpp` - CSS 动画基准
- `benchmark_dom.cpp` - DOM 基准
- `benchmark_rendering.cpp` - 渲染基准
- `benchmark_timer_performance.cpp` - 定时器性能基准
- `test_advanced_optimization.cpp` - 高级优化测试
- `test_optimization.cpp` - 优化测试

### DevTools 测试 (tests/devtools/)
- `test_devtools_improvements.cpp` - DevTools 改进
- `test_devtools_properties.cpp` - DevTools 属性
- `test_dom_tree_view.cpp` - DOM 树视图

### 布局对比测试 (tests/layout_comparison/)
- `layout_comparison_test.cpp` - 布局对比测试
- `compare_layouts.py` - 布局对比脚本
- `compare_advanced.py` - 高级对比脚本
- `compare_unified.py` - 统一对比脚本
- `layout_extractor.js` - 布局提取器
- 相关 HTML 和 JSON 数据文件

### DOM 渲染对比测试 (tests/dom_render_comparison/)
- `dom_render_test.cpp` - DOM 渲染测试
- `compare_render.py` - 渲染对比脚本
- `extract_json.py` - JSON 提取脚本
- `save_browser_data.py` - 浏览器数据保存脚本
- 相关测试用例和输出文件

### 根目录测试文件
- `react_binding_test.cpp` - React 绑定测试
- `react_dom_test.js` - React DOM 测试
- `react_event_test.js` - React 事件测试
- `test_js_value_wrapper.cpp` - JS 值包装器测试
- `test_use_effect.cpp` - useEffect 测试
- `inspect.py` / `scratch.py` - 辅助脚本

---

## 测试分类汇总

| 类别 | 数量 | 说明 |
|------|------|------|
| 单元测试 | 54 | 核心功能单元测试 |
| 渲染测试 | 11 | 渲染相关测试 |
| 集成测试 | 10 | 模块集成测试 |
| 性能测试 | 7 | 性能相关测试 |
| 基准测试 | 6 | 性能基准测试 |
| DevTools 测试 | 3 | 开发工具测试 |
| 布局对比测试 | 多个 | 与浏览器布局对比 |
| DOM 渲染对比 | 多个 | DOM 渲染对比 |
| Examples | 100+ | 各类示例和演示 |

---

## 重建建议

1. 优先重建核心单元测试（DOM、CSS、渲染）
2. 建立自动化测试框架
3. 添加 CI/CD 集成
4. 使用统一的测试命名规范
5. 分离单元测试和集成测试
6. 建立性能基准线
