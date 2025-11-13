/**
 * @file js_value_wrapper.h
 * @brief RAII 包装器，自动管理 JSValue 生命周期
 * 
 * 功能：
 * - 自动调用 JS_DupValue 增加引用计数
 * - 自动调用 JS_FreeValue 释放引用
 * - 支持移动语义，避免不必要的引用计数操作
 * - 线程安全（假设 QuickJS 在单线程中使用）
 * 
 * 使用场景：
 * - Lambda 捕获 JSValue
 * - 容器存储 JSValue
 * - 跨函数传递 JSValue
 * 
 * 示例：
 * ```cpp
 * // 错误的做法（会泄漏）
 * JSValue listener = JS_DupValue(ctx, argv[0]);
 * auto lambda = [listener]() {
 *     JS_Call(ctx, listener, ...);
 *     // ❌ listener 永远不会被释放
 * };
 * 
 * // 正确的做法（使用 JSValueWrapper）
 * auto listener_wrapper = std::make_shared<JSValueWrapper>(ctx, argv[0]);
 * auto lambda = [listener_wrapper]() {
 *     JS_Call(ctx, listener_wrapper->Get(), ...);
 *     // ✅ listener_wrapper 析构时自动释放
 * };
 * ```
 */

#pragma once

#include "quickjs.h"
#include <utility>

namespace lightui {

/**
 * @brief RAII 包装器，自动管理 JSValue 生命周期
 * 
 * 这个类确保 JSValue 的引用计数正确管理：
 * - 构造时调用 JS_DupValue（引用计数 +1）
 * - 析构时调用 JS_FreeValue（引用计数 -1）
 * - 移动时转移所有权，不改变引用计数
 * 
 * 注意：
 * - 禁止拷贝（避免意外的引用计数增加）
 * - 支持移动（高效转移所有权）
 * - 线程不安全（QuickJS 本身不是线程安全的）
 */
class JSValueWrapper {
public:
    /**
     * @brief 构造函数，增加 JSValue 引用计数
     * @param ctx QuickJS 上下文
     * @param value JSValue（会被 DupValue）
     */
    JSValueWrapper(JSContext* ctx, JSValue value)
        : ctx_(ctx)
        , value_(JS_DupValue(ctx, value))
        , owns_value_(true) {
    }

    /**
     * @brief 析构函数，释放 JSValue 引用
     */
    ~JSValueWrapper() {
        if (owns_value_ && ctx_) {
            JS_FreeValue(ctx_, value_);
        }
    }

    // ========== 禁止拷贝 ==========
    
    /**
     * @brief 禁止拷贝构造
     * 
     * 原因：拷贝会导致引用计数增加，可能不是用户期望的行为。
     * 如果需要共享，请使用 std::shared_ptr<JSValueWrapper>。
     */
    JSValueWrapper(const JSValueWrapper&) = delete;
    
    /**
     * @brief 禁止拷贝赋值
     */
    JSValueWrapper& operator=(const JSValueWrapper&) = delete;

    // ========== 支持移动 ==========
    
    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     * 
     * 转移所有权，不改变引用计数。
     */
    JSValueWrapper(JSValueWrapper&& other) noexcept
        : ctx_(other.ctx_)
        , value_(other.value_)
        , owns_value_(other.owns_value_) {
        other.owns_value_ = false;  // 转移所有权
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     */
    JSValueWrapper& operator=(JSValueWrapper&& other) noexcept {
        if (this != &other) {
            // 释放当前持有的值
            if (owns_value_ && ctx_) {
                JS_FreeValue(ctx_, value_);
            }

            // 转移所有权
            ctx_ = other.ctx_;
            value_ = other.value_;
            owns_value_ = other.owns_value_;
            other.owns_value_ = false;
        }
        return *this;
    }

    // ========== 访问器 ==========
    
    /**
     * @brief 获取 JSValue
     * @return JSValue（不增加引用计数）
     * 
     * 注意：返回的 JSValue 的生命周期由 JSValueWrapper 管理，
     * 不要在 JSValueWrapper 析构后使用返回的值。
     */
    JSValue Get() const {
        return value_;
    }

    /**
     * @brief 获取 QuickJS 上下文
     * @return JSContext 指针
     */
    JSContext* GetContext() const {
        return ctx_;
    }

    /**
     * @brief 检查是否持有有效值
     * @return true 表示持有有效值
     */
    bool IsValid() const {
        return owns_value_ && ctx_ != nullptr;
    }

    /**
     * @brief 检查 JSValue 是否为 undefined
     * @return true 表示为 undefined
     */
    bool IsUndefined() const {
        return JS_IsUndefined(value_);
    }

    /**
     * @brief 检查 JSValue 是否为 null
     * @return true 表示为 null
     */
    bool IsNull() const {
        return JS_IsNull(value_);
    }

    /**
     * @brief 检查 JSValue 是否为函数
     * @return true 表示为函数
     */
    bool IsFunction() const {
        return ctx_ && JS_IsFunction(ctx_, value_);
    }

    /**
     * @brief 检查 JSValue 是否为对象
     * @return true 表示为对象
     */
    bool IsObject() const {
        return JS_IsObject(value_);
    }

    // ========== 调用支持 ==========
    
    /**
     * @brief 调用 JSValue（如果是函数）
     * @param this_val this 值
     * @param argc 参数数量
     * @param argv 参数数组
     * @return 调用结果（需要手动释放）
     * 
     * 注意：返回的 JSValue 需要调用者负责释放。
     */
    JSValue Call(JSValue this_val, int argc, JSValueConst* argv) const {
        if (!ctx_ || !IsFunction()) {
            return JS_UNDEFINED;
        }
        return JS_Call(ctx_, value_, this_val, argc, argv);
    }

    /**
     * @brief 调用 JSValue（无参数版本）
     * @return 调用结果（需要手动释放）
     */
    JSValue Call() const {
        return Call(JS_UNDEFINED, 0, nullptr);
    }

private:
    JSContext* ctx_;      ///< QuickJS 上下文
    JSValue value_;       ///< 持有的 JSValue
    bool owns_value_;     ///< 是否拥有值的所有权
};

/**
 * @brief 创建 shared_ptr<JSValueWrapper> 的辅助函数
 * @param ctx QuickJS 上下文
 * @param value JSValue
 * @return shared_ptr<JSValueWrapper>
 * 
 * 使用示例：
 * ```cpp
 * auto wrapper = MakeJSValueWrapper(ctx, argv[0]);
 * auto lambda = [wrapper]() {
 *     wrapper->Call();
 * };
 * ```
 */
inline std::shared_ptr<JSValueWrapper> MakeJSValueWrapper(JSContext* ctx, JSValue value) {
    return std::make_shared<JSValueWrapper>(ctx, value);
}

} // namespace lightui

