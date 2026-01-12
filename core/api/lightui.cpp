/**
 * @file lightui.cpp
 * @brief LightUI C API 实现
 */

#include "lightui.h"
#include "core/bridge/state_manager.h"

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace {

// 全局状态
bool g_initialized = false;
std::string g_lastError;
std::mutex g_errorMutex;

// 窗口上下文（简化版，仅包含 StateManager）
struct WindowContext {
    std::unique_ptr<lightui::StateManager> stateManager;
    std::unordered_map<int, std::pair<LightUIStateCallback, void*>> watchCallbacks;
    std::unordered_map<std::string, std::pair<LightUICallback, void*>> boundFunctions;
    std::string title;
    int width = 800;
    int height = 600;
};

// 设置错误信息
void setLastError(const std::string& error) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastError = error;
}

// 转换错误码
int toErrorCode(lightui::LightUIError err) {
    switch (err) {
        case lightui::LightUIError::Ok: return LIGHTUI_OK;
        case lightui::LightUIError::InvalidHandle: return LIGHTUI_ERROR_INVALID_HANDLE;
        case lightui::LightUIError::NotFound: return LIGHTUI_ERROR_NOT_FOUND;
        case lightui::LightUIError::TypeMismatch: return LIGHTUI_ERROR_TYPE_MISMATCH;
        case lightui::LightUIError::IndexOutOfRange: return LIGHTUI_ERROR_OUT_OF_RANGE;
        case lightui::LightUIError::InvalidJson: return LIGHTUI_ERROR_INVALID_PARAM;
        case lightui::LightUIError::AlreadyExists: return LIGHTUI_ERROR_INVALID_PARAM;
        case lightui::LightUIError::InvalidName: return LIGHTUI_ERROR_INVALID_PARAM;
        default: return LIGHTUI_ERROR_UNKNOWN;
    }
}

// 转换类型
LightUIType toLightUIType(lightui::LightUIType type) {
    switch (type) {
        case lightui::LightUIType::Null: return LIGHTUI_TYPE_NULL;
        case lightui::LightUIType::Bool: return LIGHTUI_TYPE_BOOL;
        case lightui::LightUIType::Int: return LIGHTUI_TYPE_INT;
        case lightui::LightUIType::Double: return LIGHTUI_TYPE_DOUBLE;
        case lightui::LightUIType::String: return LIGHTUI_TYPE_STRING;
        case lightui::LightUIType::Array: return LIGHTUI_TYPE_ARRAY;
        case lightui::LightUIType::Object: return LIGHTUI_TYPE_OBJECT;
        default: return LIGHTUI_TYPE_NULL;
    }
}

// 获取上下文
WindowContext* getContext(LightUIHandle handle) {
    return reinterpret_cast<WindowContext*>(handle);
}

// 复制字符串（调用者需要 free）
char* duplicateString(const std::string& str) {
    char* result = static_cast<char*>(malloc(str.size() + 1));
    if (result) {
        memcpy(result, str.c_str(), str.size() + 1);
    }
    return result;
}

} // anonymous namespace

// ========== 生命周期 ==========

int lightui_init(void) {
    if (g_initialized) return LIGHTUI_OK;
    g_initialized = true;
    return LIGHTUI_OK;
}

void lightui_cleanup(void) {
    g_initialized = false;
}

const char* lightui_version(void) {
    return "0.1.0";
}

// ========== 窗口管理 ==========

LightUIHandle lightui_create(const char* title, int width, int height) {
    if (!g_initialized) {
        setLastError("LightUI not initialized");
        return nullptr;
    }
    
    try {
        auto ctx = new WindowContext();
        ctx->stateManager = std::make_unique<lightui::StateManager>();
        ctx->title = title ? title : "LightUI";
        ctx->width = width;
        ctx->height = height;
        return reinterpret_cast<LightUIHandle>(ctx);
    } catch (const std::exception& e) {
        setLastError(e.what());
        return nullptr;
    }
}

void lightui_destroy(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    delete ctx;
}

void lightui_run(LightUIHandle handle) {
    // TODO: 集成窗口事件循环
    (void)handle;
}

void lightui_stop(LightUIHandle handle) {
    // TODO: 停止窗口事件循环
    (void)handle;
}

bool lightui_poll_events(LightUIHandle handle) {
    if (!handle) return false;
    auto ctx = getContext(handle);
    // 处理状态队列
    ctx->stateManager->processQueue();
    return true;
}

int lightui_set_title(LightUIHandle handle, const char* title) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    if (title) ctx->title = title;
    return LIGHTUI_OK;
}

