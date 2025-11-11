# LightUI 技术选型完整分析

## 📋 目录

1. [当前技术栈](#当前技术栈)
2. [核心依赖分析](#核心依赖分析)
3. [可选依赖建议](#可选依赖建议)
4. [技术选型对比](#技术选型对比)
5. [集成建议](#集成建议)
6. [风险评估](#风险评估)

---

## 🎯 项目目标回顾

**核心目标**: 使用 QuickJS + Skia 实现一个能加载 React 的极致桌面 UI 开发框架

**关键要求**:
- ✅ 轻量级（10-15MB vs Electron 100MB+）
- ✅ 高性能（60fps 流畅渲染）
- ✅ 完整的 DOM API 和事件系统
- ✅ 支持 React/Preact 生态
- ✅ 跨平台（Windows, macOS, Linux）
- ✅ 多语言绑定（Python, C++, Rust, Go）

---

## 📦 当前技术栈

### ✅ 已集成的核心依赖

| 库名 | 版本 | 大小 | 用途 | 状态 | 评价 |
|------|------|------|------|------|------|
| **QuickJS-ng** | latest | ~600KB | JavaScript 引擎 | ✅ 已集成 | ⭐⭐⭐⭐⭐ 最佳选择 |
| **Skia** | m122 | 5-8MB | 2D 图形渲染 | ✅ 已集成 | ⭐⭐⭐⭐⭐ 行业标准 |
| **SDL3** | 3.x | 1-2MB | 窗口/事件系统 | ✅ 已集成 | ⭐⭐⭐⭐⭐ 跨平台首选 |
| **Yoga** | 2.0 | ~200KB | Flexbox 布局 | ✅ 已集成 | ⭐⭐⭐⭐⭐ Facebook 出品 |
| **nlohmann/json** | 3.11 | 单头文件 | JSON 解析 | ✅ 已集成 | ⭐⭐⭐⭐⭐ C++ 最佳 JSON 库 |
| **GoogleTest** | 1.14 | ~2MB | 单元测试 | ✅ 已集成 | ⭐⭐⭐⭐⭐ 行业标准 |

**总大小**: ~10-15MB（符合目标）

### 📊 当前技术栈评估

#### ✅ 优势
1. **轻量级**: 总体积远小于 Electron（100MB+）
2. **高性能**: Skia 硬件加速 + QuickJS 高效执行
3. **成熟稳定**: 所有库都是行业验证的成熟方案
4. **零依赖冲突**: 各库之间无依赖冲突
5. **跨平台**: 所有库都支持 Windows/macOS/Linux

#### ⚠️ 不足
1. **缺少 HTML/CSS 解析器** - 需要自己实现或集成第三方库
2. **缺少图片加载库** - 需要支持 PNG/JPG/GIF 等格式
3. **缺少 HTTP 客户端** - 未来 fetch API 需要
4. **缺少文本编辑组件** - input/textarea 需要复杂的文本处理

---

## 🔍 核心依赖分析

### 1. JavaScript 引擎：QuickJS-ng ✅

**当前选择**: QuickJS-ng (QuickJS 的现代化分支)

**对比分析**:

| 引擎 | 大小 | 性能 | ES6+ 支持 | 嵌入难度 | 推荐度 |
|------|------|------|-----------|----------|--------|
| **QuickJS-ng** | 600KB | ⭐⭐⭐⭐ | ✅ 完整 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| V8 | 50MB+ | ⭐⭐⭐⭐⭐ | ✅ 完整 | ⭐⭐ | ⭐⭐ |
| JavaScriptCore | 10MB+ | ⭐⭐⭐⭐⭐ | ✅ 完整 | ⭐⭐⭐ | ⭐⭐⭐ |
| Duktape | 200KB | ⭐⭐ | ❌ ES5 | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| Hermes | 2MB | ⭐⭐⭐⭐ | ✅ 部分 | ⭐⭐⭐ | ⭐⭐⭐ |

**结论**: ✅ **保持 QuickJS-ng**
- 体积最小，性能足够
- 完整的 ES6+ 支持（支持 React）
- 易于嵌入和调试
- 活跃维护（相比原版 QuickJS）

---

### 2. 渲染引擎：Skia ✅

**当前选择**: Skia (Google Chrome/Android 使用)

**对比分析**:

| 引擎 | 大小 | 性能 | 功能 | 跨平台 | 推荐度 |
|------|------|------|------|--------|--------|
| **Skia** | 5-8MB | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐⭐ |
| Cairo | 2MB | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐ |
| NanoVG | 100KB | ⭐⭐⭐ | ⭐⭐⭐ | ✅ | ⭐⭐⭐ |
| Direct2D | 系统自带 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ❌ Windows | ⭐⭐ |

**结论**: ✅ **保持 Skia**
- Chrome/Flutter 使用，久经考验
- 硬件加速 + CPU 软件渲染双模式
- 完整的文本渲染（HarfBuzz + ICU）
- 内置图片解码（PNG/JPG/WebP）
- 最佳的跨平台支持

---

### 3. 窗口系统：SDL3 ✅

**当前选择**: SDL3 (最新版本)

**对比分析**:

| 库 | 大小 | 功能 | 跨平台 | 社区 | 推荐度 |
|------|------|------|--------|------|--------|
| **SDL3** | 1-2MB | ⭐⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| GLFW | 500KB | ⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| SFML | 3MB | ⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Qt | 30MB+ | ⭐⭐⭐⭐⭐ | ✅ | ⭐⭐⭐⭐⭐ | ⭐⭐ |

**结论**: ✅ **保持 SDL3**
- 游戏行业标准（Unity, Unreal 等使用）
- 完整的事件系统（鼠标、键盘、触摸、手柄）
- 内置文本输入和 IME 支持
- OpenGL/Vulkan/Metal 支持
- SDL3 是最新版本，API 更现代

---

### 4. 布局引擎：Yoga ✅

**当前选择**: Yoga (Facebook 出品)

**对比分析**:

| 引擎 | 大小 | 性能 | 标准兼容 | 推荐度 |
|------|------|------|----------|--------|
| **Yoga** | 200KB | ⭐⭐⭐⭐⭐ | ✅ Flexbox | ⭐⭐⭐⭐⭐ |
| Stretch | 300KB | ⭐⭐⭐⭐ | ✅ Flexbox | ⭐⭐⭐⭐ |
| 自己实现 | - | ⭐⭐ | ❌ | ⭐ |

**结论**: ✅ **保持 Yoga**
- React Native 使用，与 React 生态完美契合
- 完整的 Flexbox 实现
- 高性能 C++ 实现
- 跨平台支持

---

## 🆕 可选依赖建议

### 🔴 P0: 强烈推荐立即集成

#### 1. **stb_image** - 图片加载 ⭐⭐⭐⭐⭐

**用途**: 加载 PNG/JPG/BMP/GIF 等图片格式

**优势**:
- ✅ 单头文件库（零依赖）
- ✅ 体积极小（~10KB）
- ✅ 被广泛使用（Unity, Unreal, Godot）
- ✅ 支持所有常见格式
- ✅ 5 分钟即可集成

**集成方式**:
```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int width, height, channels;
unsigned char* data = stbi_load("image.png", &width, &height, &channels, 4);
// 使用 Skia 渲染
SkBitmap bitmap;
bitmap.installPixels(SkImageInfo::MakeN32Premul(width, height), data, width * 4);
stbi_image_free(data);
```

**替代方案**:
- ❌ Skia 内置解码器 - 需要额外配置，不够灵活
- ❌ libpng/libjpeg - 需要多个库，体积大

**建议**: ✅ **立即集成 stb_image**

---

#### 2. **Lexbor** - HTML/CSS 解析器 ⭐⭐⭐⭐⭐

**用途**: 解析 HTML/CSS，实现 querySelector 等 API

**优势**:
- ✅ 纯 C 语言，超高性能
- ✅ 完整的 HTML5 + CSS3 支持
- ✅ 零依赖
- ✅ 被 Servo、Chromium 等项目参考
- ✅ 完整的 CSS 选择器引擎

**功能**:
- HTML 解析（比我们自己实现的更完整）
- CSS 选择器（querySelector, querySelectorAll）
- CSS 解析（`<style>` 标签支持）
- DOM 树构建

**集成方式**:
```bash
git submodule add https://github.com/lexbor/lexbor third_party/lexbor
```

**替代方案**:
- ❌ 自己实现 - 需要 2-3 周，容易有 bug
- ⚠️ gumbo-parser - 只支持 HTML，不支持 CSS
- ⚠️ pugixml - 只支持 XML，不支持 HTML5

**建议**: ✅ **立即集成 Lexbor**（节省 2-3 周开发时间）

---

### 🟠 P1: 推荐近期集成

#### 3. **cpp-httplib** - HTTP 客户端 ⭐⭐⭐⭐⭐

**用途**: 实现 fetch API，支持网络请求

**优势**:
- ✅ 单头文件库
- ✅ 支持 HTTP/HTTPS
- ✅ 简单易用
- ✅ 支持异步请求

**使用场景**:
- fetch API 实现
- 加载远程资源
- API 调用

**集成方式**:
```cpp
#include "httplib.h"

httplib::Client cli("http://api.example.com");
auto res = cli.Get("/data");
if (res && res->status == 200) {
    // 处理响应
}
```

**建议**: ⚠️ **Phase 3 再集成**（当前不需要）

---

#### 4. **ICU** - 国际化支持 ⭐⭐⭐⭐

**用途**: Unicode 处理、文本分割、国际化

**优势**:
- ✅ 行业标准（Chrome, Firefox 使用）
- ✅ 完整的 Unicode 支持
- ✅ 文本分割（断行、断词）

**问题**:
- ❌ 体积大（10MB+）
- ⚠️ Skia 已经包含 ICU

**建议**: ✅ **使用 Skia 内置的 ICU**（无需额外集成）

---

### 🟡 P2: 可选集成

#### 5. **libuv** - 异步 I/O ⭐⭐⭐⭐

**用途**: 异步文件 I/O、网络 I/O

**优势**:
- ✅ Node.js 使用
- ✅ 跨平台事件循环
- ✅ 高性能

**问题**:
- ⚠️ 我们已经有 EventLoop
- ⚠️ 增加复杂度

**建议**: ❌ **暂不集成**（我们的 EventLoop 足够）

---

#### 6. **WebView** - 嵌入浏览器 ⭐⭐⭐

**用途**: 嵌入系统浏览器（用于 OAuth 等）

**优势**:
- ✅ 轻量级（使用系统浏览器）
- ✅ 完整的 Web 支持

**问题**:
- ❌ 与项目目标冲突（我们要替代浏览器）

**建议**: ❌ **不集成**

---

## 📊 技术选型对比表

### JavaScript 引擎对比

| 特性 | QuickJS-ng | V8 | JavaScriptCore | Duktape |
|------|-----------|----|----|---------|
| **体积** | 600KB | 50MB+ | 10MB+ | 200KB |
| **ES6+ 支持** | ✅ 完整 | ✅ 完整 | ✅ 完整 | ❌ ES5 |
| **性能** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| **嵌入难度** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **内存占用** | 低 | 高 | 中 | 极低 |
| **React 支持** | ✅ | ✅ | ✅ | ❌ |
| **推荐度** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐ |

**结论**: QuickJS-ng 是最佳选择

---

### HTML/CSS 解析器对比

| 库 | 语言 | 大小 | HTML5 | CSS3 | 选择器 | 推荐度 |
|------|------|------|-------|------|--------|--------|
| **Lexbor** | C | 500KB | ✅ | ✅ | ✅ | ⭐⭐⭐⭐⭐ |
| gumbo-parser | C | 200KB | ✅ | ❌ | ❌ | ⭐⭐⭐ |
| pugixml | C++ | 100KB | ❌ | ❌ | ❌ | ⭐⭐ |
| 自己实现 | C++ | - | ⚠️ | ⚠️ | ⚠️ | ⭐ |

**结论**: Lexbor 是最佳选择

---

## 🎯 最终推荐技术栈

### ✅ 核心依赖（已集成，保持不变）

1. **QuickJS-ng** - JavaScript 引擎
2. **Skia** - 2D 渲染引擎
3. **SDL3** - 窗口/事件系统
4. **Yoga** - Flexbox 布局
5. **nlohmann/json** - JSON 解析
6. **GoogleTest** - 单元测试

### 🆕 新增依赖（强烈推荐）

7. **stb_image** - 图片加载（P0，立即集成）
8. **Lexbor** - HTML/CSS 解析（P0，立即集成）

### ⏳ 未来可选依赖

9. **cpp-httplib** - HTTP 客户端（Phase 3）
10. **easing-functions** - 动画缓动（Phase 2.5 Task 10）

---

## 📋 集成建议

### Phase 2.5 Week 1: 集成 stb_image 和 Lexbor

#### Step 1: 集成 stb_image（30 分钟）

```bash
# 下载单头文件
mkdir -p third_party/stb
curl -o third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

**修改 CMakeLists.txt**:
```cmake
# third_party/CMakeLists.txt
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${THIRD_PARTY_DIR}/stb")
```

#### Step 2: 集成 Lexbor（1 小时）

```bash
# 添加为 submodule
cd third_party
git submodule add https://github.com/lexbor/lexbor.git
cd lexbor
cmake -B build
cmake --build build
```

**修改 CMakeLists.txt**:
```cmake
# third_party/CMakeLists.txt
if(EXISTS "${THIRD_PARTY_DIR}/lexbor/CMakeLists.txt")
    message(STATUS "  Found Lexbor")
    add_subdirectory(lexbor)
    set(LEXBOR_FOUND TRUE PARENT_SCOPE)
endif()
```

---

## ⚠️ 风险评估

### 低风险
- ✅ stb_image - 单头文件，零依赖，广泛使用
- ✅ 保持现有核心依赖 - 已验证稳定

### 中风险
- ⚠️ Lexbor - 需要学习 API，但文档完善
- ⚠️ cpp-httplib - HTTPS 需要 OpenSSL

### 高风险
- ❌ 替换 QuickJS - 会破坏现有代码
- ❌ 替换 Skia - 工作量巨大

---

## 📝 总结

### ✅ 当前技术栈评价：优秀

我们的核心技术选型非常合理：
- QuickJS-ng: 轻量级 + 完整 ES6+ 支持
- Skia: 行业标准渲染引擎
- SDL3: 最新的跨平台窗口系统
- Yoga: React Native 同款布局引擎

### 🆕 建议新增

1. **stb_image** - 必须（图片加载）
2. **Lexbor** - 强烈推荐（节省 2-3 周开发时间）

### ❌ 不建议集成

- libuv（我们有 EventLoop）
- WebView（与目标冲突）
- 其他 JS 引擎（QuickJS-ng 已是最佳）

---

**文档创建时间**: 2025-11-11  
**下次审查时间**: Phase 3 开始前

