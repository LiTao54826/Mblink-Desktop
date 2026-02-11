/**
 * @file bindings.cpp
 * @brief LightUI Python 绑定 (pybind11)
 *
 * 使用 pybind11 将 LightUI C++ API 暴露给 Python。
 * 编译成 .pyd (Windows) 或 .so (Linux/macOS) 扩展模块。
 *
 * Phase 1: 状态管理功能（已完成）
 * Phase 2: Window 绑定（已完成）
 * Phase 3: 完整功能修复（增量渲染、FetchBindings、DevTools 等）
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <iostream>

#include "core/bridge/state_manager.h"
#include "core/bridge/host_bridge.h"
#include "core/window/window.h"
#include "core/window/window_manager.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/bindings/dom_bindings.h"
#include "core/quickjs/dom_binding_map.h"

#include "core/event/loop/event_loop.h"
#include "core/event/loop/task_scheduler.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/quickjs/window_bindings.h"
#include "core/network/fetch_bindings.h"
#include "core/devtools/devtools_manager.h"
#include "core/render/image/image_loader.h"
#include "core/render/text/font_manager.h"
#include "nlohmann/json.hpp"

#include <memory>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <filesystem>

namespace py = pybind11;
using namespace lightui;
using json = nlohmann::json;
namespace fs = std::filesystem;

// 前向声明
class PyRuntime;
class PyWindow;

// ========== JSON ↔ Python 转换 ==========

py::object jsonToPython(const json& j) {
    if (j.is_null()) return py::none();
    if (j.is_boolean()) return py::bool_(j.get<bool>());
    if (j.is_number_integer()) return py::int_(j.get<int64_t>());
    if (j.is_number_float()) return py::float_(j.get<double>());
    if (j.is_string()) return py::str(j.get<std::string>());
    if (j.is_array()) {
        py::list result;
        for (const auto& item : j) {
            result.append(jsonToPython(item));
        }
        return result;
    }
    if (j.is_object()) {
        py::dict result;
        for (auto& [key, value] : j.items()) {
            result[py::str(key)] = jsonToPython(value);
        }
        return result;
    }
    return py::none();
}

json pythonToJson(const py::object& obj) {
    if (obj.is_none()) return nullptr;
    if (py::isinstance<py::bool_>(obj)) return obj.cast<bool>();
    if (py::isinstance<py::int_>(obj)) return obj.cast<int64_t>();
    if (py::isinstance<py::float_>(obj)) return obj.cast<double>();
    if (py::isinstance<py::str>(obj)) return obj.cast<std::string>();
    if (py::isinstance<py::list>(obj)) {
        json arr = json::array();
        for (auto item : obj) {
            arr.push_back(pythonToJson(py::reinterpret_borrow<py::object>(item)));
        }
        return arr;
    }
    if (py::isinstance<py::dict>(obj)) {
        json dict = json::object();
        for (auto item : obj.cast<py::dict>()) {
            dict[item.first.cast<std::string>()] = 
                pythonToJson(py::reinterpret_borrow<py::object>(item.second));
        }
        return dict;
    }
    return nullptr;
}

// ========== State 类包装 ==========

class PyState {
protected:
    StateManager* sm_;
    std::string name_;
    std::unordered_map<int, py::function> callbacks_;

public:
    PyState(StateManager* sm, const std::string& name) 
        : sm_(sm), name_(name) {}
    
    virtual ~PyState() = default;
    
    py::object get() {
        return jsonToPython(sm_->getJson(name_));
    }
    
    void set(const py::object& value) {
        sm_->setJson(name_, pythonToJson(value));
        // 注意：不在这里调用 processQueue()
        // 队列将在 EventLoop 的每帧更新中由主线程处理
    }
    
    int watch(py::function callback) {
        auto cb = callback;
        int watchId = sm_->watch(name_, [cb](const std::string& name, const json& value) {
            py::gil_scoped_acquire acquire;
            try {
                cb(jsonToPython(value));
            } catch (const py::error_already_set& e) {
                // Python 异常，记录但不抛出
            }
        });
        callbacks_[watchId] = callback;
        return watchId;
    }
    
    void unwatch(int watchId) {
        sm_->unwatch(watchId);
        callbacks_.erase(watchId);
    }
    
    const std::string& getName() const { return name_; }
};

class PyIntState : public PyState {
public:
    using PyState::PyState;
    
    int64_t get() { return sm_->getInt(name_); }
    
    void set(int64_t value) { 
        sm_->setInt(name_, value); 
    }
    
    void increment(int64_t delta = 1) { 
        sm_->increment(name_, static_cast<double>(delta)); 
    }
    
    void multiply(double factor) { 
        sm_->multiply(name_, factor); 
    }
};

class PyStringState : public PyState {
public:
    using PyState::PyState;
    
    std::string get() { return sm_->getString(name_); }
    
    void set(const std::string& value) { 
        sm_->setString(name_, value); 
    }
    
    void append(const std::string& suffix) { 
        sm_->stringAppend(name_, suffix); 
    }
    
    void prepend(const std::string& prefix) { 
        sm_->stringPrepend(name_, prefix); 
    }
    
    size_t len() { return sm_->getLength(name_); }
};

class PyListState : public PyState {
public:
    using PyState::PyState;
    
    void append(const py::object& item) { 
        sm_->arrayPush(name_, pythonToJson(item)); 
    }
    
    void pop() { 
        sm_->arrayPop(name_); 
    }
    
    void shift() { 
        sm_->arrayShift(name_); 
    }
    
    void unshift(const py::object& item) { 
        sm_->arrayUnshift(name_, pythonToJson(item)); 
    }
    
    void remove(int index) { 
        sm_->arrayRemove(name_, index); 
    }
    
    void clear() { 
        sm_->arrayClear(name_); 
    }
    
    size_t len() { return sm_->getLength(name_); }
    
    py::object getitem(int index) {
        return jsonToPython(sm_->getAt(name_, index));
    }
    
    void setitem(int index, const py::object& value) {
        sm_->arraySet(name_, index, pythonToJson(value));
    }
};

class PyDictState : public PyState {
public:
    using PyState::PyState;
    
    void set_key(const std::string& key, const py::object& value) {
        sm_->objectSet(name_, key, pythonToJson(value));
    }
    
    void remove_key(const std::string& key) { 
        sm_->objectRemove(name_, key); 
    }
    
    void clear() { 
        sm_->objectClear(name_); 
    }
    
    py::object getitem(const std::string& key) {
        return jsonToPython(sm_->getKey(name_, key));
    }
    
    bool contains(const std::string& key) {
        return !sm_->getKey(name_, key).is_null();
    }
    
    py::list keys() {
        auto obj = sm_->getJson(name_);
        py::list result;
        if (obj.is_object()) {
            for (auto& [key, _] : obj.items()) {
                result.append(key);
            }
        }
        return result;
    }
};


// ========== App 类 ==========

class PyApp {
public:
    PyApp(const std::string& title = "LightUI App", int width = 800, int height = 600)
        : title_(title), width_(width), height_(height) {
        sm_ = std::make_unique<StateManager>();
    }
    
    ~PyApp() = default;
    
    py::object state(const std::string& name, const py::object& initial) {
        auto it = states_.find(name);
        if (it != states_.end()) {
            return it->second;
        }
        
        json initialJson = pythonToJson(initial);
        
        if (initialJson.is_null()) {
            sm_->createNull(name);
            auto state = std::make_shared<PyState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_boolean()) {
            sm_->createBool(name, initialJson.get<bool>());
            auto state = std::make_shared<PyState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_number_integer()) {
            sm_->createInt(name, initialJson.get<int64_t>());
            auto state = std::make_shared<PyIntState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_number_float()) {
            sm_->createDouble(name, initialJson.get<double>());
            auto state = std::make_shared<PyState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_string()) {
            sm_->createString(name, initialJson.get<std::string>());
            auto state = std::make_shared<PyStringState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_array()) {
            sm_->createJson(name, initialJson);
            auto state = std::make_shared<PyListState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        else if (initialJson.is_object()) {
            sm_->createJson(name, initialJson);
            auto state = std::make_shared<PyDictState>(sm_.get(), name);
            py::object pyState = py::cast(state);
            states_[name] = pyState;
            return pyState;
        }
        
        sm_->createNull(name);
        auto state = std::make_shared<PyState>(sm_.get(), name);
        py::object pyState = py::cast(state);
        states_[name] = pyState;
        return pyState;
    }
    
    void bind(const std::string& name, py::function func) {
        boundFunctions_[name] = func;
    }
    
    void unbind(const std::string& name) {
        boundFunctions_.erase(name);
    }
    
    void load_file(const std::string& path) {
        throw std::runtime_error("load_file not implemented - requires Window integration");
    }
    
    void load_js(const std::string& code) {
        throw std::runtime_error("load_js not implemented - requires Window integration");
    }
    
    void run() {
        throw std::runtime_error("run not implemented - requires Window integration");
    }
    
    void stop() {}
    
    void batch_begin() { sm_->batchBegin(); }
    void batch_end() { sm_->batchEnd(); }
    void process_queue() { sm_->processQueue(); }
    StateManager* getStateManager() { return sm_.get(); }

private:
    std::string title_;
    int width_;
    int height_;
    std::unique_ptr<StateManager> sm_;
    std::unordered_map<std::string, py::object> states_;
    std::unordered_map<std::string, py::function> boundFunctions_;
};

class BatchContext {
public:
    BatchContext(PyApp* app) : app_(app) {}
    
    BatchContext& enter() {
        app_->batch_begin();
        return *this;
    }
    
    void exit(py::object, py::object, py::object) {
        app_->batch_end();
    }

private:
    PyApp* app_;
};

// ========== Phase 2: Window 绑定 ==========

/**
 * @brief PyElement - DOM 元素包装类
 */
