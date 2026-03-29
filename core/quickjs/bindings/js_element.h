/**
 * @file js_element.h
 * @brief Element 类的 JavaScript 绑定
 * 
 * 功能：
 * - 将 C++ Element 对象包装为 JavaScript 对象
 * - 继承自 JSNode
 * - 实现 DOM Element 接口
 * - 支持属性和样式操作
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/element.h"

namespace mbink {
namespace bindings {

/**
 * @brief 初始化 Element 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitElementBinding(JSContext* ctx);
void DumpElementListenerStats();

/**
 * @brief 将 C++ Element 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param element C++ Element 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapElement(JSContext* ctx, std::shared_ptr<Element> element);

/**
 * @brief 从 JSValue 提取 C++ Element 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ Element 对象，如果提取失败返回nullptr
 */
std::shared_ptr<Element> UnwrapElement(JSContext* ctx, JSValue value);

/**
 * @brief 获取 Element 类的 JSClassID
 * @return JSClassID
 */
JSClassID GetElementClassID();

} // namespace bindings
} // namespace mbink
