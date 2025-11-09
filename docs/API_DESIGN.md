# LightUI C API 设计文档

## 1. 设计原则

### 1.1 核心原则

- **简单性**: API简单易用，学习曲线平缓
- **一致性**: 命名和行为保持一致
- **安全性**: 错误处理完善，避免崩溃
- **跨语言**: 纯C接口，方便各语言绑定
- **向后兼容**: 保持API稳定性

### 1.2 命名约定

```c
// 所有API以lightui_前缀
lightui_init()
lightui_create_window()

// 类型以LightUI前缀，PascalCase
typedef struct LightUIWindow* LightUIWindowHandle;

// 常量以LIGHTUI_前缀，全大写
#define LIGHTUI_VERSION_MAJOR 1
#define LIGHTUI_VERSION_MINOR 0
```

---

## 2. 核心API

### 2.1 初始化和清理

```c
/**
 * 初始化LightUI库
 * 
 * @return 0表示成功，非0表示失败
 * 
 * @note 必须在使用其他API之前调用
 * @note 线程安全：否
 */
int lightui_init(void);

/**
 * 清理LightUI库
 * 
 * @note 释放所有资源
 * @note 调用后不能再使用其他API
 */
void lightui_cleanup(void);

/**
 * 获取版本信息
 * 
 * @param major 主版本号输出
 * @param minor 次版本号输出
 * @param patch 补丁版本号输出
 */
void lightui_get_version(int* major, int* minor, int* patch);
```

---

### 2.2 窗口管理

```c
/**
 * 窗口句柄（不透明指针）
 */
typedef struct LightUIWindow* LightUIWindowHandle;

/**
 * 窗口配置
 */
typedef struct {
    const char* title;        // 窗口标题
    int width;                // 窗口宽度
    int height;               // 窗口高度
    int x;                    // 窗口X位置（-1表示居中）
    int y;                    // 窗口Y位置（-1表示居中）
    int resizable;            // 是否可调整大小（0或1）
    int fullscreen;           // 是否全屏（0或1）
    int borderless;           // 是否无边框（0或1）
} LightUIWindowConfig;

/**
 * 创建窗口（简化版）
 * 
 * @param title 窗口标题
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 窗口句柄，失败返回NULL
 */
LightUIWindowHandle lightui_create_window(
    const char* title,
    int width,
    int height
);

/**
 * 创建窗口（完整版）
 * 
 * @param config 窗口配置
 * @return 窗口句柄，失败返回NULL
 */
LightUIWindowHandle lightui_create_window_ex(
    const LightUIWindowConfig* config
);

/**
 * 销毁窗口
 * 
 * @param window 窗口句柄
 */
void lightui_destroy_window(LightUIWindowHandle window);

/**
 * 显示窗口
 * 
 * @param window 窗口句柄
 */
void lightui_show_window(LightUIWindowHandle window);

/**
 * 隐藏窗口
 * 
 * @param window 窗口句柄
 */
void lightui_hide_window(LightUIWindowHandle window);

/**
 * 设置窗口标题
 * 
 * @param window 窗口句柄
 * @param title 新标题
 */
void lightui_set_window_title(LightUIWindowHandle window, const char* title);

/**
 * 设置窗口大小
 * 
 * @param window 窗口句柄
 * @param width 新宽度
 * @param height 新高度
 */
void lightui_set_window_size(LightUIWindowHandle window, int width, int height);

/**
 * 获取窗口大小
 * 
 * @param window 窗口句柄
 * @param width 宽度输出
 * @param height 高度输出
 */
void lightui_get_window_size(LightUIWindowHandle window, int* width, int* height);
```

---

### 2.3 UI加载

