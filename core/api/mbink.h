/**
 * @file mbink.h
 * @brief MBink C API v2 - 跨语言绑定统一接口
 *
 * 这是 MBink 框架面向所有语言（Python/Go/Rust/Node.js）的唯一入口。
 * 所有绑定都通过此 C ABI 调用 mbink.dll / libmbink.so。
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
  #ifdef MBINK_BUILDING_DLL
    #define MBINK_API __declspec(dllexport)
  #else
    #define MBINK_API __declspec(dllimport)
  #endif
#else
  #define MBINK_API __attribute__((visibility("default")))
#endif

// ========== 类型定义 ==========

typedef struct MBinkWindow* MBinkHandle;

typedef enum {
    MBINK_OK = 0,
    MBINK_ERROR_INVALID_HANDLE = -1,
    MBINK_ERROR_INVALID_PARAM = -2,
    MBINK_ERROR_NOT_FOUND = -3,
    MBINK_ERROR_TYPE_MISMATCH = -4,
    MBINK_ERROR_OUT_OF_RANGE = -5,
    MBINK_ERROR_JS_ERROR = -6,
    MBINK_ERROR_UNKNOWN = -99
} MBinkError;

typedef enum {
    MBINK_TYPE_NULL = 0,
    MBINK_TYPE_BOOL = 1,
    MBINK_TYPE_INT = 2,
    MBINK_TYPE_DOUBLE = 3,
    MBINK_TYPE_STRING = 4,
    MBINK_TYPE_ARRAY = 5,
    MBINK_TYPE_OBJECT = 6
} MBinkType;

typedef enum {
    MBINK_LIFECYCLE_CREATED = 0,
    MBINK_LIFECYCLE_LOADED = 1,
    MBINK_LIFECYCLE_RUNNING = 2,
    MBINK_LIFECYCLE_CLOSE_REQUESTED = 3,
    MBINK_LIFECYCLE_STOPPED = 4,
    MBINK_LIFECYCLE_DESTROYED = 5
} MBinkLifecycleState;

typedef enum {
    MBINK_OBSERVE_CONSOLE = 1,
    MBINK_OBSERVE_ERROR = 2
} MBinkObserveKind;

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
} MBinkConfig;

typedef struct {
    const char* runtime_epoch;
    bool load_embedded_runtime;
    bool load_official_preact;
} MBinkRuntimeOptions;

typedef struct {
    const char* runtime_epoch;
    size_t max_nodes;
    int max_depth;
    const char* root_selector;
    bool include_screenshot;
    bool inline_screenshot;
    const char* screenshot_file;
} MBinkUiDevSnapshotOptions;

// ========== 回调类型 ==========

// 函数绑定回调: JS 调用 backend.xxx() 时触发，返回 JSON 字符串。
// 返回值必须由 MBink 运行时通过 mbink_free() 释放。
// 建议绑定层使用 mbink_copy_string() 分配返回字符串，确保分配/释放在同一运行时。
typedef char* (*MBinkCallback)(const char* args_json, void* user_data);
typedef char* (*MBinkAsyncCallback)(const char* args_json, void* user_data);

// 状态变更回调
typedef void (*MBinkStateCallback)(const char* name, const char* value_json,
                                     void* user_data);

// 事件回调
typedef void (*MBinkResizeCallback)(int width, int height, void* user_data);
typedef void (*MBinkVoidCallback)(void* user_data);
typedef bool (*MBinkBoolCallback)(void* user_data);
typedef void (*MBinkUpdateCallback)(float delta_time, void* user_data);
typedef void (*MBinkObserveCallback)(MBinkObserveKind kind,
                                     const char* entry_json,
                                     void* user_data);

// ========== 生命周期 ==========

MBINK_API int mbink_init(void);
MBINK_API void mbink_cleanup(void);
MBINK_API const char* mbink_version(void);

// ========== 窗口管理 ==========

/** 创建窗口（简单版，使用默认配置） */
MBINK_API MBinkHandle mbink_create(const char* title, int width,
                                         int height);

/** 创建窗口（完整版，使用 MBinkConfig） */
MBINK_API MBinkHandle mbink_create_ex(const MBinkConfig* config);

/** 获取默认配置（可修改后传给 mbink_create_ex） */
MBINK_API MBinkConfig mbink_default_config(void);

MBINK_API void mbink_destroy(MBinkHandle handle);

/** 阻塞运行事件循环，直到窗口关闭或调用 mbink_stop() */
MBINK_API void mbink_run(MBinkHandle handle);
MBINK_API void mbink_stop(MBinkHandle handle);

