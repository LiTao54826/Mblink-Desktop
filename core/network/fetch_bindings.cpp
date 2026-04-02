/**
 * @file fetch_bindings.cpp
 * @brief fetch API 绑定实现
 */

#include "fetch_bindings.h"
#include <iostream>
#include <cstring>

namespace mbink {

// 静态实例指针
FetchBindings* FetchBindings::instance_ = nullptr;

FetchBindings::FetchBindings(JSContext* ctx,
                             std::shared_ptr<TaskScheduler> task_scheduler)
    : ctx_(ctx)
    , task_scheduler_(task_scheduler)
    , http_client_(std::make_unique<HttpClient>())
    , next_request_id_(1) {
    instance_ = this;
}

FetchBindings::~FetchBindings() {
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

void FetchBindings::InitBindings() {
    RegisterNativeFunctions();
    RegisterJSPolyfill();
}

// 静态回调函数：__fetch_request
static JSValue js_fetch_request(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv) {
    if (!FetchBindings::instance_) {
        return JS_ThrowInternalError(ctx, "FetchBindings not initialized");
    }

    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "fetch requires at least 1 argument");
    }

    // 获取 URL
    const char* url_str = JS_ToCString(ctx, argv[0]);
    if (!url_str) {
        return JS_EXCEPTION;
    }
    std::string url(url_str);
    JS_FreeCString(ctx, url_str);

    // 获取选项
    nlohmann::json options = nlohmann::json::object();
    if (argc > 1 && !JS_IsUndefined(argv[1])) {
        options = FetchBindings::instance_->JSValueToJson(argv[1]);
    }

    int request_id = FetchBindings::instance_->DoFetch(url, options);
    return JS_NewInt32(ctx, request_id);
}

// 静态回调函数：__fetch_check_response
static JSValue js_fetch_check_response(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    if (!FetchBindings::instance_) {
        return JS_NULL;
    }

    std::lock_guard<std::mutex> lock(FetchBindings::instance_->pending_mutex_);

    if (FetchBindings::instance_->pending_responses_.empty()) {
        return JS_NULL;
    }

    auto& response = FetchBindings::instance_->pending_responses_.front();
    nlohmann::json result = nlohmann::json::object();
    result["id"] = response.request_id;
    result["response"] = FetchBindings::instance_->ResponseToJson(response.response);
    FetchBindings::instance_->pending_responses_.pop();

    return FetchBindings::instance_->JsonToJSValue(result);
}

void FetchBindings::RegisterNativeFunctions() {
    JSValue global = JS_GetGlobalObject(ctx_);

    // 注册 __fetch_request
    JSValue fetch_func = JS_NewCFunction(ctx_, js_fetch_request, "__fetch_request", 2);
    JS_SetPropertyStr(ctx_, global, "__fetch_request", fetch_func);

    // 注册 __fetch_check_response
    JSValue check_func = JS_NewCFunction(ctx_, js_fetch_check_response, "__fetch_check_response", 0);
    JS_SetPropertyStr(ctx_, global, "__fetch_check_response", check_func);

    JS_FreeValue(ctx_, global);
}

