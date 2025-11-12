/**
 * @file preact_bindings.h
 * @brief Preact JavaScript绑定
 *
 * 功能：
 * - 将PreactRenderer绑定到QuickJS
 * - 提供JavaScript可访问的Preact API
 * - 连接Preact Virtual DOM和MBink DOM
 */

#pragma once

#include "quickjs/quickjs.h"
#include "preact_renderer.h"
#include <memory>

namespace lightui {

/**
 * @brief Preact绑定类
 */
class PreactBindings {
public:
    /**
     * @brief 初始化Preact绑定
     * @param ctx QuickJS上下文
     * @param renderer PreactRenderer实例
     */
    static void Init(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer);

    /**
     * @brief 清理Preact绑定
     * @param ctx QuickJS上下文
     */
    static void Cleanup(JSContext* ctx);

    /**
     * @brief 注册Preact模块
     * @param ctx QuickJS上下文
     * @param renderer PreactRenderer实例
     */
    static void RegisterPreactModule(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer);

private:
    // ========== JavaScript函数绑定 ==========

    /**
     * @brief render(vnode, container) - 渲染VNode到容器
     */
    static JSValue js_render(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

    /**
     * @brief createDOMElement(vnode) - 从VNode创建DOM元素
     */
    static JSValue js_createDOMElement(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

    /**
     * @brief diff(oldVNode, newVNode, container) - 对比并更新VNode
     */
    static JSValue js_diff(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);

    // ========== 辅助函数 ==========

    /**
     * @brief 获取PreactRenderer实例
     */
    static std::shared_ptr<PreactRenderer> GetRenderer(JSContext* ctx);

    /**
     * @brief 设置PreactRenderer实例
     */
    static void SetRenderer(JSContext* ctx, std::shared_ptr<PreactRenderer> renderer);
};

} // namespace lightui