```c
/**
 * 加载UI（从JavaScript代码字符串）
 * 
 * @param window 窗口句柄
 * @param js_code JavaScript代码
 * @return 0表示成功，非0表示失败
 * 
 * @example
 *   const char* code = "import { render } from 'preact'; ...";
 *   lightui_load_ui(window, code);
 */
int lightui_load_ui(
    LightUIWindowHandle window,
    const char* js_code
);

/**
 * 加载UI（从文件）
 * 
 * @param window 窗口句柄
 * @param js_file_path JavaScript文件路径
 * @return 0表示成功，非0表示失败
 */
int lightui_load_ui_file(
    LightUIWindowHandle window,
    const char* js_file_path
);

/**
 * 重新加载UI
 * 
 * @param window 窗口句柄
 * @return 0表示成功，非0表示失败
 * 
 * @note 用于热重载
 */
int lightui_reload_ui(LightUIWindowHandle window);
```

---

### 2.4 函数绑定

```c
/**
 * 回调函数类型
 * 
 * @param args JSON格式的参数字符串
 * @param result JSON格式的结果字符串输出（需要调用者释放）
 * @param user_data 用户数据
 */
typedef void (*LightUICallback)(
    const char* args,
    char** result,
    void* user_data
);

/**
 * 绑定函数（让JavaScript可以调用）
 * 
 * @param window 窗口句柄
 * @param function_name 函数名称
 * @param callback 回调函数
 * @param user_data 用户数据（可选，传递给回调）
 * @return 0表示成功，非0表示失败
 * 
 * @example
 *   void my_callback(const char* args, char** result, void* user_data) {
 *       *result = strdup("{\"success\": true}");
 *   }
 *   lightui_bind_function(window, "myFunction", my_callback, NULL);
 */
int lightui_bind_function(
    LightUIWindowHandle window,
    const char* function_name,
    LightUICallback callback,
    void* user_data
);

/**
 * 解绑函数
 * 
 * @param window 窗口句柄
 * @param function_name 函数名称
 */
void lightui_unbind_function(
    LightUIWindowHandle window,
    const char* function_name
);
```

---

### 2.5 JavaScript调用

```c
/**
 * 调用JavaScript函数
 * 
 * @param window 窗口句柄
 * @param function_name 函数名称
 * @param args_json JSON格式的参数
 * @param result_json JSON格式的结果输出（需要调用者释放）
 * @return 0表示成功，非0表示失败
 * 
 * @example
 *   char* result = NULL;
 *   lightui_call_js_function(window, "updateData", "[1, 2, 3]", &result);
 *   printf("Result: %s\n", result);
 *   lightui_free_string(result);
 */
int lightui_call_js_function(
    LightUIWindowHandle window,
    const char* function_name,
    const char* args_json,
    char** result_json
);

/**
 * 执行JavaScript代码
 * 
 * @param window 窗口句柄
 * @param js_code JavaScript代码
 * @param result 结果输出（需要调用者释放）
 * @return 0表示成功，非0表示失败
 */
int lightui_eval_js(
    LightUIWindowHandle window,
    const char* js_code,
    char** result
);
```

---

### 2.6 事件循环

```c
/**
 * 运行事件循环（阻塞）
 * 
 * @param window 窗口句柄
 * 
 * @note 此函数会阻塞直到窗口关闭
 */
void lightui_run(LightUIWindowHandle window);

/**
 * 停止事件循环
 * 
 * @param window 窗口句柄
 * 
 * @note 可以从回调函数中调用
 */
void lightui_stop(LightUIWindowHandle window);

/**
 * 处理一次事件（非阻塞）
 * 
 * @param window 窗口句柄
 * @return 1表示有事件处理，0表示无事件
 * 
 * @note 用于自定义事件循环
 */
int lightui_poll_events(LightUIWindowHandle window);
```

---

### 2.7 工具函数

