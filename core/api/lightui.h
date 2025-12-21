/**
 * @file lightui.h
 * @brief LightUI C API - 跨语言绑定接口
 *
 * 特性：
 * - 所有状态操作线程安全
 * - 直接类型接口避免 JSON 序列化开销
 * - 操作队列合并优化
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
#define LIGHTUI_API __declspec(dllexport)
#else
#define LIGHTUI_API __attribute__((visibility("default")))
#endif

// ========== 类型定义 ==========

typedef struct LightUIWindow* LightUIHandle;

typedef enum {
    LIGHTUI_OK = 0,
    LIGHTUI_ERROR_INVALID_HANDLE = -1,
    LIGHTUI_ERROR_INVALID_PARAM = -2,
    LIGHTUI_ERROR_NOT_FOUND = -3,
    LIGHTUI_ERROR_TYPE_MISMATCH = -4,
    LIGHTUI_ERROR_OUT_OF_RANGE = -5,
    LIGHTUI_ERROR_JS_ERROR = -6,
    LIGHTUI_ERROR_UNKNOWN = -99
} LightUIError;

typedef enum {
    LIGHTUI_TYPE_NULL = 0,
    LIGHTUI_TYPE_BOOL = 1,
    LIGHTUI_TYPE_INT = 2,
    LIGHTUI_TYPE_DOUBLE = 3,
    LIGHTUI_TYPE_STRING = 4,
    LIGHTUI_TYPE_ARRAY = 5,
    LIGHTUI_TYPE_OBJECT = 6
} LightUIType;

// 函数绑定回调
typedef char* (*LightUICallback)(const char* args_json, void* user_data);

// 状态变更回调
typedef void (*LightUIStateCallback)(const char* name, const char* value_json,
                                     void* user_data);

// ========== 生命周期 ==========

LIGHTUI_API int lightui_init(void);
LIGHTUI_API void lightui_cleanup(void);
LIGHTUI_API const char* lightui_version(void);

// ========== 窗口管理 ==========

LIGHTUI_API LightUIHandle lightui_create(const char* title, int width,
                                         int height);
LIGHTUI_API void lightui_destroy(LightUIHandle handle);
LIGHTUI_API void lightui_run(LightUIHandle handle);
LIGHTUI_API void lightui_stop(LightUIHandle handle);
LIGHTUI_API bool lightui_poll_events(LightUIHandle handle);

LIGHTUI_API int lightui_set_title(LightUIHandle handle, const char* title);
LIGHTUI_API int lightui_set_size(LightUIHandle handle, int width, int height);

// ========== UI 加载 ==========

LIGHTUI_API int lightui_load_js(LightUIHandle handle, const char* js_code);
LIGHTUI_API int lightui_load_file(LightUIHandle handle, const char* filepath);
LIGHTUI_API int lightui_load_bytecode(LightUIHandle handle, const void* data,
                                      size_t size);

// ========== 函数绑定 ==========

LIGHTUI_API int lightui_bind(LightUIHandle handle, const char* name,
                             LightUICallback callback, void* user_data);
LIGHTUI_API void lightui_unbind(LightUIHandle handle, const char* name);


// ========== 状态创建 ==========

LIGHTUI_API int lightui_state_create_null(LightUIHandle handle,
                                          const char* name);
LIGHTUI_API int lightui_state_create_bool(LightUIHandle handle,
                                          const char* name, bool value);
LIGHTUI_API int lightui_state_create_int(LightUIHandle handle, const char* name,
                                         int64_t value);
LIGHTUI_API int lightui_state_create_double(LightUIHandle handle,
                                            const char* name, double value);
LIGHTUI_API int lightui_state_create_string(LightUIHandle handle,
                                            const char* name,
                                            const char* value);
LIGHTUI_API int lightui_state_create_array(LightUIHandle handle,
                                           const char* name);
LIGHTUI_API int lightui_state_create_object(LightUIHandle handle,
                                            const char* name);
LIGHTUI_API int lightui_state_create_json(LightUIHandle handle,
                                          const char* name, const char* json);

// ========== 状态查询 ==========

LIGHTUI_API bool lightui_state_exists(LightUIHandle handle, const char* name);
LIGHTUI_API LightUIType lightui_state_type(LightUIHandle handle,
                                           const char* name);
LIGHTUI_API void lightui_state_delete(LightUIHandle handle, const char* name);

// ========== 状态读取（直接类型，无需 free） ==========

LIGHTUI_API bool lightui_state_get_bool(LightUIHandle handle, const char* name);
LIGHTUI_API int64_t lightui_state_get_int(LightUIHandle handle,
                                          const char* name);
LIGHTUI_API double lightui_state_get_double(LightUIHandle handle,
                                            const char* name);
LIGHTUI_API const char* lightui_state_get_string(LightUIHandle handle,
                                                 const char* name);
LIGHTUI_API int lightui_state_get_length(LightUIHandle handle,
                                         const char* name);

// ========== 状态读取（JSON，需要 free） ==========

LIGHTUI_API char* lightui_state_get_json(LightUIHandle handle,
                                         const char* name);
LIGHTUI_API char* lightui_state_get_at(LightUIHandle handle, const char* name,
                                       int index);
LIGHTUI_API char* lightui_state_get_key(LightUIHandle handle, const char* name,
                                        const char* key);

// ========== 状态写入（直接类型，线程安全） ==========

LIGHTUI_API int lightui_state_set_null(LightUIHandle handle, const char* name);
LIGHTUI_API int lightui_state_set_bool(LightUIHandle handle, const char* name,
                                       bool value);
LIGHTUI_API int lightui_state_set_int(LightUIHandle handle, const char* name,
                                      int64_t value);
LIGHTUI_API int lightui_state_set_double(LightUIHandle handle, const char* name,
                                         double value);
LIGHTUI_API int lightui_state_set_string(LightUIHandle handle, const char* name,
                                         const char* value);
LIGHTUI_API int lightui_state_set_json(LightUIHandle handle, const char* name,
                                       const char* json);

// ========== 数组操作（线程安全） ==========

LIGHTUI_API int lightui_state_array_push(LightUIHandle handle, const char* name,
                                         const char* item_json);
LIGHTUI_API int lightui_state_array_push_int(LightUIHandle handle,
                                             const char* name, int64_t value);
LIGHTUI_API int lightui_state_array_push_double(LightUIHandle handle,
                                                const char* name, double value);
LIGHTUI_API int lightui_state_array_push_string(LightUIHandle handle,
                                                const char* name,
                                                const char* value);
LIGHTUI_API int lightui_state_array_push_bool(LightUIHandle handle,
                                              const char* name, bool value);
LIGHTUI_API int lightui_state_array_pop(LightUIHandle handle, const char* name);
LIGHTUI_API int lightui_state_array_shift(LightUIHandle handle,
                                          const char* name);
LIGHTUI_API int lightui_state_array_unshift(LightUIHandle handle,
                                            const char* name,
                                            const char* item_json);
LIGHTUI_API int lightui_state_array_remove(LightUIHandle handle,
                                           const char* name, int index);
LIGHTUI_API int lightui_state_array_clear(LightUIHandle handle,
                                          const char* name);
LIGHTUI_API int lightui_state_array_set(LightUIHandle handle, const char* name,
                                        int index, const char* item_json);
LIGHTUI_API int lightui_state_array_set_int(LightUIHandle handle,
                                            const char* name, int index,
                                            int64_t value);
LIGHTUI_API int lightui_state_array_set_double(LightUIHandle handle,
                                               const char* name, int index,
                                               double value);
LIGHTUI_API int lightui_state_array_set_string(LightUIHandle handle,
                                               const char* name, int index,
                                               const char* value);

// ========== 对象操作（线程安全） ==========

LIGHTUI_API int lightui_state_object_set(LightUIHandle handle, const char* name,
                                         const char* key,
                                         const char* value_json);
LIGHTUI_API int lightui_state_object_set_int(LightUIHandle handle,
                                             const char* name, const char* key,
                                             int64_t value);
LIGHTUI_API int lightui_state_object_set_double(LightUIHandle handle,
                                                const char* name,
                                                const char* key, double value);
LIGHTUI_API int lightui_state_object_set_string(LightUIHandle handle,
                                                const char* name,
                                                const char* key,
                                                const char* value);
LIGHTUI_API int lightui_state_object_set_bool(LightUIHandle handle,
                                              const char* name, const char* key,
                                              bool value);
LIGHTUI_API int lightui_state_object_remove(LightUIHandle handle,
                                            const char* name, const char* key);
LIGHTUI_API int lightui_state_object_clear(LightUIHandle handle,
                                           const char* name);

// ========== 数值操作（线程安全，原子） ==========

LIGHTUI_API int lightui_state_increment(LightUIHandle handle, const char* name,
                                        double delta);
LIGHTUI_API int lightui_state_multiply(LightUIHandle handle, const char* name,
                                       double factor);

// ========== 字符串操作（线程安全） ==========

LIGHTUI_API int lightui_state_string_append(LightUIHandle handle,
                                            const char* name,
                                            const char* suffix);
LIGHTUI_API int lightui_state_string_prepend(LightUIHandle handle,
                                             const char* name,
                                             const char* prefix);

// ========== 监听 ==========

LIGHTUI_API int lightui_state_watch(LightUIHandle handle, const char* name,
                                    LightUIStateCallback callback,
                                    void* user_data);
LIGHTUI_API void lightui_state_unwatch(LightUIHandle handle, int watch_id);

// ========== 批量操作 ==========

LIGHTUI_API void lightui_state_batch_begin(LightUIHandle handle);
LIGHTUI_API void lightui_state_batch_end(LightUIHandle handle);

// ========== 队列控制 ==========

LIGHTUI_API void lightui_state_set_merge_mode(LightUIHandle handle,
                                              bool enable);
LIGHTUI_API int lightui_process_queue(LightUIHandle handle);
LIGHTUI_API int lightui_queue_size(LightUIHandle handle);

// ========== 工具函数 ==========

LIGHTUI_API void lightui_free(void* ptr);
LIGHTUI_API const char* lightui_last_error(void);

#ifdef __cplusplus
}
#endif