class PyElement {
public:
    PyElement(std::shared_ptr<Element> element) : element_(element) {}
    
    std::string getTagName() const {
        return element_ ? element_->GetTagName() : "";
    }
    
    std::string getId() const {
        return element_ ? element_->GetAttribute("id") : "";
    }
    
    void setId(const std::string& id) {
        if (element_) element_->SetAttribute("id", id);
    }
    
    std::string getClassName() const {
        return element_ ? element_->GetAttribute("class") : "";
    }
    
    void setClassName(const std::string& className) {
        if (element_) element_->SetAttribute("class", className);
    }
    
    std::string getAttribute(const std::string& name) const {
        return element_ ? element_->GetAttribute(name) : "";
    }
    
    void setAttribute(const std::string& name, const std::string& value) {
        if (element_) element_->SetAttribute(name, value);
    }
    
    void removeAttribute(const std::string& name) {
        if (element_) element_->RemoveAttribute(name);
    }
    
    std::string getInnerHTML() const {
        return element_ ? element_->GetInnerHTML() : "";
    }
    
    void setInnerHTML(const std::string& html) {
        if (element_) element_->SetInnerHTML(html);
    }
    
    std::string getTextContent() const {
        return element_ ? element_->GetTextContent() : "";
    }
    
    void setTextContent(const std::string& text) {
        if (element_) element_->SetTextContent(text);
    }
    
    py::object querySelector(const std::string& selector) {
        if (!element_) return py::none();
        auto elem = element_->QuerySelector(selector);
        if (!elem) return py::none();
        return py::cast(std::make_shared<PyElement>(elem));
    }
    
    py::list querySelectorAll(const std::string& selector) {
        py::list result;
        if (!element_) return result;
        auto elements = element_->QuerySelectorAll(selector);
        for (auto& elem : elements) {
            result.append(py::cast(std::make_shared<PyElement>(elem)));
        }
        return result;
    }
    
    std::shared_ptr<Element> getElement() const { return element_; }
    
    bool isValid() const { return element_ != nullptr; }

private:
    std::shared_ptr<Element> element_;
};

/**
 * @brief PyDocument - DOM 文档包装类
 */
