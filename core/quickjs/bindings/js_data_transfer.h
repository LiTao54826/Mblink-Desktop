/**
 * @file js_data_transfer.h
 * @brief DataTransfer 类的 JavaScript 绑定
 *
 * 功能：
 * - 将 C++ DataTransfer 对象包装为 JavaScript 对象
 * - 实现 W3C DataTransfer 接口
 * - 支持 setData/getData/clearData 方法
 * - 支持 effectAllowed/dropEffect 属性
 *
 * 参考：
 * - W3C HTML5 - DataTransfer
 * - MDN Web Docs - DataTransfer
 */

#pragma once

#include <memory>
#include "quickjs.h"

namespace lightui {

// 前向声明
class DataTransfer;

namespace bindings {

/**
 * @brief 初始化 DataTransfer 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitDataTransferBinding(JSContext* ctx);

/**
 * @brief 将 C++ DataTransfer 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param data_transfer C++ DataTransfer 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapDataTransfer(JSContext* ctx, std::shared_ptr<DataTransfer> data_transfer);

/**
 * @brief 从 JSValue 提取 C++ DataTransfer 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ DataTransfer 对象，如果提取失败返回 nullptr
 */
std::shared_ptr<DataTransfer> UnwrapDataTransfer(JSContext* ctx, JSValue value);

/**
 * @brief 获取 DataTransfer 的 ClassID
 * @return ClassID
 */
JSClassID GetDataTransferClassID();

} // namespace bindings
} // namespace lightui
