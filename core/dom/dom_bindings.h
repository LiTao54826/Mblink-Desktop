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
#include "dom_token_list.h"
#include "css_style_declaration.h"
#include "dom_string_map.h"
#include "html_input_element.h"
#include "html_textarea_element.h"
#include "core/event/task_scheduler.h"
#include <memory>
#include <unordered_map>

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
     * @brief 设置全局 document 对象
     * @param ctx QuickJS 上下文
     * @param document Document 对象
     */
    static void SetGlobalDocument(JSContext* ctx, std::shared_ptr<Document> document);

    /**
     * @brief 设置全局 TaskScheduler
     * @param ctx QuickJS 上下文
     * @param scheduler TaskScheduler 对象
     */
    static void SetGlobalTaskScheduler(JSContext* ctx, std::shared_ptr<TaskScheduler> scheduler);

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
    static JSClassID dom_token_list_class_id;
    static JSClassID css_style_declaration_class_id;
    static JSClassID dom_string_map_class_id;

    // 缓存管理函数 (public for finalizers)
    static void RemoveFromElementCache(Element* ptr);
    static void RemoveFromTextCache(Text* ptr);
    static void RemoveFromDocumentCache(Document* ptr);

private:
    // 初始化标志
    static bool initialized;

    // 初始化各个类
    static void InitElementClass(JSContext* ctx);
    static void InitTextClass(JSContext* ctx);
    static void InitDocumentClass(JSContext* ctx);
    static void InitEventClass(JSContext* ctx);
    static void InitDOMTokenListClass(JSContext* ctx);
    static void InitCSSStyleDeclarationClass(JSContext* ctx);
    static void InitDOMStringMapClass(JSContext* ctx);

    // 对象缓存：Element* → (JSContext*, JSValue)
    // 用于防止同一个C++对象被包装多次
    // 存储JSContext*以便在清理时调用JS_FreeValue
    static std::unordered_map<Element*, std::pair<JSContext*, JSValue>> element_cache_;
    static std::unordered_map<Text*, std::pair<JSContext*, JSValue>> text_cache_;
    static std::unordered_map<Document*, std::pair<JSContext*, JSValue>> document_cache_;
};

} // namespace lightui
