/**
 * @file dom_bindings.h
 * @brief Legacy DOM 绑定过渡层
 */

#pragma once

#include "quickjs/quickjs.h"

namespace mbink {

class DOMBindings {
public:
    /**
     * @brief 清理 legacy DOM 绑定残留状态
     * @param ctx QuickJS 上下文；传 nullptr 时仅保持 no-op 兼容语义
     */
    static void Cleanup(JSContext* ctx);
};

// 初始化 Image 构造函数（注册到全局对象）
void InitImageConstructor(JSContext* ctx);

} // namespace mbink