void FetchBindings::RegisterJSPolyfill() {
    // fetch API polyfill
    std::string fetch_code = R"(
(function(global) {
    'use strict';
    
    // 存储 pending promises
    const pendingFetches = new Map();
    let fetchPolling = false;
    let fetchCleanupRequested = false;

    function schedulePendingResponseCheck() {
        if (fetchCleanupRequested || fetchPolling || pendingFetches.size === 0) {
            return;
        }
        fetchPolling = true;
        setTimeout(checkPendingResponses, 10);
    }

    // Response 类
    class Response {
        constructor(data) {
            this._data = data;
            this.ok = data.ok;
            this.status = data.status;
            this.statusText = data.statusText || '';
            this.headers = new Headers(data.headers || {});
            this._body = data.body || '';
            this._bodyUsed = false;
        }
        
        get bodyUsed() {
            return this._bodyUsed;
        }
        
        async text() {
            if (this._bodyUsed) {
                throw new TypeError('Body has already been consumed');
            }
            this._bodyUsed = true;
            return this._body;
        }
        
        async json() {
            const text = await this.text();
            return JSON.parse(text);
        }
        
        async blob() {
            const text = await this.text();
            return new Blob([text]);
        }
        
        async arrayBuffer() {
            const text = await this.text();
            const encoder = new TextEncoder();
            return encoder.encode(text).buffer;
        }
        
        clone() {
            if (this._bodyUsed) {
                throw new TypeError('Body has already been consumed');
            }
            return new Response({...this._data});
        }
    }
    
    // Headers 类
    class Headers {
        constructor(init) {
            this._headers = {};
            if (init) {
                if (typeof init === 'object') {
                    for (const [key, value] of Object.entries(init)) {
                        this._headers[key.toLowerCase()] = String(value);
                    }
                }
            }
        }
        
        get(name) {
            return this._headers[name.toLowerCase()] || null;
        }
        
        set(name, value) {
            this._headers[name.toLowerCase()] = String(value);
        }
        
        has(name) {
            return name.toLowerCase() in this._headers;
        }
        
        delete(name) {
            delete this._headers[name.toLowerCase()];
        }
        
        forEach(callback) {
            for (const [key, value] of Object.entries(this._headers)) {
                callback(value, key, this);
            }
        }
        
        entries() {
            return Object.entries(this._headers)[Symbol.iterator]();
        }
        
        keys() {
            return Object.keys(this._headers)[Symbol.iterator]();
        }
        
        values() {
            return Object.values(this._headers)[Symbol.iterator]();
        }
    }
    
    // fetch 函数
    function fetch(url, options) {
        options = options || {};

        return new Promise((resolve, reject) => {
            if (fetchCleanupRequested) {
                reject(new Error('Fetch subsystem is cleaning up'));
                return;
            }

            // 准备请求选项
            const fetchOptions = {
                method: options.method || 'GET',
                headers: options.headers || {},
                body: options.body || ''
            };

            // 发起请求
            const requestId = __fetch_request(url, fetchOptions);

            if (typeof requestId !== 'number' || requestId < 0) {
                reject(new Error('Failed to initiate fetch request'));
                return;
            }

            // 存储 promise 的 resolve/reject
            pendingFetches.set(requestId, { resolve, reject });
            schedulePendingResponseCheck();
        });
    }

    // 轮询检查响应的函数
    function checkPendingResponses() {
        fetchPolling = false;

        if (fetchCleanupRequested) {
            return;
        }

        const result = __fetch_check_response();

        if (result && result.id) {
            const pending = pendingFetches.get(result.id);
            if (pending) {
                pendingFetches.delete(result.id);

                if (result.response.error) {
                    pending.reject(new Error(result.response.error));
                } else {
                    pending.resolve(new Response(result.response));
                }
            }
        }

        // 如果还有 pending 请求，继续轮询
        if (pendingFetches.size > 0) {
            schedulePendingResponseCheck();
        }
    }

    // 包装 fetch 以启动轮询
    const originalFetch = fetch;
    global.fetch = function(url, options) {
        return originalFetch(url, options);
    };

    global.__fetchCleanup = function() {
        fetchCleanupRequested = true;
        fetchPolling = false;
        pendingFetches.forEach(({ reject }) => {
            if (typeof reject === 'function') {
                reject(new Error('Fetch subsystem cleaned up'));
            }
        });
        pendingFetches.clear();
    };

    // 导出到全局
    global.Response = Response;
    global.Headers = Headers;

})(globalThis);
)";

    EvalJS(fetch_code, "<fetch_polyfill>");
}

int FetchBindings::DoFetch(const std::string& url, const nlohmann::json& options) {
    int request_id = next_request_id_++;
    
    // 准备请求选项
    HttpRequestOptions http_options;
    
    if (options.contains("method")) {
        http_options.method = options["method"].get<std::string>();
    }
    
    if (options.contains("headers") && options["headers"].is_object()) {
        for (auto& [key, value] : options["headers"].items()) {
            http_options.headers[key] = value.get<std::string>();
        }
    }
    
    if (options.contains("body")) {
        http_options.body = options["body"].get<std::string>();
    }
    
    // 异步执行请求
    http_client_->RequestAsync(url, http_options, 
        [this, request_id](const HttpResponse& response) {
            // 将响应添加到待处理队列
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_responses_.push({request_id, response});
        });
    
    return request_id;
}