```c
/**
 * 释放字符串
 * 
 * @param str 由LightUI分配的字符串
 * 
 * @note 必须用此函数释放LightUI返回的字符串
 */
void lightui_free_string(char* str);

/**
 * 获取最后的错误信息
 * 
 * @return 错误信息字符串（不需要释放）
 */
const char* lightui_get_last_error(void);

/**
 * 设置日志级别
 * 
 * @param level 日志级别（0=关闭, 1=错误, 2=警告, 3=信息, 4=调试）
 */
void lightui_set_log_level(int level);

/**
 * 设置日志回调
 * 
 * @param callback 日志回调函数
 */
typedef void (*LightUILogCallback)(int level, const char* message);
void lightui_set_log_callback(LightUILogCallback callback);
```

---

## 3. 使用示例

### 3.1 C语言示例

```c
#include "lightui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 回调函数
void get_data_callback(const char* args, char** result, void* user_data) {
    // 返回JSON数据
    *result = strdup("{\"users\": [{\"name\": \"Alice\"}, {\"name\": \"Bob\"}]}");
}

int main() {
    // 1. 初始化
    if (lightui_init() != 0) {
        fprintf(stderr, "Failed to initialize LightUI\n");
        return 1;
    }
    
    // 2. 创建窗口
    LightUIWindowHandle window = lightui_create_window("My App", 800, 600);
    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", lightui_get_last_error());
        lightui_cleanup();
        return 1;
    }
    
    // 3. 绑定函数
    lightui_bind_function(window, "getData", get_data_callback, NULL);
    
    // 4. 加载UI
    const char* ui_code = 
        "import { render } from 'preact';"
        "import { useState, useEffect } from 'preact/hooks';"
        "function App() {"
        "  const [users, setUsers] = useState([]);"
        "  useEffect(() => {"
        "    const data = window.getData();"
        "    setUsers(data.users);"
        "  }, []);"
        "  return <div>{users.map(u => <p>{u.name}</p>)}</div>;"
        "}"
        "render(<App />, document.body);";
    
    if (lightui_load_ui(window, ui_code) != 0) {
        fprintf(stderr, "Failed to load UI: %s\n", lightui_get_last_error());
        lightui_destroy_window(window);
        lightui_cleanup();
        return 1;
    }
    
    // 5. 运行
    lightui_run(window);
    
    // 6. 清理
    lightui_destroy_window(window);
    lightui_cleanup();
    
    return 0;
}
```

### 3.2 C++示例

```cpp
#include "lightui.h"
#include <iostream>
#include <string>
#include <functional>

class App {
public:
    App() {
        lightui_init();
        window_ = lightui_create_window("My App", 800, 600);
        
        // 使用lambda
        lightui_bind_function(window_, "getData", 
            [](const char* args, char** result, void* user_data) {
                auto* app = static_cast<App*>(user_data);
                *result = strdup(app->GetData().c_str());
            }, this);
    }
    
    ~App() {
        lightui_destroy_window(window_);
        lightui_cleanup();
    }
    
    void LoadUI(const std::string& code) {
        lightui_load_ui(window_, code.c_str());
    }
    
    void Run() {
        lightui_run(window_);
    }
    
    std::string GetData() {
        return R"({"users": [{"name": "Alice"}]})";
    }
    
private:
    LightUIWindowHandle window_;
};

int main() {
    App app;
    app.LoadUI("...");
    app.Run();
    return 0;
}
```

---

## 4. 错误处理

### 4.1 错误码

```c
#define LIGHTUI_OK                  0
#define LIGHTUI_ERROR_INIT_FAILED   -1
#define LIGHTUI_ERROR_INVALID_PARAM -2
#define LIGHTUI_ERROR_OUT_OF_MEMORY -3
#define LIGHTUI_ERROR_JS_ERROR      -4
#define LIGHTUI_ERROR_FILE_NOT_FOUND -5
```

### 4.2 错误处理示例