class PyDocument {
public:
    PyDocument(std::shared_ptr<Document> doc) : doc_(doc) {}
    
    py::object createElement(const std::string& tagName) {
        if (!doc_) return py::none();
        auto elem = doc_->CreateElement(tagName);
        if (!elem) return py::none();
        return py::cast(std::make_shared<PyElement>(elem));
    }
    
    py::object querySelector(const std::string& selector) {
        if (!doc_) return py::none();
        auto body = doc_->GetBody();
        if (!body) return py::none();
        auto elem = body->QuerySelector(selector);
        if (!elem) return py::none();
        return py::cast(std::make_shared<PyElement>(elem));
    }
    
    py::list querySelectorAll(const std::string& selector) {
        py::list result;
        if (!doc_) return result;
        auto body = doc_->GetBody();
        if (!body) return result;
        auto elements = body->QuerySelectorAll(selector);
        for (auto& elem : elements) {
            result.append(py::cast(std::make_shared<PyElement>(elem)));
        }
        return result;
    }
    
    py::object getBody() {
        if (!doc_) return py::none();
        auto body = doc_->GetBody();
        if (!body) return py::none();
        return py::cast(std::make_shared<PyElement>(body));
    }
    
    py::object getElementById(const std::string& id) {
        if (!doc_) return py::none();
        auto elem = doc_->GetElementById(id);
        if (!elem) return py::none();
        return py::cast(std::make_shared<PyElement>(elem));
    }
    
    bool loadHTML(const std::string& html) {
        if (!doc_) return false;
        bool result = doc_->LoadHTML(html);
        if (result) {
            // 重新设置全局 document（因为 LoadHTML 会重建 DOM 树）
            // 这样 JS 端的 document.head/body 才能正确获取
            auto runtime = doc_->GetJSRuntime();
            if (runtime) {
                DOMBindings::SetGlobalDocument(runtime->GetContext(), doc_);
            }
            // 自动执行 <script> 标签中的代码
            doc_->ExecuteScripts();
        }
        return result;
    }

    bool loadHTMLFile(const std::string& path) {
        if (!doc_) return false;
        bool result = doc_->LoadHTMLFile(path);
        if (result) {
            // 重新设置全局 document（因为 LoadHTML 会重建 DOM 树）
            auto runtime = doc_->GetJSRuntime();
            if (runtime) {
                DOMBindings::SetGlobalDocument(runtime->GetContext(), doc_);
            }
            // 自动执行 <script> 标签中的代码
            doc_->ExecuteScripts();
        }
        return result;
    }
    
    std::string saveHTML() {
        if (!doc_) return "";
        return doc_->SaveHTML();
    }
    
    std::shared_ptr<Document> getDocument() const { return doc_; }
    
    bool isValid() const { return doc_ != nullptr; }

private:
    std::shared_ptr<Document> doc_;
};

/**
 * @brief PyWindow - 窗口包装类
 */
class PyWindow {
public:
    PyWindow(const std::string& title = "LightUI Window", 
             int width = 800, int height = 600,
             bool headless = false) {
        WindowConfig config;
        config.title = title;
        config.width = width;
        config.height = height;
        config.headless = headless;
        window_ = std::make_shared<Window>(config);
        
        // 创建 Document 并设置到窗口
        document_ = std::make_shared<Document>();
        window_->SetDocument(document_);
        
        // 创建 TaskScheduler（用于 setTimeout/setInterval/requestAnimationFrame）
        task_scheduler_ = std::make_shared<TaskScheduler>();
        
        // 注册到 WindowManager，这样 EventLoop 才能正确管理窗口
        WindowManager::Instance().RegisterWindow(window_);
    }
    
    ~PyWindow() {
        // 清理 WindowBindings
        window_bindings_.reset();
        
        // 从 WindowManager 注销
        if (window_) {
            WindowManager::Instance().UnregisterWindow(window_);
        }
    }
    
    void show() { if (window_) window_->Show(); }
    void hide() { if (window_) window_->Hide(); }
    void close() { if (window_) window_->SetShouldClose(true); }
    
    std::string getTitle() const {
        return window_ ? window_->GetTitle() : "";
    }
    
    void setTitle(const std::string& title) {
        if (window_) window_->SetTitle(title);
    }
    
    py::tuple getSize() const {
        int w = 0, h = 0;
        if (window_) window_->GetSize(&w, &h);
        return py::make_tuple(w, h);
    }
    
    void setSize(int width, int height) {
        if (window_) window_->SetSize(width, height);
    }
    
    py::tuple getPosition() const {
        int x = 0, y = 0;
        if (window_) window_->GetPosition(&x, &y);
        return py::make_tuple(x, y);
    }
    
    void setPosition(int x, int y) {
        if (window_) window_->SetPosition(x, y);
    }
    
    void setFullscreen(bool fullscreen) {
        if (window_) window_->SetFullscreen(fullscreen);
    }
    
    void setResizable(bool resizable) {
        if (window_) window_->SetResizable(resizable);
    }
    
    void setBorderless(bool borderless) {
        if (window_) window_->SetBorderless(borderless);
    }
    
    void setAlwaysOnTop(bool onTop) {
        if (window_) window_->SetAlwaysOnTop(onTop);
    }
    
    void minimize() { if (window_) window_->Minimize(); }
    void maximize() { if (window_) window_->Maximize(); }
    void restore() { if (window_) window_->Restore(); }
    
    bool shouldClose() const {
        return window_ ? window_->ShouldClose() : true;
    }
    
    void render() {
        if (window_) window_->Render();
    }
    
    void swapBuffers() {
        if (window_) window_->SwapBuffers();
    }
    
    py::object getDocument() {
        if (!window_) return py::none();
        auto doc = window_->GetDocument();
        if (!doc) return py::none();
        return py::cast(std::make_shared<PyDocument>(doc));
    }
    
    void setDocument(std::shared_ptr<PyDocument> pyDoc) {
        if (window_ && pyDoc) {
            window_->SetDocument(pyDoc->getDocument());
        }
    }
    