/** 单次事件循环迭代（高级用法） */
MBINK_API bool mbink_poll_events(MBinkHandle handle);
MBINK_API MBinkRuntimeOptions mbink_default_runtime_options(void);
MBINK_API int mbink_configure_runtime(MBinkHandle handle,
                                      const MBinkRuntimeOptions* options);
MBINK_API int mbink_load_embedded_runtime(MBinkHandle handle,
                                          bool include_official_preact);
MBINK_API int mbink_load_entry_file(MBinkHandle handle, const char* entry_path,
                                    bool execute_html_scripts);
MBINK_API int mbink_load_module_file(MBinkHandle handle,
                                     const char* entry_path);
MBINK_API int mbink_render_frame(MBinkHandle handle, int passes);
MBINK_API int mbink_runtime_epoch(MBinkHandle handle, char** out_epoch);
MBINK_API MBinkLifecycleState mbink_lifecycle_state(MBinkHandle handle);
MBINK_API int mbink_lifecycle_reason(MBinkHandle handle, char** out_reason);

// ========== 窗口属性 ==========

MBINK_API int mbink_set_title(MBinkHandle handle, const char* title);
MBINK_API int mbink_tray_create(MBinkHandle handle, const char* tooltip);
MBINK_API int mbink_tray_destroy(MBinkHandle handle);
MBINK_API int mbink_tray_set_tooltip(MBinkHandle handle, const char* tooltip);
MBINK_API int mbink_tray_set_menu(MBinkHandle handle, const char* menu_json);
MBINK_API int mbink_tray_set_left_click_callback(MBinkHandle handle,
                                                 MBinkVoidCallback callback,
                                                 void* user_data);
MBINK_API int mbink_tray_set_menu_callback(MBinkHandle handle,
                                           MBinkCallback callback,
                                           void* user_data);
MBINK_API int mbink_set_size(MBinkHandle handle, int width, int height);
MBINK_API int mbink_get_size(MBinkHandle handle, int* width, int* height);
MBINK_API int mbink_set_position(MBinkHandle handle, int x, int y);
MBINK_API int mbink_get_position(MBinkHandle handle, int* x, int* y);
MBINK_API int mbink_set_min_size(MBinkHandle handle, int width, int height);
MBINK_API int mbink_set_max_size(MBinkHandle handle, int width, int height);
MBINK_API int mbink_minimize(MBinkHandle handle);
MBINK_API int mbink_maximize(MBinkHandle handle);
MBINK_API int mbink_restore(MBinkHandle handle);
MBINK_API int mbink_show(MBinkHandle handle);
MBINK_API int mbink_hide(MBinkHandle handle);
MBINK_API int mbink_set_fullscreen(MBinkHandle handle, bool fullscreen);
MBINK_API int mbink_set_resizable(MBinkHandle handle, bool resizable);
MBINK_API int mbink_set_borderless(MBinkHandle handle, bool borderless);
MBINK_API int mbink_set_always_on_top(MBinkHandle handle, bool on_top);

// ========== UI 加载 ==========

/** 加载 HTML 字符串到窗口 */
MBINK_API int mbink_load_html(MBinkHandle handle, const char* html);

/** 从文件路径加载 HTML */
MBINK_API int mbink_load_html_file(MBinkHandle handle, const char* filepath);

/** 执行 JavaScript 代码 */
MBINK_API int mbink_eval_js(MBinkHandle handle, const char* js_code);

/** 执行 ES 模块代码 */
MBINK_API int mbink_eval_module(MBinkHandle handle, const char* code,
                                    const char* filename);

/** 加载 JavaScript 文件 */
MBINK_API int mbink_load_js_file(MBinkHandle handle, const char* filepath);

/** 加载 QuickJS 字节码 */
MBINK_API int mbink_load_bytecode(MBinkHandle handle, const void* data,
                                      size_t size);

/** 编译资源文件/目录为加密资源包；.js/.mjs 会编译为 QuickJS bytecode */
MBINK_API int mbink_compile_resources(const char* input_path,
                                      const char* output_file,
                                      const char* encryption_key);

/** 从资源包按路径加载文件；返回数据需用 mbink_free 释放 */
MBINK_API int mbink_load_resource_file(const char* package_file,
                                       const char* resource_path,
                                       const char* encryption_key,
                                       void** out_data,
                                       size_t* out_size,
                                       uint32_t* out_flags);

/** 挂载资源包；挂载后 load_html_file/load_js_file/import 优先从资源包解析 */
MBINK_API int mbink_mount_resource_package(MBinkHandle handle,
                                           const char* package_file,
                                           const char* encryption_key,
                                           const char* mount_point);

