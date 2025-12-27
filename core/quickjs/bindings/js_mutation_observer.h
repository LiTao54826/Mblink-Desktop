/**
 * @file js_mutation_observer.h
 * @brief MutationObserver 类的 JavaScript 绑定
 *
 * 功能：
 * - 将 C++ MutationObserver 包装为 JavaScript 对象
 * - 实现 DOM MutationObserver 接口
 * - 支持 DOM 变化监听
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/mutation_observer.h"

namespace lightui {

namespace bindings {

/**
 * @brief 获取 MutationObserver 的 ClassID
 * @return MutationObserver 的 JSClassID
 */
JSClassID GetMutationObserverClassID();

/**
 * @brief 获取 MutationRecord 的 ClassID
 * @return MutationRecord 的 JSClassID
 */
JSClassID GetMutationRecordClassID();

/**
 * @brief 初始化 MutationObserver 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitMutationObserverBinding(JSContext* ctx);

/**
 * @brief 将 C++ MutationObserver 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param observer C++ MutationObserver 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapMutationObserver(JSContext* ctx, std::shared_ptr<MutationObserver> observer);

/**
 * @brief 从 JSValue 提取 C++ MutationObserver 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ MutationObserver 对象，如果提取失败返回 nullptr
 */
std::shared_ptr<MutationObserver> UnwrapMutationObserver(JSContext* ctx, JSValue value);

/**
 * @brief 将 MutationRecord 转换为 JSValue
 * @param ctx QuickJS 上下文
 * @param record MutationRecord
 * @return JSValue（JS 对象）
 */
JSValue MutationRecordToJS(JSContext* ctx, const MutationRecord& record);

} // namespace bindings
} // namespace lightui
