/**
 * @file terminal_bindings.h
 * @brief Terminal 和 LogView JavaScript 绑定
 *
 * 功能：
 * - 将 HTMLTerminalElement 绑定到 QuickJS
 * - 将 HTMLLogViewElement 绑定到 QuickJS
 * - 提供 JavaScript 可访问的 Terminal/LogView API
 */

#ifndef LIGHTUI_DOM_BINDINGS_TERMINAL_BINDINGS_H_
#define LIGHTUI_DOM_BINDINGS_TERMINAL_BINDINGS_H_

#include "quickjs/quickjs.h"

#include <memory>

namespace lightui {
class HTMLTerminalElement;
class HTMLLogViewElement;
}

namespace lightui {

/**
 * @brief Terminal 和 LogView 绑定类
 */
class TerminalBindings {
public:
    /**
     * @brief 初始化绑定
     * @param ctx QuickJS 上下文
     */
    static void Init(JSContext* ctx);

    /**
     * @brief 清理绑定
     * @param ctx QuickJS 上下文
     */
    static void Cleanup(JSContext* ctx);

    // === Terminal 包装/解包 ===

    /**
     * @brief 将 HTMLTerminalElement 包装为 JS 对象
     */
    static JSValue WrapTerminal(JSContext* ctx, HTMLTerminalElement* terminal);

    /**
     * @brief 从 JS 对象解包 HTMLTerminalElement
     */
    static HTMLTerminalElement* UnwrapTerminal(JSContext* ctx, JSValue obj);

    // === LogView 包装/解包 ===

    /**
     * @brief 将 HTMLLogViewElement 包装为 JS 对象
     */
    static JSValue WrapLogView(JSContext* ctx, HTMLLogViewElement* logview);

    /**
     * @brief 从 JS 对象解包 HTMLLogViewElement
     */
    static HTMLLogViewElement* UnwrapLogView(JSContext* ctx, JSValue obj);

    // Class IDs
    static JSClassID terminal_class_id;
    static JSClassID logview_class_id;

private:
    static bool initialized_;

    static void InitTerminalClass(JSContext* ctx);
    static void InitLogViewClass(JSContext* ctx);
};

}  // namespace lightui

#endif  // LIGHTUI_DOM_BINDINGS_TERMINAL_BINDINGS_H_