// ========== 函数绑定 ==========

/** 绑定宿主函数，JS 中通过 backend.name(args) 调用 */
MBINK_API int mbink_bind(MBinkHandle handle, const char* name,
                             MBinkCallback callback, void* user_data);
/** 绑定异步宿主函数，JS 中通过 await backend.name(args) 调用 */
MBINK_API int mbink_bind_async(MBinkHandle handle, const char* name,
                               MBinkAsyncCallback callback, void* user_data);
MBINK_API void mbink_unbind(MBinkHandle handle, const char* name);

// ========== 事件回调 ==========

MBINK_API int mbink_on_resize(MBinkHandle handle, MBinkResizeCallback callback, void* user_data);
MBINK_API int mbink_on_close_request(MBinkHandle handle, MBinkBoolCallback callback, void* user_data);
MBINK_API int mbink_on_close(MBinkHandle handle, MBinkVoidCallback callback, void* user_data);
MBINK_API int mbink_on_focus(MBinkHandle handle, MBinkVoidCallback callback, void* user_data);
MBINK_API int mbink_on_blur(MBinkHandle handle, MBinkVoidCallback callback, void* user_data);
MBINK_API int mbink_on_update(MBinkHandle handle, MBinkUpdateCallback callback, void* user_data);

// ========== 事件发送 ==========

/** 从宿主语言向 JS 端发送事件 */
MBINK_API int mbink_emit(MBinkHandle handle, const char* event_name,
                             const char* data_json);

// ========== DevTools ==========

MBINK_API int mbink_devtools_open(MBinkHandle handle);
MBINK_API int mbink_devtools_close(MBinkHandle handle);

// ========== Observability ==========

MBINK_API int mbink_observe_set_callback(MBinkHandle handle,
                                         MBinkObserveCallback callback,
                                         void* user_data);
MBINK_API int mbink_observe_console_json(MBinkHandle handle, char** out_json);
MBINK_API int mbink_observe_errors_json(MBinkHandle handle, char** out_json);
MBINK_API int mbink_observe_lifecycle_json(MBinkHandle handle, char** out_json);
MBINK_API int mbink_observe_clear(MBinkHandle handle, MBinkObserveKind kind);

// ========== UI Dev / Control ==========

MBINK_API MBinkUiDevSnapshotOptions mbink_ui_dev_default_snapshot_options(void);
MBINK_API int mbink_ui_dev_snapshot_json(MBinkHandle handle,
                                         const MBinkUiDevSnapshotOptions* options,
                                         char** out_json);
MBINK_API int mbink_ui_dev_snapshot_file(MBinkHandle handle,
                                         const char* output_path,
                                         const MBinkUiDevSnapshotOptions* options);
MBINK_API int mbink_ui_dev_command_json(MBinkHandle handle,
                                        const char* command_json,
                                        char** out_response_json);


// ========== 状态创建 ==========

MBINK_API int mbink_state_create_null(MBinkHandle handle,
                                          const char* name);
MBINK_API int mbink_state_create_bool(MBinkHandle handle,
                                          const char* name, bool value);
MBINK_API int mbink_state_create_int(MBinkHandle handle, const char* name,
                                         int64_t value);
MBINK_API int mbink_state_create_double(MBinkHandle handle,
                                            const char* name, double value);
MBINK_API int mbink_state_create_string(MBinkHandle handle,
                                            const char* name,
                                            const char* value);
MBINK_API int mbink_state_create_array(MBinkHandle handle,
                                           const char* name);
MBINK_API int mbink_state_create_object(MBinkHandle handle,
                                            const char* name);
MBINK_API int mbink_state_create_json(MBinkHandle handle,
                                          const char* name, const char* json);

// ========== 状态查询 ==========

MBINK_API bool mbink_state_exists(MBinkHandle handle, const char* name);
MBINK_API MBinkType mbink_state_type(MBinkHandle handle,
                                           const char* name);
MBINK_API void mbink_state_delete(MBinkHandle handle, const char* name);

// ========== 状态读取（直接类型，无需 free） ==========

MBINK_API bool mbink_state_get_bool(MBinkHandle handle, const char* name);
MBINK_API int64_t mbink_state_get_int(MBinkHandle handle,
                                          const char* name);
MBINK_API double mbink_state_get_double(MBinkHandle handle,
                                            const char* name);
MBINK_API const char* mbink_state_get_string(MBinkHandle handle,
                                                 const char* name);
MBINK_API int mbink_state_get_length(MBinkHandle handle,
                                         const char* name);

// ========== 状态读取（JSON，需要 free） ==========

