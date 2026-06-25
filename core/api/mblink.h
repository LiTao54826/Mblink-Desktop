/**
 * @file mblink.h
 * @brief MBlink C API v2 - 跨语言绑定统一接口
 *
 * 这是 MBlink 框架面向所有语言（Python/Go/Rust/Node.js）的唯一入口。
 * 所有绑定都通过此 C ABI 调用 mblink.dll / libmblink.so。
 *
 * 特性：
 * - 一个 create() 调用完成全部初始化（Window+Document+Runtime+EventLoop+HostBridge）
 * - 所有状态操作线程安全
 * - 直接类型接口避免 JSON 序列化开销
 * - 操作队列合并优化
 * - 事件回调注册
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ========== 导出宏 ==========
#ifdef _WIN32
  #ifdef MBLINK_BUILDING_DLL
    #define MBLINK_API __declspec(dllexport)
  #else
    #define MBLINK_API __declspec(dllimport)
  #endif
#else
  #define MBLINK_API __attribute__((visibility("default")))
#endif

// ========== 类型定义 ==========

typedef struct MBlinkWindow* MBlinkHandle;

typedef enum {
    MBLINK_OK = 0,
    MBLINK_ERROR_INVALID_HANDLE = -1,
    MBLINK_ERROR_INVALID_PARAM = -2,
    MBLINK_ERROR_NOT_FOUND = -3,
    MBLINK_ERROR_TYPE_MISMATCH = -4,
    MBLINK_ERROR_OUT_OF_RANGE = -5,
    MBLINK_ERROR_JS_ERROR = -6,
    MBLINK_ERROR_UNKNOWN = -99
} MBlinkError;

typedef enum {
    MBLINK_TYPE_NULL = 0,
    MBLINK_TYPE_BOOL = 1,
    MBLINK_TYPE_INT = 2,
    MBLINK_TYPE_DOUBLE = 3,
    MBLINK_TYPE_STRING = 4,
    MBLINK_TYPE_ARRAY = 5,
    MBLINK_TYPE_OBJECT = 6
} MBlinkType;

typedef enum {
    MBLINK_LIFECYCLE_CREATED = 0,
    MBLINK_LIFECYCLE_LOADED = 1,
    MBLINK_LIFECYCLE_RUNNING = 2,
    MBLINK_LIFECYCLE_CLOSE_REQUESTED = 3,
    MBLINK_LIFECYCLE_STOPPED = 4,
    MBLINK_LIFECYCLE_DESTROYED = 5
} MBlinkLifecycleState;

typedef enum {
    MBLINK_OBSERVE_CONSOLE = 1,
    MBLINK_OBSERVE_ERROR = 2
} MBlinkObserveKind;

// ========== 窗口配置 ==========

typedef struct {
    const char* title;
    int width;
    int height;
    bool headless;
    bool borderless;
    bool transparent;
    bool always_on_top;
    bool resizable;
    bool gpu;
    bool fullscreen;
    int resize_border_width;
    int min_width, min_height;
    int max_width, max_height;
} MBlinkConfig;

typedef struct {
    const char* runtime_epoch;
    bool load_embedded_runtime;
    bool load_official_preact;
} MBlinkRuntimeOptions;

// ========== 回调类型 ==========

// 函数绑定回调: JS 调用 backend.xxx() 时触发，返回 JSON 字符串。
// 返回值必须由 MBlink 运行时通过 mblink_free() 释放。
// 建议绑定层使用 mblink_copy_string() 分配返回字符串，确保分配/释放在同一运行时。
typedef char* (*MBlinkCallback)(const char* args_json, void* user_data);
typedef char* (*MBlinkAsyncCallback)(const char* args_json, void* user_data);

// 状态变更回调
typedef void (*MBlinkStateCallback)(const char* name, const char* value_json,
                                     void* user_data);

// 事件回调
typedef void (*MBlinkResizeCallback)(int width, int height, void* user_data);
typedef void (*MBlinkVoidCallback)(void* user_data);
typedef bool (*MBlinkBoolCallback)(void* user_data);
typedef void (*MBlinkUpdateCallback)(float delta_time, void* user_data);
typedef void (*MBlinkObserveCallback)(MBlinkObserveKind kind,
                                     const char* entry_json,
                                     void* user_data);

// ========== 生命周期 ==========

MBLINK_API int mblink_init(void);
MBLINK_API void mblink_cleanup(void);
MBLINK_API const char* mblink_version(void);

// ========== 窗口管理 ==========

/** 创建窗口（简单版，使用默认配置） */
MBLINK_API MBlinkHandle mblink_create(const char* title, int width,
                                         int height);

