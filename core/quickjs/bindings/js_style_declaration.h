/**
 * @file js_style_declaration.h
 * @brief CSSStyleDeclaration 类的 JavaScript 绑定
 * 
 * 功能：
 * - 将 C++ CSSStyleDeclaration 对象包装为 JavaScript 对象
 * - 实现 CSSOM CSSStyleDeclaration 接口
 * - 支持 style 属性访问
 */

#pragma once

#include <memory>
#include "quickjs.h"
#include "core/dom/css_style_declaration.h"

namespace lightui {
namespace bindings {

/**
 * @brief 初始化 CSSStyleDeclaration 类的 JavaScript 绑定
 * @param ctx QuickJS 上下文
 */
void InitStyleDeclarationBinding(JSContext* ctx);

/**
 * @brief 将 C++ CSSStyleDeclaration 包装为 JSValue
 * @param ctx QuickJS 上下文
 * @param style C++ CSSStyleDeclaration 对象
 * @return JSValue（JS 对象）
 */
JSValue WrapStyleDeclaration(JSContext* ctx, std::shared_ptr<CSSStyleDeclaration> style);

/**
 * @brief 从 JSValue 提取 C++ CSSStyleDeclaration 对象
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return C++ CSSStyleDeclaration 对象，如果提取失败返回nullptr
 */
std::shared_ptr<CSSStyleDeclaration> UnwrapStyleDeclaration(JSContext* ctx, JSValue value);

} // namespace bindings
} // namespace lightui