    void setJSRuntime(QuickJSRuntime* runtime) {
        if (document_ && runtime) {
            document_->SetJSRuntime(runtime);
            // 初始化 DOM 绑定并设置全局 document 对象
            DOMBindings::Init(runtime->GetContext());
            DOMBindings::SetGlobalDocument(runtime->GetContext(), document_);
            
            // 初始化 WindowBindings（提供 setTimeout, setInterval, requestAnimationFrame 等）
            window_bindings_ = std::make_unique<WindowBindings>(runtime, window_, task_scheduler_);
            window_bindings_->InitBindings();
        }
    }
    
    /**
     * @brief 获取 TaskScheduler（用于 EventLoop 集成）
     */
    std::shared_ptr<TaskScheduler> getTaskScheduler() const { return task_scheduler_; }
    
    // 事件回调
    void setOnResize(py::function callback) {
        if (window_) {
            auto cb = callback;
            window_->SetOnResizeCallback([cb](int w, int h) {
                py::gil_scoped_acquire acquire;
                try { cb(w, h); } catch (...) {}
            });
        }
    }
    
    void setOnClose(py::function callback) {
        if (window_) {
            auto cb = callback;
            window_->SetOnCloseCallback([cb]() {
                py::gil_scoped_acquire acquire;
                try { cb(); } catch (...) {}
            });
        }
    }
    
    void setOnFocus(py::function callback) {
        if (window_) {
            auto cb = callback;
            window_->SetOnFocusCallback([cb]() {
                py::gil_scoped_acquire acquire;
                try { cb(); } catch (...) {}
            });
        }
    }
    
    void setOnBlur(py::function callback) {
        if (window_) {
            auto cb = callback;
            window_->SetOnBlurCallback([cb]() {
                py::gil_scoped_acquire acquire;
                try { cb(); } catch (...) {}
            });
        }
    }
    
    /**
     * @brief 检查窗口是否需要重绘（增量渲染）
     */
    bool needsRepaint() const {
        return window_ ? window_->NeedsRepaint() : false;
    }

    /**
     * @brief 标记窗口需要重绘
     */
    void markDirty() {
        if (window_) window_->SetNeedsRepaint();
    }

    std::shared_ptr<Window> getWindow() const { return window_; }
    std::shared_ptr<Document> getDocumentPtr() const { return document_; }

    bool isValid() const { return window_ != nullptr; }

private:
    std::shared_ptr<Window> window_;
    std::shared_ptr<Document> document_;
    std::shared_ptr<TaskScheduler> task_scheduler_;
    std::unique_ptr<WindowBindings> window_bindings_;
};

/**
 * @brief PyEventLoop - 事件循环包装类（支持增量渲染）
 */
class PyEventLoop {
public:
    PyEventLoop() : event_loop_(std::make_unique<EventLoop>()) {}

    /**
     * @brief 使用 TaskScheduler 创建事件循环（支持 setTimeout 等异步任务）
     */
    PyEventLoop(std::shared_ptr<TaskScheduler> task_scheduler)
        : task_scheduler_(task_scheduler)
        , event_loop_(std::make_unique<EventLoop>(task_scheduler)) {}

    ~PyEventLoop() = default;

    void run() {
        if (event_loop_) {
            // 设置内部更新回调，每帧处理 StateManager 队列
            if (state_manager_) {
                auto sm = state_manager_;
                auto user_callback = user_update_callback_;
                auto bridge = host_bridge_;
                event_loop_->SetUpdateCallback([sm, user_callback, bridge](float dt) {
                    // 先处理状态队列（主线程）
                    sm->processQueue();

                    // 刷新事件队列（状态 watcher 可能产生了事件）
                    if (bridge) {
                        bridge->flushEvents();
                    }

                    // 再调用用户回调
                    if (user_callback) {
                        py::gil_scoped_acquire acquire;
                        try { user_callback(dt); } catch (...) {}
                    }
                });
            }

            // 设置默认渲染回调（增量渲染）
            if (!user_render_callback_ && window_) {
                auto win = window_;
                event_loop_->SetRenderCallback([win]() {
                    if (win->NeedsRepaint()) {
                        win->Render();
                        win->SwapBuffers();
                    }
                });
            }

            py::gil_scoped_release release;
            event_loop_->Run();
        }
        // 事件循环结束后，强制退出以避免资源清理时卡住
        // 参考 esm_loader 的实现
        std::quick_exit(0);
    }

    void stop() {
        if (event_loop_) event_loop_->Stop();
    }

    void runOnce() {
        if (event_loop_) {
            // 处理状态队列
            if (state_manager_) {
                state_manager_->processQueue();
            }
            py::gil_scoped_release release;
            event_loop_->RunOnce();
        }
    }

    bool isRunning() const {
        return event_loop_ ? event_loop_->IsRunning() : false;
    }

    bool shouldQuit() const {
        return event_loop_ ? event_loop_->ShouldQuit() : true;
    }

    /**
     * @brief 设置 QuickJS 运行时（用于执行 JS 定时器回调）
     */
    void setQuickJSRuntime(QuickJSRuntime* runtime) {
        if (event_loop_) {
            event_loop_->SetQuickJSRuntime(runtime);
            // 设置全局 EventLoop（用于 execCommand 等 DOM API）
            if (runtime) {
                DOMBindings::SetGlobalEventLoop(runtime->GetContext(), event_loop_.get());
            }
        }
    }

    /**
     * @brief 设置 Window（用于自动增量渲染）
     */
    void setWindow(std::shared_ptr<Window> window) {
        window_ = window;
    }

    /**
     * @brief 设置 StateManager（用于线程安全的状态队列处理）
     *
     * 设置后，EventLoop 会在每帧自动调用 StateManager::processQueue()
     * 这样后台线程的状态更新会在主线程统一处理
     */
    void setStateManager(StateManager* sm) {
        state_manager_ = sm;
    }

    /**
     * @brief 设置 HostBridge（用于事件队列刷新）
     *
     * 设置后，EventLoop 会在每帧 processQueue 之后自动调用 flushEvents()
     */
    void setHostBridge(HostBridge* bridge) {
        host_bridge_ = bridge;
    }

