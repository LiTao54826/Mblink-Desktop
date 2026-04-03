/**
 * @file fetch_bindings.cpp
 * @brief fetch API 绑定实现
 */

#include "fetch_bindings.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace mbink {
namespace {

std::string NormalizePathString(std::string path, bool force_leading_slash = true) {
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.empty()) {
        return force_leading_slash ? "/" : "";
    }

    fs::path normalized_fs = fs::path(path).lexically_normal();
    path = normalized_fs.generic_string();
    if (path == ".") {
        return force_leading_slash ? "/" : "";
    }

    if (force_leading_slash && !path.empty() && path.front() != '/') {
        path.insert(path.begin(), '/');
    }
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

bool IsAbsoluteFsPath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    fs::path fs_path(path);
    if (fs_path.is_absolute()) {
        return true;
    }
#ifdef _WIN32
    return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':';
#else
    return false;
#endif
}

bool IsNetworkUrl(const std::string& url) {
    return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
}

bool IsDataUrl(const std::string& url) {
    return url.rfind("data:", 0) == 0;
}

std::string ResolvePathAgainstBase(const std::string& path, const std::string& base_path) {
    if (path.empty()) {
        return "";
    }
    if (IsNetworkUrl(path) || IsDataUrl(path)) {
        return path;
    }
    if (IsAbsoluteFsPath(path) || path.front() == '/') {
        return NormalizePathString(path, true);
    }
    if (base_path.empty()) {
        return NormalizePathString(path, false);
    }
    return NormalizePathString((fs::path(base_path) / fs::path(path)).generic_string(), true);
}

std::string EncodeBase64(const std::string& input) {
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= input.size()) {
        const unsigned int a = static_cast<unsigned char>(input[i++]);
        const unsigned int b = static_cast<unsigned char>(input[i++]);
        const unsigned int c = static_cast<unsigned char>(input[i++]);
        output.push_back(kAlphabet[(a >> 2) & 0x3F]);
        output.push_back(kAlphabet[((a & 0x03) << 4) | ((b >> 4) & 0x0F)]);
        output.push_back(kAlphabet[((b & 0x0F) << 2) | ((c >> 6) & 0x03)]);
        output.push_back(kAlphabet[c & 0x3F]);
    }

    const size_t remaining = input.size() - i;
    if (remaining == 1) {
        const unsigned int a = static_cast<unsigned char>(input[i]);
        output.push_back(kAlphabet[(a >> 2) & 0x3F]);
        output.push_back(kAlphabet[(a & 0x03) << 4]);
        output.push_back('=');
        output.push_back('=');
    } else if (remaining == 2) {
        const unsigned int a = static_cast<unsigned char>(input[i++]);
        const unsigned int b = static_cast<unsigned char>(input[i]);
        output.push_back(kAlphabet[(a >> 2) & 0x3F]);
        output.push_back(kAlphabet[((a & 0x03) << 4) | ((b >> 4) & 0x0F)]);
        output.push_back(kAlphabet[(b & 0x0F) << 2]);
        output.push_back('=');
    }
    return output;
}

HttpResponse LoadLocalResponse(const std::string& resolved_path,
                               const FetchBindings::AssetProvider& asset_provider) {
    HttpResponse response;
    response.status_code = 404;
    response.status_text = "Not Found";

    if (resolved_path.empty()) {
        response.error = "Invalid local resource path";
        return response;
    }

    if (asset_provider) {
        std::vector<uint8_t> data;
        if (asset_provider(resolved_path, data)) {
            response.status_code = 200;
            response.status_text = "OK";
            response.ok = true;
            response.body.assign(reinterpret_cast<const char*>(data.data()), data.size());
            return response;
        }
    }

    std::ifstream file(fs::path(resolved_path), std::ios::binary);
    if (!file.is_open()) {
        response.error = "Local resource not found: " + resolved_path;
        return response;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    response.status_code = 200;
    response.status_text = "OK";
    response.ok = true;
    response.body = buffer.str();
    return response;
}

} // namespace