int lightui_set_size(LightUIHandle handle, int width, int height) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->width = width;
    ctx->height = height;
    return LIGHTUI_OK;
}


// ========== UI 加载 ==========

int lightui_load_js(LightUIHandle handle, const char* js_code) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!js_code) return LIGHTUI_ERROR_INVALID_PARAM;
    // TODO: 集成 QuickJS 运行时
    (void)js_code;
    return LIGHTUI_OK;
}

int lightui_load_file(LightUIHandle handle, const char* filepath) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!filepath) return LIGHTUI_ERROR_INVALID_PARAM;
    // TODO: 加载文件并执行
    (void)filepath;
    return LIGHTUI_OK;
}

int lightui_load_bytecode(LightUIHandle handle, const void* data, size_t size) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!data || size == 0) return LIGHTUI_ERROR_INVALID_PARAM;
    // TODO: 加载字节码
    (void)data;
    (void)size;
    return LIGHTUI_OK;
}

// ========== 函数绑定 ==========

int lightui_bind(LightUIHandle handle, const char* name,
                 LightUICallback callback, void* user_data) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !callback) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    ctx->boundFunctions[name] = {callback, user_data};
    return LIGHTUI_OK;
}

void lightui_unbind(LightUIHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->boundFunctions.erase(name);
}

// ========== 状态创建 ==========

int lightui_state_create_null(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createNull(name));
}

int lightui_state_create_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createBool(name, value));
}

int lightui_state_create_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createInt(name, value));
}

int lightui_state_create_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createDouble(name, value));
}

int lightui_state_create_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createString(name, value ? value : ""));
}

int lightui_state_create_array(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createArray(name));
}

int lightui_state_create_object(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->createObject(name));
}

int lightui_state_create_json(LightUIHandle handle, const char* name, const char* json_str) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return LIGHTUI_ERROR_INVALID_PARAM;
    
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(json_str);
        return toErrorCode(ctx->stateManager->createJson(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}


// ========== 状态查询 ==========

bool lightui_state_exists(LightUIHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->exists(name);
}

LightUIType lightui_state_type(LightUIHandle handle, const char* name) {
    if (!handle || !name) return LIGHTUI_TYPE_NULL;
    auto ctx = getContext(handle);
    return toLightUIType(ctx->stateManager->type(name));
}

void lightui_state_delete(LightUIHandle handle, const char* name) {
    if (!handle || !name) return;
    auto ctx = getContext(handle);
    ctx->stateManager->remove(name);
    ctx->stateManager->processQueue();
}

// ========== 状态读取（直接类型） ==========

bool lightui_state_get_bool(LightUIHandle handle, const char* name) {
    if (!handle || !name) return false;
    auto ctx = getContext(handle);
    return ctx->stateManager->getBool(name);
}

int64_t lightui_state_get_int(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getInt(name);
}

double lightui_state_get_double(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0.0;
    auto ctx = getContext(handle);
    return ctx->stateManager->getDouble(name);
}

const char* lightui_state_get_string(LightUIHandle handle, const char* name) {
    if (!handle || !name) return "";
    auto ctx = getContext(handle);
    return ctx->stateManager->getString(name).c_str();
}

int lightui_state_get_length(LightUIHandle handle, const char* name) {
    if (!handle || !name) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->getLength(name));
}

// ========== 状态读取（JSON） ==========

char* lightui_state_get_json(LightUIHandle handle, const char* name) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getJson(name);
    return duplicateString(j.dump());
}

char* lightui_state_get_at(LightUIHandle handle, const char* name, int index) {
    if (!handle || !name) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getAt(name, index);
    return duplicateString(j.dump());
}

char* lightui_state_get_key(LightUIHandle handle, const char* name, const char* key) {
    if (!handle || !name || !key) return nullptr;
    auto ctx = getContext(handle);
    auto j = ctx->stateManager->getKey(name, key);
    return duplicateString(j.dump());
}

// ========== 状态写入 ==========

int lightui_state_set_null(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setNull(name));
}

int lightui_state_set_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setBool(name, value));
}

int lightui_state_set_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setInt(name, value));
}

int lightui_state_set_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setDouble(name, value));
}

int lightui_state_set_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->setString(name, value ? value : ""));
}

int lightui_state_set_json(LightUIHandle handle, const char* name, const char* json_str) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !json_str) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(json_str);
        return toErrorCode(ctx->stateManager->setJson(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}


// ========== 数组操作 ==========

int lightui_state_array_push(LightUIHandle handle, const char* name, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayPush(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_push_int(LightUIHandle handle, const char* name, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_push_double(LightUIHandle handle, const char* name, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_push_string(LightUIHandle handle, const char* name, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value ? value : ""));
}

int lightui_state_array_push_bool(LightUIHandle handle, const char* name, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPush(name, value));
}

int lightui_state_array_pop(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayPop(name));
}

int lightui_state_array_shift(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayShift(name));
}