```c
int result = lightui_load_ui(window, code);
if (result != LIGHTUI_OK) {
    const char* error = lightui_get_last_error();
    fprintf(stderr, "Error: %s\n", error);
    
    switch (result) {
        case LIGHTUI_ERROR_JS_ERROR:
            // JavaScript错误
            break;
        case LIGHTUI_ERROR_FILE_NOT_FOUND:
            // 文件未找到
            break;
        default:
            // 其他错误
            break;
    }
}
```

---

## 5. 线程安全

### 5.1 线程安全性

- **lightui_init/cleanup**: 不是线程安全的
- **窗口操作**: 必须在主线程调用
- **回调函数**: 在主线程中调用

### 5.2 多线程示例

```c
// ❌ 错误：在工作线程中操作窗口
void* worker_thread(void* arg) {
    LightUIWindowHandle window = (LightUIWindowHandle)arg;
    lightui_set_window_title(window, "New Title");  // 不安全！
    return NULL;
}

// ✅ 正确：通过消息队列通知主线程
void* worker_thread(void* arg) {
    // 计算结果
    char* result = compute_data();
    
    // 发送到主线程
    post_to_main_thread(result);
    return NULL;
}
```

---

## 6. 内存管理

### 6.1 内存所有权

```c
// LightUI分配的内存，必须用lightui_free_string释放
char* result = NULL;
lightui_call_js_function(window, "func", "[]", &result);
// 使用result...
lightui_free_string(result);  // 必须释放

// 传递给LightUI的字符串，LightUI会复制
const char* title = "My App";
lightui_set_window_title(window, title);  // LightUI内部会复制
// title可以立即释放或重用
```

### 6.2 回调函数中的内存管理

```c
void callback(const char* args, char** result, void* user_data) {
    // args由LightUI管理，不要释放
    
    // result需要分配新内存
    *result = strdup("{\"success\": true}");  // 或malloc
    // LightUI会自动释放
}
```

---

## 7. 版本兼容性

### 7.1 API版本

```c
#define LIGHTUI_VERSION_MAJOR 1
#define LIGHTUI_VERSION_MINOR 0
#define LIGHTUI_VERSION_PATCH 0

// 检查版本
int major, minor, patch;
lightui_get_version(&major, &minor, &patch);
if (major != LIGHTUI_VERSION_MAJOR) {
    fprintf(stderr, "Version mismatch!\n");
}
```

### 7.2 向后兼容承诺

- 主版本号（Major）：可能有破坏性变更
- 次版本号（Minor）：新增功能，向后兼容
- 补丁版本号（Patch）：Bug修复，完全兼容

---

## 8. 性能考虑

### 8.1 最佳实践

```c
// ✅ 好：批量操作
char* result = NULL;
lightui_call_js_function(window, "batchUpdate", "[...]", &result);

// ❌ 差：频繁调用
for (int i = 0; i < 1000; i++) {
    char* result = NULL;
    lightui_call_js_function(window, "update", "[...]", &result);
    lightui_free_string(result);
}
```

### 8.2 性能提示

- 减少C和JavaScript之间的调用次数
- 使用批量操作
- 避免在回调中执行耗时操作
- 使用异步操作处理长时间任务

---

## 9. 调试支持

### 9.1 日志

```c
// 设置日志级别
lightui_set_log_level(4);  // 调试级别

// 自定义日志处理
void my_log_handler(int level, const char* message) {
    printf("[%d] %s\n", level, message);
}
lightui_set_log_callback(my_log_handler);
```

### 9.2 调试工具

```c
// 启用开发者工具（未来功能）
lightui_enable_devtools(window, 1);
```

---

## 10. 平台特定

### 10.1 Windows

```c
// 获取HWND（Windows句柄）
HWND hwnd = lightui_get_native_handle(window);
```

### 10.2 macOS

```c
// 获取NSWindow
void* nswindow = lightui_get_native_handle(window);
```

### 10.3 Linux

```c
// 获取X11 Window
unsigned long xwindow = (unsigned long)lightui_get_native_handle(window);
```

