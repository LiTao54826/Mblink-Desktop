# Style Tests | 样式测试

## Overview | 概览

`tests/style_tests/` contains helper materials for observing style and layout behavior.
`tests/style_tests/` 包含用于观察样式系统和布局行为的辅助测试材料。

## Best Interpreted As | 更准确的定位

This directory is better treated as:
这部分内容更适合作为：

- manual verification scripts / 手工验证脚本
- browser comparison materials / 浏览器对比素材
- style or layout issue reproductions / 样式或布局问题复现样例

rather than a fully maintained and automatically validated formal test suite.
而不是已经稳定维护且自动验证通过的正式测试套件。

## Typical Contents | 常见内容

- `.js` test scripts / `.js` 测试脚本
- `.html` comparison pages / `.html` 对比页面
- local launch scripts / 本地启动脚本
- batch run helpers / 批量运行脚本

## Recommended Workflow | 建议流程

1. open the corresponding HTML page in a browser
   先在浏览器中打开对应 HTML 页面
2. run the related JS script with `esm_loader`
   再用 `esm_loader` 运行对应 JS 脚本
3. compare layout, scrolling, text, color, and interaction differences
   比较布局、滚动、文本、颜色与交互差异

## Non-Claims | 不应直接承诺的内容

- all style tests are automated / 所有样式测试都已自动化
- all scenarios match browsers exactly / 所有场景都与浏览器完全一致
- all scripts are currently verified / 所有脚本当前都已验证通过