nlohmann::json FetchBindings::ResponseToJson(const HttpResponse& response) {
    nlohmann::json result;
    result["ok"] = response.ok;
    result["status"] = response.status_code;
    result["statusText"] = response.status_text;
    result["headers"] = response.headers;
    result["body"] = response.body;
    
    if (!response.error.empty()) {
        result["error"] = response.error;
    }
    
    return result;
}

void FetchBindings::ProcessPendingResponses() {
    // 这个方法不再需要，因为 JS 端会通过轮询来检查响应
}

bool FetchBindings::HasPendingResponses() const {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    return !pending_responses_.empty();
}

JSValue FetchBindings::JsonToJSValue(const nlohmann::json& j) {
    if (j.is_null()) {
        return JS_NULL;
    } else if (j.is_boolean()) {
        return JS_NewBool(ctx_, j.get<bool>());
    } else if (j.is_number_integer()) {
        return JS_NewInt64(ctx_, j.get<int64_t>());
    } else if (j.is_number_float()) {
        return JS_NewFloat64(ctx_, j.get<double>());
    } else if (j.is_string()) {
        return JS_NewString(ctx_, j.get<std::string>().c_str());
    } else if (j.is_array()) {
        JSValue arr = JS_NewArray(ctx_);
        uint32_t i = 0;
        for (const auto& elem : j) {
            JS_SetPropertyUint32(ctx_, arr, i++, JsonToJSValue(elem));
        }
        return arr;
    } else if (j.is_object()) {
        JSValue obj = JS_NewObject(ctx_);
        for (auto& [key, value] : j.items()) {
            JS_SetPropertyStr(ctx_, obj, key.c_str(), JsonToJSValue(value));
        }
        return obj;
    }
    return JS_UNDEFINED;
}

nlohmann::json FetchBindings::JSValueToJson(JSValue val) {
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return nullptr;
    } else if (JS_IsBool(val)) {
        return JS_ToBool(ctx_, val) != 0;
    } else if (JS_IsNumber(val)) {
        double d;
        JS_ToFloat64(ctx_, &d, val);
        return d;
    } else if (JS_IsString(val)) {
        const char* str = JS_ToCString(ctx_, val);
        nlohmann::json result = str ? std::string(str) : "";
        JS_FreeCString(ctx_, str);
        return result;
    } else if (JS_IsArray(val)) {
        nlohmann::json arr = nlohmann::json::array();
        JSValue length_val = JS_GetPropertyStr(ctx_, val, "length");
        uint32_t length = 0;
        JS_ToUint32(ctx_, &length, length_val);
        JS_FreeValue(ctx_, length_val);

        for (uint32_t i = 0; i < length; i++) {
            JSValue elem = JS_GetPropertyUint32(ctx_, val, i);
            arr.push_back(JSValueToJson(elem));
            JS_FreeValue(ctx_, elem);
        }
        return arr;
    } else if (JS_IsObject(val)) {
        nlohmann::json obj = nlohmann::json::object();

        JSPropertyEnum* props = nullptr;
        uint32_t prop_count = 0;
        if (JS_GetOwnPropertyNames(ctx_, &props, &prop_count, val, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < prop_count; i++) {
                const char* key = JS_AtomToCString(ctx_, props[i].atom);
                if (key) {
                    JSValue prop_val = JS_GetProperty(ctx_, val, props[i].atom);
                    obj[key] = JSValueToJson(prop_val);
                    JS_FreeValue(ctx_, prop_val);
                    JS_FreeCString(ctx_, key);
                }
                JS_FreeAtom(ctx_, props[i].atom);
            }
            js_free(ctx_, props);
        }
        return obj;
    }
    return nullptr;
}

void FetchBindings::EvalJS(const std::string& code, const std::string& filename) {
    JSValue result = JS_Eval(ctx_, code.c_str(), code.length(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        JSValue exception = JS_GetException(ctx_);
        const char* err_str = JS_ToCString(ctx_, exception);
        if (err_str) {
            JS_FreeCString(ctx_, err_str);
        }
        JS_FreeValue(ctx_, exception);
    }
    JS_FreeValue(ctx_, result);
}

} // namespace mbink