/** 创建窗口（完整版，使用 MBlinkConfig） */
MBLINK_API MBlinkHandle mblink_create_ex(const MBlinkConfig* config);

/** 获取默认配置（可修改后传给 mblink_create_ex） */
MBLINK_API MBlinkConfig mblink_default_config(void);

MBLINK_API void mblink_destroy(MBlinkHandle handle);

/** 阻塞运行事件循环，直到窗口关闭或调用 mblink_stop() */
MBLINK_API void mblink_run(MBlinkHandle handle);
MBLINK_API void mblink_stop(MBlinkHandle handle);

/** 单次事件循环迭代（高级用法） */
MBLINK_API bool mblink_poll_events(MBlinkHandle handle);
MBLINK_API bool mblink_wait_events(MBlinkHandle handle);
MBLINK_API MBlinkRuntimeOptions mblink_default_runtime_options(void);
MBLINK_API int mblink_configure_runtime(MBlinkHandle handle,
                                      const MBlinkRuntimeOptions* options);
MBLINK_API int mblink_load_embedded_runtime(MBlinkHandle handle,
                                          bool include_official_preact);
MBLINK_API int mblink_load_entry_file(MBlinkHandle handle, const char* entry_path,
                                    bool execute_html_scripts);
MBLINK_API int mblink_load_module_file(MBlinkHandle handle,
                                     const char* entry_path);
MBLINK_API int mblink_render_frame(MBlinkHandle handle, int passes);
MBLINK_API int mblink_runtime_epoch(MBlinkHandle handle, char** out_epoch);
MBLINK_API MBlinkLifecycleState mblink_lifecycle_state(MBlinkHandle handle);
MBLINK_API int mblink_lifecycle_reason(MBlinkHandle handle, char** out_reason);

// ========== 窗口属性 ==========

MBLINK_API int mblink_set_title(MBlinkHandle handle, const char* title);
MBLINK_API int mblink_tray_create(MBlinkHandle handle, const char* tooltip);
MBLINK_API int mblink_tray_destroy(MBlinkHandle handle);
MBLINK_API int mblink_tray_set_tooltip(MBlinkHandle handle, const char* tooltip);
MBLINK_API int mblink_tray_set_menu(MBlinkHandle handle, const char* menu_json);
MBLINK_API int mblink_tray_set_left_click_callback(MBlinkHandle handle,
                                                 MBlinkVoidCallback callback,
                                                 void* user_data);
MBLINK_API int mblink_tray_set_menu_callback(MBlinkHandle handle,
                                           MBlinkCallback callback,
                                           void* user_data);
MBLINK_API int mblink_set_size(MBlinkHandle handle, int width, int height);
MBLINK_API int mblink_get_size(MBlinkHandle handle, int* width, int* height);
MBLINK_API int mblink_set_position(MBlinkHandle handle, int x, int y);
MBLINK_API int mblink_get_position(MBlinkHandle handle, int* x, int* y);
MBLINK_API int mblink_set_min_size(MBlinkHandle handle, int width, int height);
MBLINK_API int mblink_set_max_size(MBlinkHandle handle, int width, int height);
MBLINK_API int mblink_minimize(MBlinkHandle handle);
MBLINK_API int mblink_maximize(MBlinkHandle handle);
MBLINK_API int mblink_restore(MBlinkHandle handle);
MBLINK_API int mblink_show(MBlinkHandle handle);
MBLINK_API int mblink_hide(MBlinkHandle handle);
MBLINK_API int mblink_set_fullscreen(MBlinkHandle handle, bool fullscreen);
MBLINK_API int mblink_set_resizable(MBlinkHandle handle, bool resizable);
MBLINK_API int mblink_set_borderless(MBlinkHandle handle, bool borderless);
MBLINK_API int mblink_set_always_on_top(MBlinkHandle handle, bool on_top);

