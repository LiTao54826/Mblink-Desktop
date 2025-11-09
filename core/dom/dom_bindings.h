/**
 * @file dom_bindings.h
 * @brief DOM JavaScript 绑定
 *
 * 功能：
 * - 将 C++ DOM 类绑定到 QuickJS
 * - 提供 JavaScript 可访问的 DOM API
 * - 管理 C++ 对象和 JS 对象的生命周期
 */

#pragma once

#include "quickjs/quickjs.h"
#include "node.h"
#include "element.h"
#include "text.h"
#include "document.h"
#include "event.h"
#include <memory>

namespace lightui {

/**
 * @brief DOM 绑定类
 */
class DOMBindings {
public:
    /**
     * @brief 初始化 DOM 绑定
     * @param ctx QuickJS 上下文
     */
    static void Init(JSContext* ctx);

    /**
     * @brief 清理 DOM 绑定
     * @param ctx QuickJS 上下文
     */
    static void Cleanup(JSContext* ctx);

    // ========== 对象包装 ==========

    /**
     * @brief 将 C++ Element 包装为 JS 对象
     * @param ctx QuickJS 上下文
     * @param element C++ Element 对象
     * @return JS 对象
     */
    static JSValue WrapElement(JSContext* ctx, std::shared_ptr<Element> element);

    /**
     * @brief 将 C++ Text 包装为 JS 对象
     * @param ctx QuickJS 上下文
     * @param text C++ Text 对象
     * @return JS 对象
     */
    static JSValue WrapText(JSContext* ctx, std::shared_ptr<Text> text);

    /**
     * @brief 将 C++ Document 包装为 JS 对象
     * @param ctx QuickJS 上下文
     * @param document C++ Document 对象
     * @return JS 对象
     */
    static JSValue WrapDocument(JSContext* ctx, std::shared_ptr<Document> document);

    /**
     * @brief 将 C++ Event 包装为 JS 对象
     * @param ctx QuickJS 上下文
     * @param event C++ Event 对象
     * @return JS 对象
     */
    static JSValue WrapEvent(JSContext* ctx, std::shared_ptr<Event> event);

    /**
     * @brief 将 C++ Node 包装为 JS 对象（自动识别类型）
     * @param ctx QuickJS 上下文
     * @param node C++ Node 对象
     * @return JS 对象
     */
    static JSValue WrapNode(JSContext* ctx, std::shared_ptr<Node> node);

    // ========== 对象解包 ==========

    /**
     * @brief 从 JS 对象解包 C++ Element
     * @param ctx QuickJS 上下文
     * @param obj JS 对象
     * @return C++ Element 对象
     */
    static std::shared_ptr<Element> UnwrapElement(JSContext* ctx, JSValue obj);

    /**
     * @brief 从 JS 对象解包 C++ Text
     * @param ctx QuickJS 上下文
     * @param obj JS 对象
     * @return C++ Text 对象
     */
    static std::shared_ptr<Text> UnwrapText(JSContext* ctx, JSValue obj);

    /**
     * @brief 从 JS 对象解包 C++ Document
     * @param ctx QuickJS 上下文
     * @param obj JS 对象
     * @return C++ Document 对象
     */
    static std::shared_ptr<Document> UnwrapDocument(JSContext* ctx, JSValue obj);

    /**
     * @brief 从 JS 对象解包 C++ Event
     * @param ctx QuickJS 上下文
     * @param obj JS 对象
     * @return C++ Event 对象
     */
    static std::shared_ptr<Event> UnwrapEvent(JSContext* ctx, JSValue obj);

    // Class IDs (public for finalizers)
    static JSClassID element_class_id;
    static JSClassID text_class_id;
    static JSClassID document_class_id;
    static JSClassID event_class_id;

private:
    // 初始化标志
    static bool initialized;

    // 初始化各个类
    static void InitElementClass(JSContext* ctx);
    static void InitTextClass(JSContext* ctx);
    static void InitDocumentClass(JSContext* ctx);
    static void InitEventClass(JSContext* ctx);
};

} // namespace lightui
