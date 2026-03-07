# CSS 布局引擎对比测试报告

## 测试概览

| 项目 | 说明 |
|------|------|
| 测试脚本 | `tests/css_compare/run_compare.py` |
| 测试用例 | `tests/css_compare/css_test_cases.html` |
| 测试元素数 | 78 个（含父子容器） |
| 对比容差 | ±5px（可通过 `--tolerance` 调整） |
| 对比策略 | **相对坐标**（不对比绝对 width/height） |

## 运行方法

```powershell
cd d:\code\C\MBink

# 完整对比（MBink vs Chrome）
.venv\Scripts\python.exe tests/css_compare/run_compare.py

# 调整容差
.venv\Scripts\python.exe tests/css_compare/run_compare.py --tolerance 2

# 仅查看 MBink 采集数据（不启动 Chrome）
.venv\Scripts\python.exe tests/css_compare/run_compare.py --no-chrome
```

## 对比策略说明

| 检查项 | 说明 |
|--------|------|
| `relX/relY` | 元素相对最近父 testid 容器的偏移（核心，消除字体累积误差） |
| `centerOffX/Y` | 元素中心相对父容器中心的偏差（检测 align-items/justify-content 居中） |
| `overflowX/Y` | 内容溢出状态（布尔值，必须两端一致） |
| 兄弟间距 | 同父容器下相邻子项的水平间距差（不依赖绝对宽度） |
| 根级绝对 x/y | 无父 testid 的顶级元素使用绝对坐标（viewport 已对齐） |

> **不对比 width/height**：字体渲染差异导致文字高度略有不同，直接对比尺寸会误报。

## 测试结果（2026-02-25）

```
✅ PASS : 67 / 78
⚠️ WARN :  6 / 78
❌ FAIL :  5 / 78
```

### ✅ PASS 的场景

- 盒模型（content-box / border-box）
- Flex Row / Column 布局
- justify-content（flex-start/center/flex-end/space-between/space-around）
- align-items（flex-start/center/flex-end/stretch）
- flex-grow / flex-ratio / flex-wrap
- Position relative + absolute
- 嵌套卡片布局（nested）
- min-width / max-height

### ⚠️ WARN（轻微偏差，在容差范围内）

根级容器 Y 坐标累积偏差约 1.5~3px，来源于字体行高在两端的微小差异：

| 元素 | 偏差 |
|------|------|
| `flex-grow/container` | Y +1.5px |
| `flex-ratio/container` | Y +3px |
| `flex-wrap/container` | Y +3px |
| `pos/abs-container` | Y +3px |
| `pos/relative-parent` | Y +3px |
| `minmax/min-width` | Y +3px |

### ❌ FAIL（确认 Bug）

| 元素 | 差异 | 根因 |
|------|------|------|
| `display/inline-1` | Y +30.5px | **IFC 行高计算偏小约 30px** |
| `display/inline-2` | X +20.4px, Y +30.5px | inline 宽度 + 累积 Y 偏差 |
| `display/block-1` | Y +34.5px | 上方 inline 区域高度累积偏差 |
| `nested/outer` | Y +34.5px | 同上 |
| `minmax/max-height` | overflow 状态不一致 | MBink 未检测到溢出（overflow detection Bug） |

## 已修复的问题

### 1. `window.innerWidth/Height` DPI 缩放问题（2026-02-25）

**文件**：`core/quickjs/window_bindings.cpp`

**问题**：`window.innerWidth` 错误地将物理像素除以 DPI 缩放，导致：
- MBink viewport 报告为 600×400（200% DPI 屏幕，实际物理像素 1200×800）
- Chrome headless 报告为 1200×800
- 产生 viewport 不一致警告

**修复**：直接返回物理像素宽高，与 `getBoundingClientRect()` 坐标系（也是物理像素）保持一致。

```cpp
// 修复前
return static_cast<int>(static_cast<float>(width) / dpi_scale);  // ❌ 除以了 DPI
// 修复后
return width;  // ✅ 直接返回物理像素（与布局坐标系一致）
```

## 待修复的 Bug

### Bug 1：inline 元素行高计算偏小（高优先级）

**文件**：`core/layout/ifc/`（行内格式化上下文）

**现象**：`display: inline` 元素的高度比 Chrome 少约 30px，导致后续元素 Y 坐标累积偏差。

**排查方向**：
1. `core/layout/ifc/` 中的行框（line box）高度计算
2. `font-size` 相关的 `line-height` 默认值处理
3. 字体上升/下降（ascent/descent）是否使用了正确的字体度量

### Bug 2：max-height overflow 检测缺失（中优先级）

**文件**：滚动/overflow 相关代码

**现象**：`minmax/max-height` 元素设置了 `max-height: 50px` 但内容超出时，MBink 的 `scrollHeight > clientHeight` 判断未触发。

**排查方向**：
1. 元素的 `scrollHeight` 是否正确计算
2. `max-height` 约束下内容溢出的滚动尺寸计算

## 技术架构

```
Python 脚本 (run_compare.py)
├── MBink 端
│   ├── 启动 esm_loader.exe（--borderless --no-gpu --width 1200 --height 800）
│   ├── 解析 stdout 中的 LAYOUT_DATA: JSON
│   └── 提取 relX/relY/centerOff/overflow 等字段
├── Chrome 端
│   ├── Playwright 启动本机 Chrome（headless，viewport 1200x800）
│   ├── 注入 _COLLECT_JS 采集脚本
│   └── 提取相同字段结构
└── 对比引擎
    ├── compare()：逐元素相对坐标对比
    ├── compare_sibling_gaps()：同父容器相邻子项间距对比
    ├── print_report()：彩色控制台输出
    └── save_html_report()：HTML 报告（含详细检查项）
```