FetchBindings* FetchBindings::instance_ = nullptr;
FetchBindings::AssetProvider FetchBindings::asset_provider_ = nullptr;
std::string FetchBindings::base_path_;

void FetchBindings::SetAssetProvider(AssetProvider provider) {
    asset_provider_ = std::move(provider);
}

FetchBindings::AssetProvider FetchBindings::GetAssetProvider() {
    return asset_provider_;
}

void FetchBindings::SetBasePath(const std::string& path) {
    base_path_ = path;
}

const std::string& FetchBindings::GetBasePath() {
    return base_path_;
}

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

    function decodeBase64ToUint8Array(base64) {
        if (!base64) {
            return new Uint8Array(0);
        }
        const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
        const cleaned = String(base64).replace(/=+$/, '');
        const bytes = [];
        let buffer = 0;
        let bits = 0;
        for (let i = 0; i < cleaned.length; i++) {
            const value = chars.indexOf(cleaned[i]);
            if (value < 0) {
                continue;
            }
            buffer = (buffer << 6) | value;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                bytes.push((buffer >> bits) & 0xFF);
            }
        }
        return new Uint8Array(bytes);
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
            this._bodyBase64 = data.bodyBase64 || '';
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
            if (this._bodyBase64) {
                const bytes = decodeBase64ToUint8Array(this._bodyBase64);
                if (typeof TextDecoder !== 'undefined') {
                    return new TextDecoder().decode(bytes);
                }
                let result = '';
                for (const byte of bytes) {
                    result += String.fromCharCode(byte);
                }
                return result;
            }
            return this._body || '';
        }

        async json() {
            const text = await this.text();
            return JSON.parse(text);
        }

        async blob() {
            if (this._bodyUsed) {
                throw new TypeError('Body has already been consumed');
            }
            this._bodyUsed = true;
            const bytes = this._bodyBase64 ? decodeBase64ToUint8Array(this._bodyBase64) : new TextEncoder().encode(this._body);
            return new Blob([bytes]);
        }

        async arrayBuffer() {
            if (this._bodyUsed) {
                throw new TypeError('Body has already been consumed');
            }
            this._bodyUsed = true;
            const bytes = this._bodyBase64 ? decodeBase64ToUint8Array(this._bodyBase64) : new TextEncoder().encode(this._body);
            return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
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

    HttpRequestOptions http_options;
    if (options.contains("method")) {
        http_options.method = options["method"].get<std::string>();
        std::transform(http_options.method.begin(), http_options.method.end(), http_options.method.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    }

    if (options.contains("headers") && options["headers"].is_object()) {
        for (auto& [key, value] : options["headers"].items()) {
            http_options.headers[key] = value.get<std::string>();
        }
    }

    if (options.contains("body") && options["body"].is_string()) {
        http_options.body = options["body"].get<std::string>();
    }

    if (!IsNetworkUrl(url) && !IsDataUrl(url)) {
        const std::string resolved_path = ResolvePathAgainstBase(url, base_path_);
        const AssetProvider provider = asset_provider_;

        std::thread([this, request_id, resolved_path, provider, method = http_options.method]() {
            HttpResponse response;
            if (method != "GET" && method != "HEAD") {
                response.status_code = 405;
                response.status_text = "Method Not Allowed";
                response.error = "Local/resource fetch only supports GET and HEAD";
            } else {
                response = LoadLocalResponse(resolved_path, provider);
                if (method == "HEAD") {
                    response.body.clear();
                }
            }

            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_responses_.push({request_id, response});
        }).detach();

        return request_id;
    }

    http_client_->RequestAsync(url, http_options,
        [this, request_id](const HttpResponse& response) {
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
    result["bodyBase64"] = EncodeBase64(response.body);

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