MBINK_API char* mbink_state_get_json(MBinkHandle handle,
                                         const char* name);
MBINK_API char* mbink_state_get_at(MBinkHandle handle, const char* name,
                                       int index);
MBINK_API char* mbink_state_get_key(MBinkHandle handle, const char* name,
                                        const char* key);

// ========== 状态写入（直接类型，线程安全） ==========

MBINK_API int mbink_state_set_null(MBinkHandle handle, const char* name);
MBINK_API int mbink_state_set_bool(MBinkHandle handle, const char* name,
                                       bool value);
MBINK_API int mbink_state_set_int(MBinkHandle handle, const char* name,
                                      int64_t value);
MBINK_API int mbink_state_set_double(MBinkHandle handle, const char* name,
                                         double value);
MBINK_API int mbink_state_set_string(MBinkHandle handle, const char* name,
                                         const char* value);
MBINK_API int mbink_state_set_json(MBinkHandle handle, const char* name,
                                       const char* json);

// ========== 数组操作（线程安全） ==========

MBINK_API int mbink_state_array_push(MBinkHandle handle, const char* name,
                                         const char* item_json);
MBINK_API int mbink_state_array_push_int(MBinkHandle handle,
                                             const char* name, int64_t value);
MBINK_API int mbink_state_array_push_double(MBinkHandle handle,
                                                const char* name, double value);
MBINK_API int mbink_state_array_push_string(MBinkHandle handle,
                                                const char* name,
                                                const char* value);
MBINK_API int mbink_state_array_push_bool(MBinkHandle handle,
                                              const char* name, bool value);
MBINK_API int mbink_state_array_pop(MBinkHandle handle, const char* name);
MBINK_API int mbink_state_array_shift(MBinkHandle handle,
                                          const char* name);
MBINK_API int mbink_state_array_unshift(MBinkHandle handle,
                                            const char* name,
                                            const char* item_json);
MBINK_API int mbink_state_array_remove(MBinkHandle handle,
                                           const char* name, int index);
MBINK_API int mbink_state_array_clear(MBinkHandle handle,
                                          const char* name);
MBINK_API int mbink_state_array_set(MBinkHandle handle, const char* name,
                                        int index, const char* item_json);
MBINK_API int mbink_state_array_set_int(MBinkHandle handle,
                                            const char* name, int index,
                                            int64_t value);
MBINK_API int mbink_state_array_set_double(MBinkHandle handle,
                                               const char* name, int index,
                                               double value);
MBINK_API int mbink_state_array_set_string(MBinkHandle handle,
                                               const char* name, int index,
                                               const char* value);

// ========== 对象操作（线程安全） ==========

MBINK_API int mbink_state_object_set(MBinkHandle handle, const char* name,
                                         const char* key,
                                         const char* value_json);
MBINK_API int mbink_state_object_set_int(MBinkHandle handle,
                                             const char* name, const char* key,
                                             int64_t value);
MBINK_API int mbink_state_object_set_double(MBinkHandle handle,
                                                const char* name,
                                                const char* key, double value);
MBINK_API int mbink_state_object_set_string(MBinkHandle handle,
                                                const char* name,
                                                const char* key,
                                                const char* value);
MBINK_API int mbink_state_object_set_bool(MBinkHandle handle,
                                              const char* name, const char* key,
                                              bool value);
MBINK_API int mbink_state_object_remove(MBinkHandle handle,
                                            const char* name, const char* key);
MBINK_API int mbink_state_object_clear(MBinkHandle handle,
                                           const char* name);

// ========== 数值操作（线程安全，原子） ==========

MBINK_API int mbink_state_increment(MBinkHandle handle, const char* name,
                                        double delta);
MBINK_API int mbink_state_multiply(MBinkHandle handle, const char* name,
                                       double factor);

// ========== 字符串操作（线程安全） ==========

MBINK_API int mbink_state_string_append(MBinkHandle handle,
                                            const char* name,
                                            const char* suffix);
MBINK_API int mbink_state_string_prepend(MBinkHandle handle,
                                             const char* name,
                                             const char* prefix);

// ========== 监听 ==========

MBINK_API int mbink_state_watch(MBinkHandle handle, const char* name,
                                    MBinkStateCallback callback,
                                    void* user_data);
MBINK_API void mbink_state_unwatch(MBinkHandle handle, int watch_id);

// ========== 批量操作 ==========

MBINK_API void mbink_state_batch_begin(MBinkHandle handle);
MBINK_API void mbink_state_batch_end(MBinkHandle handle);

// ========== 队列控制 ==========

MBINK_API void mbink_state_set_merge_mode(MBinkHandle handle,
                                              bool enable);
