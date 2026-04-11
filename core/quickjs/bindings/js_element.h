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

void InitElementBinding(JSContext* ctx);

/**
 * @brief 强制清理 Element wrapper 上登记的全部 JS 监听器引用
 * @param ctx QuickJS 上下文
 * @param element_obj Element 对应的 JS 对象
 *
 * 会同时清理：
 * - addEventListener 注册到 JSElementData::listeners 的监听器
 * - on* 事件属性对应的 hidden property 引用
 */
void ClearElementListenerBindings(JSContext* ctx, JSValueConst element_obj);

/**
 * @brief 强制清理 Element wrapper 上的 on* 事件属性
 * @param ctx QuickJS 上下文
 * @param element_obj Element 对应的 JS 对象
 */
void ClearElementEventProperties(JSContext* ctx, JSValueConst element_obj);

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
