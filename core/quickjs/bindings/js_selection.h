/**
 * @file js_selection.h
 * @brief Selection 类的 JavaScript 绑定
 *
 * 功能：
 * - 将 C++ Selection 对象包装为 JavaScript 对象
 * - 实现 DOM Selection 接口
 * - 支持文本选择操作
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/selection.h"

namespace lightui {

namespace bindings {

/**
 * @brief 获取 Selection 的 ClassID
 * @return Selection 的 JSClassID
 */
JSClassID GetSelectionClassID();

/**
 * @brief 初始化 Selection 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitSelectionBinding(JSContext* ctx);

/**
 * @brief 将 C++ Selection 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param selection C++ Selection 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapSelection(JSContext* ctx, std::shared_ptr<Selection> selection);

/**
 * @brief 从 JSValue 提取 C++ Selection 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ Selection 对象，如果提取失败返回 nullptr
 */
std::shared_ptr<Selection> UnwrapSelection(JSContext* ctx, JSValue value);

} // namespace bindings
} // namespace lightui