MBINK_API int mbink_process_queue(MBinkHandle handle);
MBINK_API int mbink_queue_size(MBinkHandle handle);

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
typedef struct MBinkSharedObject* MBinkSharedHandle;
typedef struct MBinkLogViewObject* MBinkLogViewHandle;
typedef struct MBinkTerminalObject* MBinkTerminalHandle;

// 创建共享对象，注册为 JS globalThis.<name>
MBINK_API MBinkSharedHandle mbink_shared_create(MBinkHandle handle,
                                                       const char* name);

// 销毁共享对象
MBINK_API void mbink_shared_destroy(MBinkSharedHandle shared);

// ---- 类型化 setter（自动触发内部 shared update 调度） ----

MBINK_API int mbink_shared_set_int(MBinkSharedHandle shared,
                                        const char* key, int64_t value);
MBINK_API int mbink_shared_set_double(MBinkSharedHandle shared,
                                           const char* key, double value);
MBINK_API int mbink_shared_set_string(MBinkSharedHandle shared,
                                           const char* key, const char* value);
MBINK_API int mbink_shared_set_bool(MBinkSharedHandle shared,
                                         const char* key, bool value);
MBINK_API int mbink_shared_set_null(MBinkSharedHandle shared,
                                         const char* key);
MBINK_API int mbink_shared_set_json(MBinkSharedHandle shared,
                                         const char* key, const char* json_str);

// ---- 类型化 getter ----

MBINK_API int64_t mbink_shared_get_int(MBinkSharedHandle shared,
                                            const char* key);
MBINK_API double mbink_shared_get_double(MBinkSharedHandle shared,
                                              const char* key);
// 返回值由调用者通过 mbink_free() 释放
MBINK_API const char* mbink_shared_get_string(MBinkSharedHandle shared,
                                                    const char* key);
MBINK_API bool mbink_shared_get_bool(MBinkSharedHandle shared,
                                          const char* key);
// 返回 JSON 字符串，调用者通过 mbink_free() 释放
MBINK_API const char* mbink_shared_get_json(MBinkSharedHandle shared,
                                                  const char* key);

// ---- 属性查询 ----

MBINK_API int mbink_shared_get_type(MBinkSharedHandle shared,
                                         const char* key);
MBINK_API int mbink_shared_delete(MBinkSharedHandle shared,
                                       const char* key);
MBINK_API bool mbink_shared_has(MBinkSharedHandle shared,
                                     const char* key);

// ---- 批量更新（抑制中间 shared update 调用） ----

MBINK_API void mbink_shared_batch_begin(MBinkSharedHandle shared);
MBINK_API void mbink_shared_batch_end(MBinkSharedHandle shared);

// ========== 原生 UI 对象句柄（LogView / Terminal） ==========

MBINK_API MBinkLogViewHandle mbink_logview_get(MBinkHandle handle,
                                                    const char* element_id);
MBINK_API void mbink_logview_destroy(MBinkLogViewHandle logview);
MBINK_API int mbink_logview_append(MBinkLogViewHandle logview,
                                        const char* level,
                                        const char* source,
                                        const char* message);
MBINK_API void mbink_logview_clear(MBinkLogViewHandle logview);
MBINK_API const char* mbink_logview_export(MBinkLogViewHandle logview,
                                                const char* format);

MBINK_API MBinkTerminalHandle mbink_terminal_get(MBinkHandle handle,
                                                      const char* element_id);
MBINK_API void mbink_terminal_destroy(MBinkTerminalHandle terminal);
MBINK_API int mbink_terminal_write(MBinkTerminalHandle terminal,
                                        const char* data);
MBINK_API void mbink_terminal_clear(MBinkTerminalHandle terminal);
MBINK_API int mbink_terminal_execute(MBinkTerminalHandle terminal,
                                          const char* command);
MBINK_API int mbink_terminal_start_shell(MBinkTerminalHandle terminal,
                                              const char* shell);
MBINK_API int mbink_terminal_send_input(MBinkTerminalHandle terminal,
                                             const char* input);
MBINK_API void mbink_terminal_resize(MBinkTerminalHandle terminal,
                                          int rows, int cols);
MBINK_API const char* mbink_terminal_serialize(MBinkTerminalHandle terminal);

// ========== 工具函数 ==========

// 拷贝字符串到 MBink 运行时分配的内存；调用者需通过 mbink_free() 释放
MBINK_API char* mbink_copy_string(const char* str);

MBINK_API void mbink_free(void* ptr);
MBINK_API const char* mbink_last_error(void);

#ifdef __cplusplus
}
#endif
