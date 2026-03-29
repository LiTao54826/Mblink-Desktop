/**
 * @file js_range.h
 * @brief Range 类的 JavaScript 绑定
 *
 * 功能：
 * - 将 C++ Range 对象包装为 JavaScript 对象
 * - 实现 DOM Range 接口
 * - 支持文档片段选择和操作
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/selection/range.h"

namespace mbink {

namespace bindings {

/**
 * @brief 获取 Range 的 ClassID
 * @return Range 的 JSClassID
 */
JSClassID GetRangeClassID();

/**
 * @brief 初始化 Range 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitRangeBinding(JSContext* ctx);

/**
 * @brief 将 C++ Range 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param range C++ Range 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapRange(JSContext* ctx, std::shared_ptr<Range> range);

/**
 * @brief 从 JSValue 提取 C++ Range 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ Range 对象，如果提取失败返回 nullptr
 */
std::shared_ptr<Range> UnwrapRange(JSContext* ctx, JSValue value);

} // namespace bindings
} // namespace mbink