// ========== UI 加载 ==========

/** 加载 HTML 字符串到窗口 */
MBLINK_API int mblink_load_html(MBlinkHandle handle, const char* html);

/** 从文件路径加载 HTML */
MBLINK_API int mblink_load_html_file(MBlinkHandle handle, const char* filepath);

/** 执行 JavaScript 代码 */
MBLINK_API int mblink_eval_js(MBlinkHandle handle, const char* js_code);

/** 执行 ES 模块代码 */
MBLINK_API int mblink_eval_module(MBlinkHandle handle, const char* code,
                                    const char* filename);

/** 加载 JavaScript 文件 */
MBLINK_API int mblink_load_js_file(MBlinkHandle handle, const char* filepath);

/** 加载 QuickJS 字节码 */
MBLINK_API int mblink_load_bytecode(MBlinkHandle handle, const void* data,
                                      size_t size);

/** 编译资源文件/目录为加密资源包；.js/.mjs 会编译为 QuickJS bytecode */
MBLINK_API int mblink_compile_resources(const char* input_path,
                                      const char* output_file,
                                      const char* encryption_key);

/** 从资源包按路径加载文件；返回数据需用 mblink_free 释放 */
MBLINK_API int mblink_load_resource_file(const char* package_file,
                                       const char* resource_path,
                                       const char* encryption_key,
                                       void** out_data,
                                       size_t* out_size,
                                       uint32_t* out_flags);

/** 挂载资源包；挂载后 load_html_file/load_js_file/import 优先从资源包解析 */
MBLINK_API int mblink_mount_resource_package(MBlinkHandle handle,
                                           const char* package_file,
                                           const char* encryption_key,
                                           const char* mount_point);

// ========== 函数绑定 ==========

/** 绑定宿主函数，JS 中通过 backend.name(args) 调用 */
MBLINK_API int mblink_bind(MBlinkHandle handle, const char* name,
                             MBlinkCallback callback, void* user_data);
/** 绑定异步宿主函数，JS 中通过 await backend.name(args) 调用 */
MBLINK_API int mblink_bind_async(MBlinkHandle handle, const char* name,
                               MBlinkAsyncCallback callback, void* user_data);
MBLINK_API void mblink_unbind(MBlinkHandle handle, const char* name);

// ========== 事件回调 ==========

MBLINK_API int mblink_on_resize(MBlinkHandle handle, MBlinkResizeCallback callback, void* user_data);
MBLINK_API int mblink_on_close_request(MBlinkHandle handle, MBlinkBoolCallback callback, void* user_data);
MBLINK_API int mblink_on_close(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data);
MBLINK_API int mblink_on_focus(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data);
MBLINK_API int mblink_on_blur(MBlinkHandle handle, MBlinkVoidCallback callback, void* user_data);
MBLINK_API int mblink_on_update(MBlinkHandle handle, MBlinkUpdateCallback callback, void* user_data);

// ========== 事件发送 ==========

/** 从宿主语言向 JS 端发送事件 */
MBLINK_API int mblink_emit(MBlinkHandle handle, const char* event_name,
                             const char* data_json);

// ========== Observability ==========

MBLINK_API int mblink_observe_set_callback(MBlinkHandle handle,
                                         MBlinkObserveCallback callback,
                                         void* user_data);
MBLINK_API int mblink_observe_console_json(MBlinkHandle handle, char** out_json);
MBLINK_API int mblink_observe_errors_json(MBlinkHandle handle, char** out_json);
MBLINK_API int mblink_observe_lifecycle_json(MBlinkHandle handle, char** out_json);
MBLINK_API int mblink_observe_clear(MBlinkHandle handle, MBlinkObserveKind kind);

// ========== 状态创建 ==========

MBLINK_API int mblink_state_create_null(MBlinkHandle handle,
                                          const char* name);
MBLINK_API int mblink_state_create_bool(MBlinkHandle handle,
                                          const char* name, bool value);
