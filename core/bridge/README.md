# Bridge 模块

## 📋 概述

Bridge 模块是 C++ 核心实现和 C API 接口之间的桥接层。它负责在 C++ 对象模型和 C 风格接口之间进行转换，处理类型转换、异常处理、资源管理等关键任务。

## 🎯 主要功能

- **类型转换**: C++ 对象与 C 结构体之间的双向转换
- **异常处理**: 将 C++ 异常转换为 C 风格的错误码
- **资源生命周期管理**: 管理跨 C/C++ 边界的对象生命周期
- **智能指针包装**: 将 `std::shared_ptr` 等智能指针转换为 C 指针
- **回调函数桥接**: 处理 C 回调函数到 C++ lambda 的转换

## 📁 文件结构

```
bridge/
├── CMakeLists.txt    # 构建配置
├── bridge.h          # 桥接层头文件
└── bridge.cpp        # 桥接层实现
```

## 🔌 核心类和函数

### Bridge 类

```cpp
class Bridge {
public:
    // C++ 对象到 C 句柄的转换
    template<typename T>
    static void* ToCHandle(std::shared_ptr<T> obj);
    
    // C 句柄到 C++ 对象的转换
    template<typename T>
    static std::shared_ptr<T> FromCHandle(void* handle);
    
    // 异常捕获和错误码转换
    static int CatchException(std::function<void()> func);
    
    // 资源释放
    template<typename T>
    static void ReleaseHandle(void* handle);
};
```

### 类型转换辅助函数

```cpp
// 字符串转换
std::string CStringToStdString(const char* str);
const char* StdStringToCString(const std::string& str);

// 布尔值转换
bool CIntToBool(int value);
int BoolToCInt(bool value);
```

## 💡 使用示例

### C++ 对象到 C 句柄

```cpp
// C++ 侧
auto window = std::make_shared<Window>(800, 600, "Test");
void* handle = Bridge::ToCHandle(window);

// 传递给 C API
return handle;
```

### C 句柄到 C++ 对象

```cpp
// C API 实现中
void lightui_destroy_window(LightUIWindow* window) {
    try {
        auto win = Bridge::FromCHandle<Window>(window);
        // 使用 C++ 对象
        win->Close();
    } catch (...) {
        // 错误处理
    }
}
```

### 异常处理

```cpp
int lightui_load_html(LightUIWindow* window, const char* html) {
    return Bridge::CatchException([&]() {
        auto win = Bridge::FromCHandle<Window>(window);
        win->LoadHTML(html);
    });
}
```

## 🏗️ 架构说明

Bridge 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  C API Layer (core/api)                 │
│  lightui.h, lightui.cpp                 │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Bridge Layer (core/bridge) ← 当前模块   │
│  类型转换、异常处理、资源管理              │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  C++ Core Modules                       │
│  Window, DOM, QuickJS, etc.             │
└─────────────────────────────────────────┘
```

## 🔗 依赖关系

### 依赖的模块

- `core/utils` - 工具函数
- `core/window` - 窗口对象
- `core/dom` - DOM 对象
- `core/quickjs` - JavaScript 运行时

### 被依赖的模块

- `core/api` - C API 接口层

## 🔧 设计模式

### 1. RAII 包装

```cpp
class HandleGuard {
public:
    HandleGuard(void* handle) : handle_(handle) {}
    ~HandleGuard() {
        if (handle_) {
            Bridge::ReleaseHandle<Window>(handle_);
        }
    }
private:
    void* handle_;
};
```

### 2. 异常安全

```cpp
int Bridge::CatchException(std::function<void()> func) {
    try {
        func();
        return 0;  // 成功
    } catch (const std::exception& e) {
        Logger::Error("Exception: {}", e.what());
        return -1;  // 失败
    } catch (...) {
        Logger::Error("Unknown exception");
        return -2;  // 未知错误
    }
}
```

### 3. 智能指针管理

```cpp
template<typename T>
void* Bridge::ToCHandle(std::shared_ptr<T> obj) {
    // 增加引用计数
    auto* ptr = new std::shared_ptr<T>(obj);
    return static_cast<void*>(ptr);
}

template<typename T>
std::shared_ptr<T> Bridge::FromCHandle(void* handle) {
    auto* ptr = static_cast<std::shared_ptr<T>*>(handle);
    return *ptr;
}
```

## ⚠️ 注意事项

### 内存管理

1. **引用计数**: 使用智能指针管理对象生命周期
2. **句柄释放**: C 侧必须调用释放函数
3. **悬空指针**: 避免在对象销毁后使用句柄

### 线程安全

1. **非线程安全**: 当前实现不是线程安全的
2. **主线程调用**: 所有 API 调用应在主线程
3. **同步机制**: 未来版本将添加线程同步

### 异常处理

1. **捕获所有异常**: 不允许异常跨越 C/C++ 边界
2. **错误码映射**: 统一的错误码定义
3. **日志记录**: 记录所有异常信息

## 📚 相关文档

- [API 设计文档](../../docs/API_DESIGN.md)
- [架构设计文档](../../docs/ARCHITECTURE.md)
- [代码规范](../../docs/CODING_STANDARDS.md)

## 🔍 调试技巧

### 句柄追踪

```cpp
// 启用句柄追踪（调试模式）
#ifdef DEBUG
    static std::unordered_map<void*, std::string> handle_map_;
    
    void* Bridge::ToCHandle(std::shared_ptr<T> obj) {
        void* handle = /* ... */;
        handle_map_[handle] = typeid(T).name();
        return handle;
    }
#endif
```

### 异常日志

```cpp
int Bridge::CatchException(std::function<void()> func) {
    try {
        func();
        return 0;
    } catch (const std::exception& e) {
        Logger::Error("Exception at {}: {}", 
            __FILE__, __LINE__, e.what());
        return -1;
    }
}
```

## 📊 性能考虑

- **零拷贝**: 尽可能使用引用和指针，避免数据拷贝
- **内联函数**: 简单的转换函数使用 `inline`
- **缓存**: 缓存频繁使用的转换结果

## 🚀 未来改进

1. **线程安全**: 添加互斥锁保护
2. **错误码扩展**: 更详细的错误分类
3. **性能优化**: 减少类型转换开销
4. **调试工具**: 句柄泄漏检测

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

