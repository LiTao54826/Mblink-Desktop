/**
 * @file js_node.h
 * @brief Node 类的 JavaScript 绑定
 * 
 * 功能：
 * - 将 C++ Node 对象包装为 JavaScript 对象
 * - 实现 DOM Node 接口
 * - 支持 DOM 树操作方法
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/node.h"

namespace lightui {

namespace bindings {

/**
 * @brief 初始化 Node 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitNodeBinding(JSContext* ctx);

/**
 * @brief 将 C++ Node 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param node C++ Node 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapNode(JSContext* ctx, std::shared_ptr<Node> node);

/**
 * @brief 从 JSValue 提取 C++ Node 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ Node 对象，如果提取失败返回nullptr
 */
std::shared_ptr<Node> UnwrapNode(JSContext* ctx, JSValue value);

} // namespace bindings
} // namespace lightui
