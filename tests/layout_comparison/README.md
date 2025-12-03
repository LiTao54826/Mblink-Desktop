# Layout Comparison Test / 布局比较测试

用于比较 MBink 和浏览器渲染结果的测试工具。

## 📁 文件结构

```
tests/layout_comparison/
├── README.md                     # 本说明文件
├── CMakeLists.txt               # CMake 构建配置
├── ifc_test_cases.html          # 测试用例 HTML
├── layout_extractor.js          # 浏览器端布局提取脚本
├── layout_comparison_test.cpp   # MBink 测试应用
├── compare_layouts.py           # 比较工具脚本
├── browser_layout_data.json     # 浏览器布局数据（从浏览器导出）
└── mbink_layout_data.json       # MBink 布局数据（测试生成）
```

## 🚀 使用方法

### 步骤 1: 获取浏览器布局数据

1. 在浏览器中打开 `ifc_test_cases.html`
2. 打开开发者工具 (F12)
3. 在控制台执行：
   ```javascript
   LayoutExtractor.output()
   ```
4. 点击页面右上角的 "Download Layout Data" 按钮
5. 保存为 `browser_layout_data.json`

### 步骤 2: 运行 MBink 测试

```bash
# 编译测试
cmake --build build --config Release --target test_layout_comparison

# 运行测试
./build/bin/Release/test_layout_comparison.exe
```

这会生成 `mbink_layout_data.json` 文件。

### 步骤 3: 比较布局数据

```bash
python tests/layout_comparison/compare_layouts.py \
    browser_layout_data.json \
    mbink_layout_data.json
```

可选参数：添加容差值（默认 2px）
```bash
python compare_layouts.py browser.json mbink.json 5
```

## 📊 输出报告

比较工具会输出：
- 匹配率
- 差异列表（位置和尺寸差异）
- 缺失/多余的元素
- JSON 格式的详细报告

示例输出：
```
============================================================
    LAYOUT COMPARISON REPORT
============================================================

📊 Summary:
   Total elements:    36
   Matched:          28
   Mismatched:       5
   Missing in MBink: 3
   Extra in MBink:   0
   Match rate:       77.78%
   Tolerance:        2.0px

❌ Differences (8):
------------------------------------------------------------
   🟠 [test4-item1] MISMATCH:
      - width: browser=80.00, mbink=100.00, diff=20.00
      - height: browser=40.00, mbink=20.00, diff=20.00
   🔴 [test1-span] MISSING - Element 'test1-span' not found in MBink data
```

## 🧪 测试用例说明

| 测试 | 说明 | 测试点 |
|------|------|--------|
| test1 | 基本文本流 | 文本渲染、行高 |
| test2 | 文本换行 | 自动换行 |
| test3 | 内联元素 | margin/padding |
| test4 | inline-block | 块级尺寸 |
| test5 | 垂直对齐 | vertical-align |
| test6 | 首行缩进 | text-indent |
| test7 | 字符间距 | letter-spacing |
| test8 | 单词间距 | word-spacing |
| test9 | 文本对齐 | text-align |
| test10 | 空白处理 | white-space |
| test11 | 混合内容 | 文本+图片 |
| test12 | 嵌套内联 | 嵌套 span |

## 🔧 扩展测试用例

1. 在 `ifc_test_cases.html` 中添加新的测试 section
2. 给需要比较的元素添加 `data-test="唯一ID"` 属性
3. 重新运行测试

## 📝 注意事项

- 浏览器和 MBink 使用相同的视口尺寸（800x600）
- 比较时使用绝对位置（相对于视口）
- 默认容差为 2px，可调整
- 字体渲染可能导致细微差异

## 🐛 已知差异

测试发现的主要差异：
1. **span 元素尺寸为 0**: MBink 内联元素布局信息未正确填充
2. **inline-block 尺寸不正确**: 边框计算问题
3. **相对位置计算**: 需要调整父元素偏移累加

这些是 IFC 实现需要继续完善的地方。

