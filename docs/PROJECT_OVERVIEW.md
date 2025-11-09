# LightUI - 轻量级跨语言UI框架

## 项目概述

LightUI 是一个轻量级、高性能的跨语言桌面UI框架，旨在为Python、C++、Rust、Go等语言提供现代化的UI开发体验。

### 核心特性

- **轻量级**: 总体积 10-15MB（相比Electron的100MB+）
- **高性能**: 基于Skia的硬件加速渲染，60fps流畅体验
- **易用性**: 使用JavaScript/Preact编写UI，支持React生态组件库
- **跨语言**: 提供Python、C++、Rust、Go等多语言绑定
- **跨平台**: 支持Windows、macOS、Linux

### 技术栈

```
核心层 (C++):
├── QuickJS (~600KB)         - JavaScript引擎
├── Skia (5-8MB)             - 2D图形渲染引擎
├── SDL3 (1-2MB)             - 跨平台窗口系统
├── Yoga (~200KB)            - Flexbox布局引擎
├── DOM实现 (2-3MB)          - 轻量级DOM API
└── 桥接层 (~500KB)          - 多语言FFI接口

JavaScript层:
├── Preact (5KB)             - 轻量级React替代
├── Ant Design (可选)        - 企业级UI组件库
└── 用户应用代码

语言绑定:
├── Python (ctypes/pybind11)
├── C++ (原生)
├── Rust (bindgen)
├── Go (cgo)
└── Node.js (N-API)
```

### 架构图

```
┌─────────────────────────────────────────────────────────┐
│  应用层 (多语言)                                         │
│  Python | C++ | Rust | Go | Node.js                     │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  LightUI C API (统一FFI接口)                            │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  JavaScript UI层 (Preact + React生态)                   │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  QuickJS引擎 + DOM实现                                  │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  布局引擎 (Yoga) + 渲染引擎 (Skia)                      │
└────────────────────┬────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────┐
│  窗口系统 (SDL3)                                        │
└─────────────────────────────────────────────────────────┘
```

## 项目目标

### 短期目标 (6-8个月)

1. **核心框架开发** (3个月)
   - 完成QuickJS + Skia + SDL3集成
   - 实现基础DOM API
   - 支持Preact运行

2. **组件库支持** (2个月)
   - 支持Ant Design
   - 支持Material-UI
   - 完善样式系统

3. **多语言绑定** (2个月)
   - Python绑定完善
   - Rust/Go绑定
   - 文档和示例

4. **优化和发布** (1个月)
   - 性能优化
   - 完整文档
   - 开源发布

### 长期目标 (1-2年)

1. **生态建设**
   - 官方组件库
   - 插件系统
   - 开发者工具

2. **功能扩展**
   - WebGL支持
   - 动画系统
   - 国际化支持

3. **社区发展**
   - 活跃的社区
   - 丰富的第三方库
   - 企业级应用案例

## 对比分析

| 特性 | LightUI | Electron | Qt | Tauri | Dear ImGui |
|------|---------|----------|----|----|------------|
| **体积** | 10-15MB | 100MB+ | 30MB+ | 20MB+ | 2MB |
| **性能** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **易用性** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| **跨语言** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **生态** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐ |
| **学习曲线** | 低 | 低 | 高 | 中 | 中 |

## 使用场景

### 适合的场景

✅ 数据分析工具（Python + LightUI）
✅ 系统监控工具
✅ 数据库管理工具
✅ 开发者工具
✅ 企业内部工具
✅ 跨平台桌面应用
✅ 嵌入式设备UI

### 不适合的场景

❌ 高性能游戏
❌ 3D渲染应用
❌ 需要原生外观的应用
❌ 极致性能要求的应用

## 快速开始

### Python示例

```python
import lightui

# 初始化
lightui.init()

# 创建窗口
app = lightui.Window("My App", 800, 600)

# 绑定Python函数
@app.bind("getData")
def get_data():
    return {"message": "Hello from Python!"}

# 加载UI
app.load_ui("""
import { render } from 'preact';
import { Button } from 'antd';

function App() {
    const handleClick = () => {
        const data = window.getData();
        alert(data.message);
    };
    
    return <Button onClick={handleClick}>Click Me</Button>;
}

render(<App />, document.body);
""")

# 运行
app.run()
```

### C++示例

```cpp
#include "lightui.h"

int main() {
    lightui_init();
    
    auto window = lightui_create_window("My App", 800, 600);
    
    lightui_bind_function(window, "getData", [](const char* args, char** result) {
        *result = strdup("{\"message\": \"Hello from C++!\"}");
    });
    
    lightui_load_ui_file(window, "ui/app.jsx");
    lightui_run(window);
    
    lightui_cleanup();
    return 0;
}
```

## 开发团队

- **核心开发**: [待定]
- **贡献者**: 欢迎社区贡献

## 许可证

MIT License

## 联系方式

- **GitHub**: [待定]
- **文档**: [待定]
- **社区**: [待定]

## 路线图

详见 [ROADMAP.md](ROADMAP.md)

## 贡献指南

详见 [CONTRIBUTING.md](CONTRIBUTING.md)

## 更新日志

详见 [CHANGELOG.md](CHANGELOG.md)