    void setUpdateCallback(py::function callback) {
        user_update_callback_ = callback;
        // 如果还没有 StateManager，直接设置回调
        if (!state_manager_ && event_loop_) {
            auto cb = callback;
            event_loop_->SetUpdateCallback([cb](float dt) {
                py::gil_scoped_acquire acquire;
                try { cb(dt); } catch (...) {}
            });
        }
    }

    void setRenderCallback(py::function callback) {
        user_render_callback_ = callback;
        if (event_loop_) {
            auto cb = callback;
            auto win = window_;
            event_loop_->SetRenderCallback([cb, win]() {
                py::gil_scoped_acquire acquire;
                try {
                    cb();
                } catch (...) {}

                // 如果用户没有手动渲染，自动执行增量渲染
                if (win && win->NeedsRepaint()) {
                    win->Render();
                    win->SwapBuffers();
                }
            });
        }
    }

    void setIdleCallback(py::function callback) {
        if (event_loop_) {
            auto cb = callback;
            event_loop_->SetIdleCallback([cb]() {
                py::gil_scoped_acquire acquire;
                try { cb(); } catch (...) {}
            });
        }
    }

    EventLoop* getEventLoop() const { return event_loop_.get(); }

private:
    std::shared_ptr<TaskScheduler> task_scheduler_;
    std::unique_ptr<EventLoop> event_loop_;
    std::shared_ptr<Window> window_;         // 窗口（用于自动增量渲染）
    StateManager* state_manager_ = nullptr;  // 状态管理器（用于线程安全队列处理）
    HostBridge* host_bridge_ = nullptr;      // HostBridge（用于事件队列刷新）
    py::function user_update_callback_;      // 用户的更新回调
    py::function user_render_callback_;      // 用户的渲染回调
};

/**
 * @brief PyRuntime - QuickJS 运行时包装类（支持 ES 模块和 FetchBindings）
 */
class PyRuntime {
public:
    PyRuntime() : runtime_(std::make_unique<QuickJSRuntime>()) {}

    ~PyRuntime() {
        // 清理 FetchBindings
        fetch_bindings_.reset();

        // 清理 DOM 绑定和全局 Node* -> JSValue 映射
        if (runtime_) {
            JSContext* ctx = runtime_->GetContext();
            DOMBindings::Cleanup(ctx);
            DOMBindingMap::GetInstance().Clear();
        }

        // 清理运行时
        runtime_.reset();
    }

    py::object eval(const std::string& code, const std::string& filename = "<eval>") {
        if (!runtime_) return py::none();

        try {
            auto result = runtime_->Eval(code, filename);
            return jsonToPython(result);
        } catch (const std::exception& e) {
            std::cerr << "[JS Error] " << filename << ": " << e.what() << std::endl;
            throw py::value_error(std::string("JavaScript execution failed: ") + e.what());
        }
    }

    py::object evalModule(const std::string& code, const std::string& filename = "<module>") {
        if (!runtime_) return py::none();

        try {
            auto result = runtime_->EvalModule(code, filename);
            return jsonToPython(result);
        } catch (const std::exception& e) {
            std::cerr << "[JS Module Error] " << filename << ": " << e.what() << std::endl;
            throw py::value_error(std::string("JavaScript module execution failed: ") + e.what());
        }
    }

    py::object evalFile(const std::string& path) {
        if (!runtime_) return py::none();

        try {
            auto result = runtime_->EvalFile(path);
            return jsonToPython(result);
        } catch (const std::exception& e) {
            std::cerr << "[JS File Error] " << path << ": " << e.what() << std::endl;
            throw py::value_error(std::string("JavaScript file execution failed: ") + e.what());
        }
    }

    /**
     * @brief 注册虚拟 ES 模块
     * @param name 模块名（如 "preact", "preact/hooks"）
     * @param code 模块代码（必须包含 export 语句）
     */
    void registerModule(const std::string& name, const std::string& code) {
        if (runtime_) {
            runtime_->RegisterModule(name, code);
        }
    }

    /**
     * @brief 设置模块基础路径（用于解析相对路径的模块）
     */
    void setBaseModulePath(const std::string& path) {
        if (runtime_) {
            runtime_->SetBaseModulePath(path);
        }
    }

    /**
     * @brief 初始化 FetchBindings（网络请求 API）
     */
    void initFetchBindings(std::shared_ptr<TaskScheduler> scheduler) {
        if (runtime_ && scheduler) {
            fetch_bindings_ = std::make_unique<FetchBindings>(runtime_->GetContext(), scheduler);
            fetch_bindings_->InitBindings();
        }
    }

    QuickJSRuntime* getRuntime() const { return runtime_.get(); }
    JSContext* getContext() const { return runtime_ ? runtime_->GetContext() : nullptr; }

private:
    std::unique_ptr<QuickJSRuntime> runtime_;
    std::unique_ptr<FetchBindings> fetch_bindings_;
};

/**
 * @brief PyHostBridge - Python ↔ JS 通信桥接类
 * 
 * 允许 Python 函数被 JS 调用，实现双向通信。
 */
class PyHostBridge {
public:
    /**
     * @brief 从 Runtime 和 App 创建 HostBridge
     */
    PyHostBridge(PyRuntime* runtime, PyApp* app) 
        : runtime_(runtime), stateManager_(app ? app->getStateManager() : nullptr) {
        initBridge();
    }
    
    /**
     * @brief 从 Runtime 和 StateManager 创建 HostBridge
     */
    PyHostBridge(PyRuntime* runtime, StateManager* stateManager) 
        : runtime_(runtime), stateManager_(stateManager) {
        initBridge();
    }
    
    ~PyHostBridge() = default;
    
