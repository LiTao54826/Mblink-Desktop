/**
 * @file lightui.h
 * @brief LightUI C API
 * 
 * 功能：
 * - 提供稳定的C语言API
 * - 用于各种语言的FFI绑定
 * - 窗口创建和管理
 * - JavaScript代码执行
 * - 函数绑定
 * 
 * 实现要点：
 * - 使用不透明指针隐藏实现细节
 * - 所有函数返回错误码
 * - 线程安全
 * - 内存管理清晰
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ========== 类型定义 ==========

/**
 * @brief 窗口句柄（不透明指针）
 */
typedef struct LightUIWindow* LightUIWindowHandle;

/**
 * @brief 错误码
 */
typedef enum {
    LIGHTUI_OK = 0,
    LIGHTUI_ERROR_INIT_FAILED = -1,
    LIGHTUI_ERROR_INVALID_HANDLE = -2,
    LIGHTUI_ERROR_INVALID_PARAMETER = -3,
    LIGHTUI_ERROR_JS_EXECUTION_FAILED = -4,
    LIGHTUI_ERROR_FUNCTION_NOT_FOUND = -5,
    LIGHTUI_ERROR_OUT_OF_MEMORY = -6,
    LIGHTUI_ERROR_UNKNOWN = -99
} LightUIError;

/**
 * @brief 回调函数类型
 * @param args 参数（JSON字符串）
 * @param user_data 用户数据
 * @return 返回值（JSON字符串），调用者负责释放
 */
typedef char* (*LightUICallback)(const char* args, void* user_data);

// ========== 初始化和清理 ==========

/**
 * @brief 初始化LightUI
 * @return 错误码
 * 
 * TODO:
 * - [ ] 初始化SDL
 * - [ ] 初始化Skia
 * - [ ] 设置日志系统
 */
int lightui_init(void);

/**
 * @brief 清理LightUI
 * 
 * TODO:
 * - [ ] 清理所有窗口
 * - [ ] 清理SDL
 * - [ ] 清理Skia
 */
void lightui_cleanup(void);

/**
 * @brief 获取版本字符串
 * @return 版本字符串（如"0.1.0"）
 */
const char* lightui_get_version(void);

// ========== 窗口管理 ==========

/**
 * @brief 创建窗口
 * @param title 窗口标题
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 窗口句柄，失败返回NULL
 * 
 * TODO:
 * - [ ] 创建Window对象
 * - [ ] 初始化QuickJS运行时
 * - [ ] 初始化DOM
 * - [ ] 返回句柄
 */
LightUIWindowHandle lightui_create_window(const char* title, int width, int height);

/**
 * @brief 销毁窗口
 * @param window 窗口句柄
 * 
 * TODO:
 * - [ ] 清理QuickJS运行时
 * - [ ] 清理DOM
 * - [ ] 销毁Window对象
 */
void lightui_destroy_window(LightUIWindowHandle window);

/**
 * @brief 显示窗口
 * @param window 窗口句柄
 * @return 错误码
 */
int lightui_show_window(LightUIWindowHandle window);

/**
 * @brief 隐藏窗口
 * @param window 窗口句柄
 * @return 错误码
 */
int lightui_hide_window(LightUIWindowHandle window);

/**
 * @brief 设置窗口标题
 * @param window 窗口句柄
 * @param title 新标题
 * @return 错误码
 */
int lightui_set_window_title(LightUIWindowHandle window, const char* title);

/**
 * @brief 设置窗口大小
 * @param window 窗口句柄
 * @param width 宽度
 * @param height 高度
 * @return 错误码
 */
int lightui_set_window_size(LightUIWindowHandle window, int width, int height);

// ========== JavaScript执行 ==========

/**
 * @brief 加载并执行JavaScript代码
 * @param window 窗口句柄
 * @param js_code JavaScript代码
 * @return 错误码
 * 
 * TODO:
 * - [ ] 执行JavaScript代码
 * - [ ] 处理错误
 * - [ ] 触发首次渲染
 */
int lightui_load_ui(LightUIWindowHandle window, const char* js_code);

/**
 * @brief 加载并执行JavaScript文件
 * @param window 窗口句柄
 * @param filepath 文件路径
 * @return 错误码
 */
int lightui_load_ui_file(LightUIWindowHandle window, const char* filepath);

/**
 * @brief 执行JavaScript代码
 * @param window 窗口句柄
 * @param js_code JavaScript代码
 * @param result 执行结果（JSON字符串），调用者负责释放
 * @return 错误码
 */
int lightui_eval(LightUIWindowHandle window, const char* js_code, char** result);

// ========== 函数绑定 ==========

/**
 * @brief 绑定C函数到JavaScript
 * @param window 窗口句柄
 * @param name 函数名（在JavaScript中的名称）
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 错误码
 * 
 * TODO:
 * - [ ] 注册回调函数
 * - [ ] 在JavaScript中创建全局函数
 * - [ ] 处理参数和返回值转换
 */
int lightui_bind_function(LightUIWindowHandle window, const char* name,
                         LightUICallback callback, void* user_data);

// ========== 事件循环 ==========

/**
 * @brief 运行事件循环（阻塞）
 * @param window 窗口句柄
 * 
 * TODO:
 * - [ ] 进入主循环
 * - [ ] 处理SDL事件
 * - [ ] 触发布局和渲染
 * - [ ] 处理JavaScript事件
 */
void lightui_run(LightUIWindowHandle window);

/**
 * @brief 处理一次事件（非阻塞）
 * @param window 窗口句柄
 * @return true表示应该继续，false表示应该退出
 */
bool lightui_poll_events(LightUIWindowHandle window);

/**
 * @brief 停止事件循环
 * @param window 窗口句柄
 */
void lightui_stop(LightUIWindowHandle window);

// ========== 错误处理 ==========

/**
 * @brief 获取最后一次错误信息
 * @return 错误信息字符串
 */
const char* lightui_get_last_error(void);

/**
 * @brief 释放字符串内存
 * @param str 要释放的字符串
 */
void lightui_free_string(char* str);

#ifdef __cplusplus
}
#endif