int lightui_state_array_unshift(LightUIHandle handle, const char* name, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arrayUnshift(name, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_remove(LightUIHandle handle, const char* name, int index) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayRemove(name, index));
}

int lightui_state_array_clear(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arrayClear(name));
}

int lightui_state_array_set(LightUIHandle handle, const char* name, int index, const char* item_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !item_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(item_json);
        return toErrorCode(ctx->stateManager->arraySet(name, index, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_array_set_int(LightUIHandle handle, const char* name, int index, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int lightui_state_array_set_double(LightUIHandle handle, const char* name, int index, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value));
}

int lightui_state_array_set_string(LightUIHandle handle, const char* name, int index, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->arraySet(name, index, value ? value : ""));
}


// ========== 对象操作 ==========

int lightui_state_object_set(LightUIHandle handle, const char* name, const char* key, const char* value_json) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key || !value_json) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    try {
        auto j = lightui::json::parse(value_json);
        return toErrorCode(ctx->stateManager->objectSet(name, key, j));
    } catch (...) {
        return LIGHTUI_ERROR_INVALID_PARAM;
    }
}

int lightui_state_object_set_int(LightUIHandle handle, const char* name, const char* key, int64_t value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_set_double(LightUIHandle handle, const char* name, const char* key, double value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_set_string(LightUIHandle handle, const char* name, const char* key, const char* value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value ? value : ""));
}

int lightui_state_object_set_bool(LightUIHandle handle, const char* name, const char* key, bool value) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectSet(name, key, value));
}

int lightui_state_object_remove(LightUIHandle handle, const char* name, const char* key) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !key) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectRemove(name, key));
}

int lightui_state_object_clear(LightUIHandle handle, const char* name) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->objectClear(name));
}

// ========== 数值操作 ==========

int lightui_state_increment(LightUIHandle handle, const char* name, double delta) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->increment(name, delta));
}

int lightui_state_multiply(LightUIHandle handle, const char* name, double factor) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->multiply(name, factor));
}

// ========== 字符串操作 ==========

int lightui_state_string_append(LightUIHandle handle, const char* name, const char* suffix) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !suffix) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringAppend(name, suffix));
}

int lightui_state_string_prepend(LightUIHandle handle, const char* name, const char* prefix) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    if (!name || !prefix) return LIGHTUI_ERROR_INVALID_PARAM;
    auto ctx = getContext(handle);
    return toErrorCode(ctx->stateManager->stringPrepend(name, prefix));
}

// ========== 监听 ==========

int lightui_state_watch(LightUIHandle handle, const char* name,
                        LightUIStateCallback callback, void* user_data) {
    if (!handle) return -1;
    if (!name || !callback) return -1;
    
    auto ctx = getContext(handle);
    int watchId = ctx->stateManager->watch(name, 
        [callback, user_data](const std::string& n, const lightui::json& v) {
            std::string jsonStr = v.dump();
            callback(n.c_str(), jsonStr.c_str(), user_data);
        });
    
    ctx->watchCallbacks[watchId] = {callback, user_data};
    return watchId;
}

void lightui_state_unwatch(LightUIHandle handle, int watch_id) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->unwatch(watch_id);
    ctx->watchCallbacks.erase(watch_id);
}

// ========== 批量操作 ==========

void lightui_state_batch_begin(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchBegin();
}

void lightui_state_batch_end(LightUIHandle handle) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->batchEnd();
}

// ========== 队列控制 ==========

void lightui_state_set_merge_mode(LightUIHandle handle, bool enable) {
    if (!handle) return;
    auto ctx = getContext(handle);
    ctx->stateManager->setMergeMode(enable);
}

int lightui_process_queue(LightUIHandle handle) {
    if (!handle) return LIGHTUI_ERROR_INVALID_HANDLE;
    auto ctx = getContext(handle);
    ctx->stateManager->processQueue();
    return LIGHTUI_OK;
}

int lightui_queue_size(LightUIHandle handle) {
    if (!handle) return 0;
    auto ctx = getContext(handle);
    return static_cast<int>(ctx->stateManager->queueSize());
}

// ========== 工具函数 ==========

void lightui_free(void* ptr) {
    free(ptr);
}

const char* lightui_last_error(void) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return g_lastError.c_str();
}