    /**
     * @brief 绑定 Python 函数供 JS 调用
     * @param name 函数名（JS 中通过 host.call(name, args) 调用）
     * @param func Python 函数
     */
    void bind(const std::string& name, py::function func) {
        if (!bridge_) {
            throw std::runtime_error("HostBridge not initialized");
        }
        
        // 保存 Python 函数引用
        boundFunctions_[name] = func;
        
        // 注册到 HostBridge
        bridge_->bind(name, [this, name](const std::string& argsJson) -> std::string {
            py::gil_scoped_acquire acquire;
            
            try {
                auto it = boundFunctions_.find(name);
                if (it == boundFunctions_.end()) {
                    return R"({"error": "Function not found"})";
                }
                
                // 解析 JSON 参数
                py::object args = py::none();
                if (!argsJson.empty() && argsJson != "null") {
                    try {
                        json j = json::parse(argsJson);
                        args = jsonToPython(j);
                    } catch (...) {
                        // 解析失败，使用原始字符串
                        args = py::str(argsJson);
                    }
                }
                
                // 调用 Python 函数
                py::object result = it->second(args);
                
                // 转换返回值为 JSON
                json resultJson = pythonToJson(result);
                return resultJson.dump();
                
            } catch (const py::error_already_set& e) {
                // Python 异常
                std::string errorMsg = e.what();
                json errorJson = {{"error", errorMsg}};
                return errorJson.dump();
            } catch (const std::exception& e) {
                json errorJson = {{"error", e.what()}};
                return errorJson.dump();
            }
        });
    }
    
    /**
     * @brief 解绑 Python 函数
     * @param name 函数名
     */
    void unbind(const std::string& name) {
        if (bridge_) {
            bridge_->unbind(name);
        }
        boundFunctions_.erase(name);
    }
    
    /**
     * @brief 从 Python 调用 JS 中注册的函数（如果有）
     * @param name 函数名
     * @param args 参数
     * @return 返回值
     */
    py::object call(const std::string& name, py::object args = py::none()) {
        if (!bridge_) {
            throw std::runtime_error("HostBridge not initialized");
        }
        
        json argsJson = pythonToJson(args);
        std::string result = bridge_->call(name, argsJson.dump());
        
        try {
            json resultJson = json::parse(result);
            return jsonToPython(resultJson);
        } catch (...) {
            return py::str(result);
        }
    }
    
    /**
     * @brief 检查是否已初始化
     */
    bool isValid() const { return bridge_ != nullptr; }
    
    HostBridge* getBridge() const { return bridge_.get(); }

    /**
     * @brief 从 Python 端向 JS 端发送事件
     * @param eventName 事件名
     * @param data Python 对象（基本类型）
     */
    void emit(const std::string& eventName, const py::object& data) {
        if (!bridge_) {
            throw std::runtime_error("HostBridge not initialized");
        }
        if (eventName.empty()) {
            throw py::value_error("eventName must not be empty");
        }

        json j;
        try {
            j = pythonToJson(data);
        } catch (...) {
            throw py::type_error("data is not JSON-serializable");
        }
        if (!data.is_none() && j.is_null()) {
            throw py::type_error("data is not JSON-serializable: unsupported type");
        }

        bridge_->emit(eventName, j.dump());
    }

    /**
     * @brief 刷新事件队列，执行 JS 回调
     */
    void flushEvents() {
        if (bridge_) {
            bridge_->flushEvents();
        }
    }

private:
    void initBridge() {
        if (runtime_ && runtime_->getContext() && stateManager_) {
            bridge_ = std::make_unique<HostBridge>(runtime_->getContext(), stateManager_);
            bridge_->registerGlobal();
        }
    }
    
    PyRuntime* runtime_;
    StateManager* stateManager_;
    std::unique_ptr<HostBridge> bridge_;
    std::unordered_map<std::string, py::function> boundFunctions_;
};

// ========== pybind11 模块定义 ==========