MBLINK_API int mblink_state_create_int(MBlinkHandle handle, const char* name,
                                         int64_t value);
MBLINK_API int mblink_state_create_double(MBlinkHandle handle,
                                            const char* name, double value);
MBLINK_API int mblink_state_create_string(MBlinkHandle handle,
                                            const char* name,
                                            const char* value);
MBLINK_API int mblink_state_create_array(MBlinkHandle handle,
                                           const char* name);
MBLINK_API int mblink_state_create_object(MBlinkHandle handle,
                                            const char* name);
MBLINK_API int mblink_state_create_json(MBlinkHandle handle,
                                          const char* name, const char* json);

// ========== 状态查询 ==========

MBLINK_API bool mblink_state_exists(MBlinkHandle handle, const char* name);
MBLINK_API MBlinkType mblink_state_type(MBlinkHandle handle,
                                           const char* name);
MBLINK_API void mblink_state_delete(MBlinkHandle handle, const char* name);

// ========== 状态读取（直接类型，无需 free） ==========

MBLINK_API bool mblink_state_get_bool(MBlinkHandle handle, const char* name);
MBLINK_API int64_t mblink_state_get_int(MBlinkHandle handle,
                                          const char* name);
MBLINK_API double mblink_state_get_double(MBlinkHandle handle,
                                            const char* name);
MBLINK_API const char* mblink_state_get_string(MBlinkHandle handle,
                                                 const char* name);
MBLINK_API int mblink_state_get_length(MBlinkHandle handle,
                                         const char* name);

// ========== 状态读取（JSON，需要 free） ==========

MBLINK_API char* mblink_state_get_json(MBlinkHandle handle,
                                         const char* name);
MBLINK_API char* mblink_state_get_at(MBlinkHandle handle, const char* name,
                                       int index);
MBLINK_API char* mblink_state_get_key(MBlinkHandle handle, const char* name,
                                        const char* key);

// ========== 状态写入（直接类型，线程安全） ==========

MBLINK_API int mblink_state_set_null(MBlinkHandle handle, const char* name);
MBLINK_API int mblink_state_set_bool(MBlinkHandle handle, const char* name,
                                       bool value);
MBLINK_API int mblink_state_set_int(MBlinkHandle handle, const char* name,
                                      int64_t value);
MBLINK_API int mblink_state_set_double(MBlinkHandle handle, const char* name,
                                         double value);
MBLINK_API int mblink_state_set_string(MBlinkHandle handle, const char* name,
                                         const char* value);
MBLINK_API int mblink_state_set_json(MBlinkHandle handle, const char* name,
                                       const char* json);

// ========== 数组操作（线程安全） ==========

MBLINK_API int mblink_state_array_push(MBlinkHandle handle, const char* name,
                                         const char* item_json);
MBLINK_API int mblink_state_array_push_int(MBlinkHandle handle,
                                             const char* name, int64_t value);
MBLINK_API int mblink_state_array_push_double(MBlinkHandle handle,
                                                const char* name, double value);
MBLINK_API int mblink_state_array_push_string(MBlinkHandle handle,
                                                const char* name,
                                                const char* value);
MBLINK_API int mblink_state_array_push_bool(MBlinkHandle handle,
                                              const char* name, bool value);
MBLINK_API int mblink_state_array_pop(MBlinkHandle handle, const char* name);
MBLINK_API int mblink_state_array_shift(MBlinkHandle handle,
                                          const char* name);
MBLINK_API int mblink_state_array_unshift(MBlinkHandle handle,
                                            const char* name,
                                            const char* item_json);
MBLINK_API int mblink_state_array_remove(MBlinkHandle handle,
                                           const char* name, int index);
MBLINK_API int mblink_state_array_clear(MBlinkHandle handle,
                                          const char* name);
MBLINK_API int mblink_state_array_set(MBlinkHandle handle, const char* name,
                                        int index, const char* item_json);
MBLINK_API int mblink_state_array_set_int(MBlinkHandle handle,
                                            const char* name, int index,
                                            int64_t value);
