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
#include "core/dom/node.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/document.h"
#include "core/dom/event.h"
#include "core/dom/utils/dom_token_list.h"
#include "core/dom/style/css_style_declaration.h"
#include "core/dom/utils/dom_string_map.h"
#include "core/dom/elements/html_input_element.h"
#include "core/dom/elements/html_textarea_element.h"
#include "core/dom/elements/html_image_element.h"
#include "core/event/loop/task_scheduler.h"
#include <memory>
#include <unordered_map>

namespace mbink {

// 前向声明
class EventLoop;

/**
 * @brief Legacy DOM 绑定类
 * @note 当前仅保留为兼容/清理过渡层。
 *       新增 DOM API 不应继续添加到此类，主线绑定统一放在 core/quickjs/*。
 */
class DOMBindings {
public:
    /**
     * @brief 清理 DOM 绑定
     * @param ctx QuickJS 上下文；传 nullptr 时仅重置 legacy 调度器/EventLoop 状态
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
    // 初始化各个类
    static void InitElementClass(JSContext* ctx);
    static void InitTextClass(JSContext* ctx);
    static void InitDocumentClass(JSContext* ctx);
    static void InitEventClass(JSContext* ctx);
    static void InitDOMTokenListClass(JSContext* ctx);
    static void InitCSSStyleDeclarationClass(JSContext* ctx);
    static void InitDOMStringMapClass(JSContext* ctx);

    static void ClearLegacyElementBindings(Element* element, JSContext* fallback_ctx,
                                           JSContext* entry_ctx, JSValueConst value);
    static void ClearLegacyCaches(JSContext* ctx);

    // legacy 对象缓存：Element* → (JSContext*, JSValue)
    // 用于防止同一个 C++ 对象被旧 wrapper 重复包装
    // 存储 JSContext* 以便 legacy 清理路径调用 JS_FreeValue
    static std::unordered_map<Element*, std::pair<JSContext*, JSValue>> element_cache_;
    static std::unordered_map<Text*, std::pair<JSContext*, JSValue>> text_cache_;
    static std::unordered_map<Document*, std::pair<JSContext*, JSValue>> document_cache_;
};

// 初始化 Image 构造函数（注册到全局对象）
void InitImageConstructor(JSContext* ctx);

} // namespace mbink