PYBIND11_MODULE(lightui_core, m) {
    m.doc() = "LightUI Python bindings - State management and Window";
    
    // ===== Phase 1: State 类 =====
    py::class_<PyState, std::shared_ptr<PyState>>(m, "State")
        .def("get", &PyState::get)
        .def("set", &PyState::set, py::arg("value"))
        .def("watch", &PyState::watch, py::arg("callback"))
        .def("unwatch", &PyState::unwatch, py::arg("watch_id"))
        .def_property_readonly("name", &PyState::getName);
    
    py::class_<PyIntState, PyState, std::shared_ptr<PyIntState>>(m, "IntState")
        .def("get", &PyIntState::get)
        .def("set", &PyIntState::set, py::arg("value"))
        .def("increment", &PyIntState::increment, py::arg("delta") = 1)
        .def("multiply", &PyIntState::multiply, py::arg("factor"));
    
    py::class_<PyStringState, PyState, std::shared_ptr<PyStringState>>(m, "StringState")
        .def("get", &PyStringState::get)
        .def("set", &PyStringState::set, py::arg("value"))
        .def("append", &PyStringState::append, py::arg("suffix"))
        .def("prepend", &PyStringState::prepend, py::arg("prefix"))
        .def("__len__", &PyStringState::len);
    
    py::class_<PyListState, PyState, std::shared_ptr<PyListState>>(m, "ListState")
        .def("append", &PyListState::append, py::arg("item"))
        .def("pop", &PyListState::pop)
        .def("shift", &PyListState::shift, "Remove and return the first element")
        .def("unshift", &PyListState::unshift, py::arg("item"), "Add item to the beginning")
        .def("remove", &PyListState::remove, py::arg("index"))
        .def("clear", &PyListState::clear)
        .def("__len__", &PyListState::len)
        .def("__getitem__", &PyListState::getitem)
        .def("__setitem__", &PyListState::setitem);
    
    py::class_<PyDictState, PyState, std::shared_ptr<PyDictState>>(m, "DictState")
        .def("set_key", &PyDictState::set_key, py::arg("key"), py::arg("value"))
        .def("remove_key", &PyDictState::remove_key, py::arg("key"))
        .def("clear", &PyDictState::clear)
        .def("keys", &PyDictState::keys)
        .def("__getitem__", &PyDictState::getitem)
        .def("__setitem__", &PyDictState::set_key)
        .def("__contains__", &PyDictState::contains);
    
    py::class_<PyApp>(m, "App")
        .def(py::init<const std::string&, int, int>(),
             py::arg("title") = "LightUI App",
             py::arg("width") = 800,
             py::arg("height") = 600)
        .def("state", &PyApp::state, py::arg("name"), py::arg("initial") = py::none())
        .def("bind", &PyApp::bind, py::arg("name"), py::arg("func"))
        .def("unbind", &PyApp::unbind, py::arg("name"))
        .def("load_file", &PyApp::load_file, py::arg("path"))
        .def("load_js", &PyApp::load_js, py::arg("code"))
        .def("run", &PyApp::run)
        .def("stop", &PyApp::stop)
        .def("batch_begin", &PyApp::batch_begin)
        .def("batch_end", &PyApp::batch_end)
        .def("process_queue", &PyApp::process_queue)
        .def("__enter__", [](PyApp& self) -> PyApp& { return self; })
        .def("__exit__", [](PyApp& self, py::object, py::object, py::object) {});
    
    py::class_<BatchContext>(m, "BatchContext")
        .def(py::init<PyApp*>())
        .def("__enter__", &BatchContext::enter, py::return_value_policy::reference)
        .def("__exit__", &BatchContext::exit);
    
    // ===== Phase 2: Window 类 =====
    py::class_<PyElement, std::shared_ptr<PyElement>>(m, "Element")
        .def_property_readonly("tag_name", &PyElement::getTagName)
        .def_property("id", &PyElement::getId, &PyElement::setId)
        .def_property("class_name", &PyElement::getClassName, &PyElement::setClassName)
        .def("get_attribute", &PyElement::getAttribute, py::arg("name"))
        .def("set_attribute", &PyElement::setAttribute, py::arg("name"), py::arg("value"))
        .def("remove_attribute", &PyElement::removeAttribute, py::arg("name"))
        .def_property("inner_html", &PyElement::getInnerHTML, &PyElement::setInnerHTML)
        .def_property("text_content", &PyElement::getTextContent, &PyElement::setTextContent)
        .def("query_selector", &PyElement::querySelector, py::arg("selector"))
        .def("query_selector_all", &PyElement::querySelectorAll, py::arg("selector"))
        .def("is_valid", &PyElement::isValid);
    
    py::class_<PyDocument, std::shared_ptr<PyDocument>>(m, "Document")
        .def("create_element", &PyDocument::createElement, py::arg("tag_name"))
        .def("query_selector", &PyDocument::querySelector, py::arg("selector"))
        .def("query_selector_all", &PyDocument::querySelectorAll, py::arg("selector"))
        .def("get_element_by_id", &PyDocument::getElementById, py::arg("id"))
        .def_property_readonly("body", &PyDocument::getBody)
        .def("load_html", &PyDocument::loadHTML, py::arg("html"))
        .def("load_html_file", &PyDocument::loadHTMLFile, py::arg("path"))
        .def("save_html", &PyDocument::saveHTML)
        .def("set_base_path", [](PyDocument& self, const std::string& path) {
            if (self.getDocument()) {
                self.getDocument()->SetBasePath(path);
            }
        }, py::arg("path"), "Set base path for resolving relative URLs")
        .def("load_external_stylesheets", [](PyDocument& self) {
            if (self.getDocument()) {
                self.getDocument()->LoadExternalStylesheets();
            }
        }, "Load external stylesheets referenced in the document")
        .def("execute_scripts", [](PyDocument& self) {
            if (self.getDocument()) {
                self.getDocument()->ExecuteScripts();
            }
        }, "Execute all script tags in the document")
        .def("is_valid", &PyDocument::isValid);
    
    py::class_<PyWindow, std::shared_ptr<PyWindow>>(m, "Window")
        .def(py::init<const std::string&, int, int, bool>(),
             py::arg("title") = "LightUI Window",
             py::arg("width") = 800,
             py::arg("height") = 600,
             py::arg("headless") = false)
        .def("show", &PyWindow::show)
        .def("hide", &PyWindow::hide)
        .def("close", &PyWindow::close)
        .def_property("title", &PyWindow::getTitle, &PyWindow::setTitle)
        .def_property_readonly("size", &PyWindow::getSize)
        .def("set_size", &PyWindow::setSize, py::arg("width"), py::arg("height"))
        .def_property_readonly("position", &PyWindow::getPosition)
        .def("set_position", &PyWindow::setPosition, py::arg("x"), py::arg("y"))
        .def("set_fullscreen", &PyWindow::setFullscreen, py::arg("fullscreen"))
        .def("set_resizable", &PyWindow::setResizable, py::arg("resizable"))
        .def("set_borderless", &PyWindow::setBorderless, py::arg("borderless"))
        .def("set_always_on_top", &PyWindow::setAlwaysOnTop, py::arg("on_top"))
        .def("minimize", &PyWindow::minimize)
        .def("maximize", &PyWindow::maximize)
        .def("restore", &PyWindow::restore)
        .def("should_close", &PyWindow::shouldClose)
        .def("render", &PyWindow::render)
        .def("swap_buffers", &PyWindow::swapBuffers)
        .def("needs_repaint", &PyWindow::needsRepaint, "Check if window needs repainting (incremental rendering)")
        .def("mark_dirty", &PyWindow::markDirty, "Mark window as needing repaint")
        .def_property_readonly("document", &PyWindow::getDocument)
        .def("set_js_runtime", [](PyWindow& self, PyRuntime& runtime) {
            self.setJSRuntime(runtime.getRuntime());
        }, py::arg("runtime"), "Set the JavaScript runtime for the window's document")
        .def("set_on_resize", &PyWindow::setOnResize, py::arg("callback"))
        .def("set_on_close", &PyWindow::setOnClose, py::arg("callback"))
        .def("set_on_focus", &PyWindow::setOnFocus, py::arg("callback"))
        .def("set_on_blur", &PyWindow::setOnBlur, py::arg("callback"))
        .def("get_task_scheduler", &PyWindow::getTaskScheduler,
             "Get the TaskScheduler for use with EventLoop")
        .def("is_valid", &PyWindow::isValid);
    
    py::class_<TaskScheduler, std::shared_ptr<TaskScheduler>>(m, "TaskScheduler")
        .def(py::init<>());
    
    py::class_<PyEventLoop>(m, "EventLoop")
        .def(py::init<>())
        .def(py::init<std::shared_ptr<TaskScheduler>>(), py::arg("task_scheduler"),
             "Create EventLoop with TaskScheduler for setTimeout/setInterval support")
        .def("run", &PyEventLoop::run)
        .def("stop", &PyEventLoop::stop)
        .def("set_quickjs_runtime", [](PyEventLoop& self, PyRuntime& runtime) {
            self.setQuickJSRuntime(runtime.getRuntime());
        }, py::arg("runtime"), "Set QuickJS runtime for executing JS timer callbacks and global EventLoop")
        .def("set_window", [](PyEventLoop& self, std::shared_ptr<PyWindow> window) {
            self.setWindow(window->getWindow());
        }, py::arg("window"), "Set Window for automatic incremental rendering")
        .def("set_state_manager", [](PyEventLoop& self, PyApp& app) {
            self.setStateManager(app.getStateManager());
        }, py::arg("app"), "Set StateManager for thread-safe state queue processing (called each frame)")
        .def("set_host_bridge", [](PyEventLoop& self, PyHostBridge& bridge) {
            self.setHostBridge(bridge.getBridge());
        }, py::arg("bridge"), "Set HostBridge for automatic event queue flushing after state processing")
        .def("run_once", &PyEventLoop::runOnce)
        .def("is_running", &PyEventLoop::isRunning)
        .def("should_quit", &PyEventLoop::shouldQuit)
        .def("set_update_callback", &PyEventLoop::setUpdateCallback, py::arg("callback"))
        .def("set_render_callback", &PyEventLoop::setRenderCallback, py::arg("callback"))
        .def("set_idle_callback", &PyEventLoop::setIdleCallback, py::arg("callback"));
    
    py::class_<PyRuntime>(m, "Runtime")
        .def(py::init<>())
        .def("eval", &PyRuntime::eval, py::arg("code"), py::arg("filename") = "<eval>")
        .def("eval_module", &PyRuntime::evalModule, py::arg("code"), py::arg("filename") = "<module>",
             "Execute ES6 module code (supports import/export)")
        .def("eval_file", &PyRuntime::evalFile, py::arg("path"))
        .def("register_module", &PyRuntime::registerModule, py::arg("name"), py::arg("code"),
             "Register a virtual ES module (e.g., 'preact', 'preact/hooks')")
        .def("set_base_module_path", &PyRuntime::setBaseModulePath, py::arg("path"),
             "Set base path for resolving relative module imports")
        .def("init_fetch_bindings", [](PyRuntime& self, std::shared_ptr<TaskScheduler> scheduler) {
            self.initFetchBindings(scheduler);
        }, py::arg("task_scheduler"), "Initialize fetch API for network requests");

    py::class_<PyHostBridge>(m, "HostBridge")
        .def(py::init<PyRuntime*, PyApp*>(), py::arg("runtime"), py::arg("app"),
             "Create HostBridge from Runtime and App")
        .def("bind", &PyHostBridge::bind, py::arg("name"), py::arg("func"),
             "Bind a Python function to be callable from JS via host.call(name, args)")
        .def("unbind", &PyHostBridge::unbind, py::arg("name"),
             "Unbind a previously bound function")
        .def("call", &PyHostBridge::call, py::arg("name"), py::arg("args") = py::none(),
             "Call a function registered in the bridge")
        .def("emit", &PyHostBridge::emit, py::arg("event_name"), py::arg("data") = py::none(),
             "Emit an event to JS listeners (thread-safe)")
        .def("flush_events", &PyHostBridge::flushEvents,
             "Flush event queue and execute JS callbacks")
        .def("is_valid", &PyHostBridge::isValid,
             "Check if the bridge is properly initialized");

    // ===== DevTools 支持 =====
    // DevToolsManager 是单例，析构函数是私有的
    // 使用 std::unique_ptr 的自定义删除器（不删除）来绑定
    py::class_<DevToolsManager, std::unique_ptr<DevToolsManager, py::nodelete>>(m, "DevToolsManager")
        .def_static("get_instance", []() -> DevToolsManager* {
            return &DevToolsManager::GetInstance();
        }, py::return_value_policy::reference,
           "Get the singleton DevTools manager instance")
        .def("initialize", [](DevToolsManager& self, std::shared_ptr<PyDocument> doc, std::shared_ptr<PyWindow> win) {
            if (doc && win) {
                self.Initialize(doc->getDocument().get(), win->getWindow().get());
            }
        }, py::arg("document"), py::arg("window"), "Initialize DevTools with document and window")
        .def("open", &DevToolsManager::Open, "Open the DevTools panel")
        .def("close", &DevToolsManager::Close, "Close the DevTools panel")
        .def("toggle", &DevToolsManager::Toggle, "Toggle DevTools visibility")
        .def("is_open", &DevToolsManager::IsOpen, "Check if DevTools is open")
        .def("shutdown", &DevToolsManager::Shutdown, "Shutdown DevTools");

    // ===== 全局辅助函数 =====
    m.def("set_image_base_path", [](const std::string& path) {
        ImageLoader::SetBasePath(path);
    }, py::arg("path"), "Set base path for loading images");

    m.def("clear_font_cache", []() {
        FontManager::GetInstance().ClearCache();
    }, "Clear the font cache");

    m.def("cleanup_dom_bindings", [](PyRuntime& runtime) {
        if (runtime.getContext()) {
            JSContext* ctx = runtime.getContext();
            DOMBindings::Cleanup(ctx);
            DOMBindingMap::GetInstance().Clear();
        }
    }, py::arg("runtime"), "Clean up DOM bindings and binding map (call before destroying runtime)");

    m.def("version", []() { return "0.5.0"; });
}