MBLINK_API int mblink_state_array_set_double(MBlinkHandle handle,
                                               const char* name, int index,
                                               double value);
MBLINK_API int mblink_state_array_set_string(MBlinkHandle handle,
                                               const char* name, int index,
                                               const char* value);

// ========== 对象操作（线程安全） ==========

MBLINK_API int mblink_state_object_set(MBlinkHandle handle, const char* name,
                                         const char* key,
                                         const char* value_json);
MBLINK_API int mblink_state_object_set_int(MBlinkHandle handle,
                                             const char* name, const char* key,
                                             int64_t value);
MBLINK_API int mblink_state_object_set_double(MBlinkHandle handle,
                                                const char* name,
                                                const char* key, double value);
MBLINK_API int mblink_state_object_set_string(MBlinkHandle handle,
                                                const char* name,
                                                const char* key,
                                                const char* value);
MBLINK_API int mblink_state_object_set_bool(MBlinkHandle handle,
                                              const char* name, const char* key,
                                              bool value);
MBLINK_API int mblink_state_object_remove(MBlinkHandle handle,
                                            const char* name, const char* key);
MBLINK_API int mblink_state_object_clear(MBlinkHandle handle,
                                           const char* name);

// ========== 数值操作（线程安全，原子） ==========

MBLINK_API int mblink_state_increment(MBlinkHandle handle, const char* name,
                                        double delta);
MBLINK_API int mblink_state_multiply(MBlinkHandle handle, const char* name,
                                       double factor);

// ========== 字符串操作（线程安全） ==========

MBLINK_API int mblink_state_string_append(MBlinkHandle handle,
                                            const char* name,
                                            const char* suffix);
MBLINK_API int mblink_state_string_prepend(MBlinkHandle handle,
                                             const char* name,
                                             const char* prefix);

// ========== 监听 ==========

MBLINK_API int mblink_state_watch(MBlinkHandle handle, const char* name,
                                    MBlinkStateCallback callback,
                                    void* user_data);
MBLINK_API void mblink_state_unwatch(MBlinkHandle handle, int watch_id);

// ========== 批量操作 ==========

MBLINK_API void mblink_state_batch_begin(MBlinkHandle handle);
MBLINK_API void mblink_state_batch_end(MBlinkHandle handle);

// ========== 队列控制 ==========

MBLINK_API void mblink_state_set_merge_mode(MBlinkHandle handle,
                                              bool enable);
MBLINK_API int mblink_process_queue(MBlinkHandle handle);
MBLINK_API int mblink_queue_size(MBlinkHandle handle);

// ========== 共享 C 对象 (SharedObject) ==========
//
// 核心思想：Python/JS 共享同一个 QuickJS JSValue 对象。
// - Python 通过 ctypes 调用 set/get 操作同一个 C 对象
// - JS 通过 globalThis.<name> 直接读写同一个对象
// - Python 写入后自动触发 JS 内部 shared update 调度 → UI 刷新
//
// 用法：
//   Python: data = app.shared("data"); data.count = 0
//   JS:     data.count  →  0
//           py.increment()  →  Python: data.count += 1  →  UI 自动更新

// 不透明句柄
typedef struct MBlinkSharedObject* MBlinkSharedHandle;
typedef struct MBlinkLogViewObject* MBlinkLogViewHandle;
typedef struct MBlinkTerminalObject* MBlinkTerminalHandle;

// 创建共享对象，注册为 JS globalThis.<name>
MBLINK_API MBlinkSharedHandle mblink_shared_create(MBlinkHandle handle,
                                                       const char* name);

// 销毁共享对象
MBLINK_API void mblink_shared_destroy(MBlinkSharedHandle shared);

// ---- 类型化 setter（自动触发内部 shared update 调度） ----

MBLINK_API int mblink_shared_set_int(MBlinkSharedHandle shared,
                                        const char* key, int64_t value);
MBLINK_API int mblink_shared_set_double(MBlinkSharedHandle shared,
                                           const char* key, double value);
MBLINK_API int mblink_shared_set_string(MBlinkSharedHandle shared,
                                           const char* key, const char* value);
MBLINK_API int mblink_shared_set_bool(MBlinkSharedHandle shared,
                                         const char* key, bool value);
