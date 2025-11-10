/**
 * @file window_bindings.h
 * @brief Window 对象的 JavaScript 绑定
 * 
 * 功能：
 * - 将 Window 对象暴露给 JavaScript
 * - 绑定 window 全局对象
 * - 绑定 document 对象
 * - 绑定定时器函数 (setTimeout, setInterval, requestAnimationFrame)
 */

#pragma once

#include "quickjs_runtime.h"
#include "core/window/window.h"
#include "core/dom/document.h"
#include "core/event/task_scheduler.h"
#include <memory>

namespace lightui {

/**
 * @brief Window 对象的 JavaScript 绑定
 */
class WindowBindings {
public:
    /**
     * @brief 构造函数
     * @param runtime QuickJS 运行时
     * @param window 窗口对象
     * @param task_scheduler 任务调度器
     */
    WindowBindings(QuickJSRuntime* runtime, 
                   std::shared_ptr<Window> window,
                   std::shared_ptr<TaskScheduler> task_scheduler);
    
    /**
     * @brief 析构函数
     */
    ~WindowBindings() = default;
    
    /**
     * @brief 初始化所有绑定
     */
    void InitBindings();
    
    /**
     * @brief 绑定 window 全局对象
     */
    void BindWindowObject();
    
    /**
     * @brief 绑定 document 对象
     */
    void BindDocumentObject();
    
    /**
     * @brief 绑定定时器函数
     */
    void BindTimers();
    
    /**
     * @brief 绑定事件监听器
     */
    void BindEventListeners();

private:
    QuickJSRuntime* runtime_;
    std::shared_ptr<Window> window_;
    std::shared_ptr<TaskScheduler> task_scheduler_;
};

/**
 * @brief Document 对象的 JavaScript 绑定
 */
class DocumentBindings {
public:
    /**
     * @brief 构造函数
     * @param runtime QuickJS 运行时
     * @param document 文档对象
     */
    DocumentBindings(QuickJSRuntime* runtime, std::shared_ptr<Document> document);
    
    /**
     * @brief 析构函数
     */
    ~DocumentBindings() = default;
    
    /**
     * @brief 初始化所有绑定
     */
    void InitBindings();
    
    /**
     * @brief 绑定 DOM 查询方法
     */
    void BindQueryMethods();
    
    /**
     * @brief 绑定 DOM 创建方法
     */
    void BindCreateMethods();
    
    /**
     * @brief 绑定 DOM 属性
     */
    void BindProperties();

private:
    QuickJSRuntime* runtime_;
    std::shared_ptr<Document> document_;
};

} // namespace lightui

