/**
 * @file js_event.h
 * @brief Event 类的 JavaScript 绑定
 * 
 * 功能：
 * - 将 C++ Event 对象包装为 JavaScript 对象
 * - 实现 DOM Events 接口
 * - 支持事件属性访问
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/event.h"

namespace lightui {
namespace bindings {

/**
 * @brief 初始化 Event 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitEventBinding(JSContext* ctx);

/**
 * @brief 将 C++ Event 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param event C++ Event 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapEvent(JSContext* ctx, std::shared_ptr<Event> event);

/**
 * @brief 从 JSValue 提取 C++ Event 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ Event 对象，如果提取失败返回nullptr
 */
std::shared_ptr<Event> UnwrapEvent(JSContext* ctx, JSValue value);

} // namespace bindings
} // namespace lightui