MBLINK_API int mblink_shared_set_null(MBlinkSharedHandle shared,
                                         const char* key);
MBLINK_API int mblink_shared_set_json(MBlinkSharedHandle shared,
                                         const char* key, const char* json_str);

// ---- 类型化 getter ----

MBLINK_API int64_t mblink_shared_get_int(MBlinkSharedHandle shared,
                                            const char* key);
MBLINK_API double mblink_shared_get_double(MBlinkSharedHandle shared,
                                              const char* key);
// 返回值由调用者通过 mblink_free() 释放
MBLINK_API const char* mblink_shared_get_string(MBlinkSharedHandle shared,
                                                    const char* key);
MBLINK_API bool mblink_shared_get_bool(MBlinkSharedHandle shared,
                                          const char* key);
// 返回 JSON 字符串，调用者通过 mblink_free() 释放
MBLINK_API const char* mblink_shared_get_json(MBlinkSharedHandle shared,
                                                  const char* key);

// ---- 属性查询 ----

MBLINK_API int mblink_shared_get_type(MBlinkSharedHandle shared,
                                         const char* key);
MBLINK_API int mblink_shared_delete(MBlinkSharedHandle shared,
                                       const char* key);
MBLINK_API bool mblink_shared_has(MBlinkSharedHandle shared,
                                     const char* key);

// ---- 批量更新（抑制中间 shared update 调用） ----

MBLINK_API void mblink_shared_batch_begin(MBlinkSharedHandle shared);
MBLINK_API void mblink_shared_batch_end(MBlinkSharedHandle shared);

// ========== 原生 UI 对象句柄（LogView / Terminal） ==========

MBLINK_API MBlinkLogViewHandle mblink_logview_get(MBlinkHandle handle,
                                                    const char* element_id);
MBLINK_API void mblink_logview_destroy(MBlinkLogViewHandle logview);
MBLINK_API int mblink_logview_append(MBlinkLogViewHandle logview,
                                        const char* level,
                                        const char* source,
                                        const char* message);
MBLINK_API void mblink_logview_clear(MBlinkLogViewHandle logview);
MBLINK_API const char* mblink_logview_export(MBlinkLogViewHandle logview,
                                                const char* format);

MBLINK_API MBlinkTerminalHandle mblink_terminal_get(MBlinkHandle handle,
                                                      const char* element_id);
MBLINK_API void mblink_terminal_destroy(MBlinkTerminalHandle terminal);
MBLINK_API int mblink_terminal_write(MBlinkTerminalHandle terminal,
                                        const char* data);
MBLINK_API void mblink_terminal_clear(MBlinkTerminalHandle terminal);
MBLINK_API int mblink_terminal_execute(MBlinkTerminalHandle terminal,
                                          const char* command);
MBLINK_API int mblink_terminal_start_shell(MBlinkTerminalHandle terminal,
                                              const char* shell);
MBLINK_API int mblink_terminal_send_input(MBlinkTerminalHandle terminal,
                                             const char* input);
MBLINK_API void mblink_terminal_resize(MBlinkTerminalHandle terminal,
                                          int rows, int cols);
MBLINK_API const char* mblink_terminal_serialize(MBlinkTerminalHandle terminal);

// ========== 工具函数 ==========

// 拷贝字符串到 MBlink 运行时分配的内存；调用者需通过 mblink_free() 释放
MBLINK_API char* mblink_copy_string(const char* str);

MBLINK_API void mblink_free(void* ptr);
MBLINK_API const char* mblink_last_error(void);

#ifdef __cplusplus
}

namespace mblink {
struct DevToolsBridgeApi;
struct DevToolsHostServices;
}

extern "C" {
MBLINK_API int mblink_devtools_host_attach(unsigned int version,
                                         const mblink::DevToolsBridgeApi* api);
MBLINK_API void mblink_devtools_host_detach(unsigned int version,
                                          const mblink::DevToolsBridgeApi* api);
MBLINK_API int mblink_devtools_host_get_services(unsigned int version,
                                               mblink::DevToolsHostServices* out_services);
}
#endif
