/**
 * @file window.cpp
 * @brief 窗口管理模块实现
 *
 * 实现内容：
 * - SDL3窗口创建和管理
 * - OpenGL上下文初始化
 * - Skia渲染表面创建
 * - 窗口事件处理
 */

// 性能优化：默认关闭调试日志（可以通过定义LIGHTUI_DEBUG_RENDERING启用）
// #define LIGHTUI_DEBUG_RENDERING

#ifdef LIGHTUI_DEBUG_RENDERING
    #define DEBUG_LOG(msg) std::cout << msg << std::endl
    #define DEBUG_LOG_FLUSH() std::cout.flush()
#else
    #define DEBUG_LOG(msg) ((void)0)
    #define DEBUG_LOG_FLUSH() ((void)0)
#endif

#include "window.h"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include "include/core/SkColorSpace.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkFont.h"
#include "include/core/SkRegion.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/dom_observer.h"
#include "core/render/renderer.h"
#include "core/render/render_object.h"
#include "core/render/style_resolver.h"
#include "core/render/text/font_manager.h"
#include "core/render/dirty_region.h"
#include "core/render/dirty_region_collector.h"
#include "core/render/transition.h"
#include "core/render/animation_timeline.h"
#include "core/render/animation_controller.h"
#include "core/render/render_tree_updater.h"
#include "core/layout/layout_engine.h"
#include "core/render/color.h"
#include "core/render/select_dropdown.h"
#include "core/utils/encoding_utils.h"
#include "core/devtools/devtools_manager.h"

namespace lightui {

// 从 render_object.cpp 导入的绘制统计变量
extern std::atomic<int> g_paint_total_calls;
extern std::atomic<int> g_paint_culled_calls;

/**
 * @brief Window 的 DOM 观察者
 *
 * 监听 DOM 变化并触发窗口重绘
 */
class WindowDOMObserver : public DOMObserver {
public:
    explicit WindowDOMObserver(Window* window) : window_(window) {}

    void OnNodeAdded(Node* node, Node* parent) override {
        DEBUG_LOG("[WindowDOMObserver::OnNodeAdded] node=" << node
                  << ", parent=" << parent
                  << ", IsInBatch=" << (node ? IsInBatch(node) : false));
        if (window_ && !IsInBatch(node)) {
            // 简化处理：总是使用 InvalidateRenderTree 重建整个渲染树
            // 这避免了增量更新可能导致的布局不一致和崩溃问题
            // 虽然性能略低，但更加稳定可靠
            window_->InvalidateRenderTree();
            window_->SetForceFullRepaint(true);
            window_->SetNeedsRepaint();
        }
    }


    void OnNodeRemoved(Node* node, Node* parent) override {
        if (window_ && !IsInBatch(node)) {
            // 简化处理：总是使用 InvalidateRenderTree 重建整个渲染树
            // 这避免了增量更新可能导致的布局不一致和崩溃问题
            window_->InvalidateRenderTree();
            window_->SetForceFullRepaint(true);
            window_->SetNeedsRepaint();
        }
    }


    void OnAttributeChanged(Element* element,
                           const std::string& name,
                           const std::string& old_value,
                           const std::string& new_value) override {
        if (window_ && !IsInBatch(element)) {
            // Phase 1: 精确脏区域标记
            // 获取关联的 RenderObject，标记其需要重绘
            if (auto render_obj = element->GetRenderObject()) {
                render_obj->MarkNeedsPaint();
                // 记录脏矩形（旧位置）
                SkRect bounds = render_obj->GetBoundingRect();
                if (!bounds.isEmpty()) {
                    element->SetDirtyRect(bounds);
                    window_->AddDirtyRect(bounds);
                    
                    // 调试输出
                    static int debug_count = 0;
                    if (++debug_count <= 5 && name == "style") {
                        std::cout << "[OnAttributeChanged] Old bounds: " 
                                  << bounds.left() << "," << bounds.top() << " " 
                                  << bounds.width() << "x" << bounds.height() << std::endl;
                    }
                }
            }
            window_->SetNeedsRepaint();
            // 注意：属性变化不调用 InvalidateRenderTree()，保持渲染树结构
        }
    }

    void OnStyleChanged(Element* element,
                       const std::string& property,
                       const std::string& old_value,
                       const std::string& new_value) override {
        if (window_ && !IsInBatch(element)) {
            // 特殊处理: display 属性变化影响元素的 RenderObject 存在性
            // display: none 的元素没有 RenderObject，变为 block/flex 等需要创建
            // 反之亦然，需要删除 RenderObject
            if (property == "display") {
                bool was_none = (old_value == "none" || old_value.empty());
                bool is_none = (new_value == "none");
                if (was_none != is_none) {
                    // 可见性发生变化，需要重建渲染树
                    window_->InvalidateRenderTree();
                    window_->SetNeedsRepaint();
                    return;
                }
            }
            
            // Phase 1: 精确脏区域标记
            // 样式变化可能影响布局或绘制
            if (auto render_obj = element->GetRenderObject()) {
                // 某些样式属性只影响绘制，不影响布局
                static const std::vector<std::string> paint_only_props = {
                    "color", "background-color", "background-image",
                    "border-color", "opacity", "visibility",
                    "box-shadow", "text-shadow", "outline"
                };

                bool is_paint_only = false;
                for (const auto& prop : paint_only_props) {
                    if (property == prop) {
                        is_paint_only = true;
                        break;
                    }
                }

                if (is_paint_only) {
                    render_obj->MarkNeedsPaint();
                } else {
                    // 其他属性可能影响布局
                    render_obj->MarkNeedsLayout();
                    render_obj->MarkNeedsPaint();
                }

                // 记录脏矩形
                SkRect bounds = render_obj->GetBoundingRect();
                if (!bounds.isEmpty()) {
                    element->SetDirtyRect(bounds);
                    window_->AddDirtyRect(bounds);
                }
            }
            window_->SetNeedsRepaint();
            // 样式变化不调用 InvalidateRenderTree()，保持渲染树结构
        }
    }

    void OnTextChanged(Node* node,
                      const std::string& old_text,
                      const std::string& new_text) override {
        if (window_ && !IsInBatch(node)) {
            // Phase 2: 文本内容变化的局部重绘
            // 先尝试获取节点自身的 RenderObject
            auto render_obj = node->GetRenderObject();

            // 如果节点没有 RenderObject，尝试获取父节点的
            if (!render_obj) {
                if (auto parent = node->GetParentNode()) {
                    render_obj = parent->GetRenderObject();
                }
            }

            if (render_obj) {
                // 如果是 RenderText，直接更新文本内容
                if (render_obj->GetType() == RenderObjectType::TEXT) {
                    auto render_text = static_cast<RenderText*>(render_obj.get());
                    // 直接设置新文本（SyncRenderTree 会处理规范化）
                    render_text->SetText(new_text);
                }

                // 文本内容变化需要重新布局（尺寸可能改变）
                render_obj->MarkNeedsLayout();
                render_obj->MarkNeedsPaint();

                // Update content version for incremental layout optimization
                // **Feature: incremental-layout-optimization**
                // **Validates: Requirements 1.1**
                if (auto* engine = window_->GetLayoutEngine()) {
                    engine->UpdateContentVersion(render_obj.get());
                }

                // 记录脏矩形
                SkRect bounds = render_obj->GetBoundingRect();
                if (!bounds.isEmpty()) {
                    node->SetDirtyRect(bounds);
                    window_->AddDirtyRect(bounds);
                }
            }
            window_->SetNeedsRepaint();
        }
    }

    void OnSubtreeModified(Node* root) override {
        if (window_) {
            window_->SetNeedsRepaint();
            window_->InvalidateRenderTree();
        }
    }

    void OnPseudoClassChanged(std::shared_ptr<Element> element,
                             const std::string& pseudo_class,
                             bool activate) override {
        if (window_ && !IsInBatch(element.get())) {
            // 性能优化：只有可能有视觉变化的伪类才触发重绘
            bool needs_repaint = false;

            if (pseudo_class == "hover") {
                // 只有这些元素有内置的 :hover 样式
                std::string tag = element->GetTagName();
                if (tag == "button" || tag == "a" || tag == "input" ||
                    tag == "textarea" || tag == "select") {
                    needs_repaint = true;
                }
            } else if (pseudo_class == "active" || pseudo_class == "focus" ||
                       pseudo_class == "focus-visible" || pseudo_class == "checked" ||
                       pseudo_class == "disabled") {
                needs_repaint = true;
            }

            if (needs_repaint) {
                // Phase 1: 精确脏区域标记
                // 伪类变化只影响绘制，不影响渲染树结构
                if (auto render_obj = element->GetRenderObject()) {
                    render_obj->MarkNeedsPaint();

                    // 记录脏矩形
                    SkRect bounds = render_obj->GetBoundingRect();
                    if (!bounds.isEmpty()) {
                        element->SetDirtyRect(bounds);
                        window_->AddDirtyRect(bounds);
                    }
                }
                window_->SetNeedsRepaint();
            }
        }
    }

private:
    /**
     * @brief 检查节点是否在批量更新中
     */
    bool IsInBatch(Node* node) const {
        if (!node) return false;

        auto doc = node->GetOwnerDocument();
        if (!doc) return false;

        auto document = std::dynamic_pointer_cast<Document>(doc);
        if (!document) return false;

        return document->IsInBatch();
    }

    Window* window_;
};

// 静态成员：SDL初始化计数器
static int sdl_init_count = 0;

#ifdef _WIN32
// Windows 子类化窗口过程，用于拦截可能导致闪烁的消息
static std::unordered_map<HWND, WNDPROC> g_original_wndprocs;
static std::unordered_map<HWND, Window*> g_hwnd_to_window;

// 调试：是否启用消息日志
static bool g_debug_messages = false;
static int g_paint_count = 0;
static int g_present_count = 0;
static DWORD g_last_stats_time = 0;

static const char* GetMessageName(UINT msg) {
    switch (msg) {
        case WM_PAINT: return "WM_PAINT";
        case WM_ERASEBKGND: return "WM_ERASEBKGND";
        case WM_NCPAINT: return "WM_NCPAINT";
        case WM_NCACTIVATE: return "WM_NCACTIVATE";
        case WM_NCHITTEST: return "WM_NCHITTEST";
        case WM_SETCURSOR: return "WM_SETCURSOR";
        case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
        case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
        case WM_SYNCPAINT: return "WM_SYNCPAINT";
        case WM_SETREDRAW: return "WM_SETREDRAW";
        case WM_NCMOUSEMOVE: return "WM_NCMOUSEMOVE";
        case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
        case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
        case WM_SIZE: return "WM_SIZE";
        case WM_MOVE: return "WM_MOVE";
        case WM_ACTIVATE: return "WM_ACTIVATE";
        case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
        case WM_DWMCOMPOSITIONCHANGED: return "WM_DWMCOMPOSITIONCHANGED";
        default: return nullptr;
    }
}

// 打印统计信息（每秒一次）
static void PrintStats() {
    DWORD now = GetTickCount();
    if (now - g_last_stats_time >= 1000) {
        if (g_debug_messages && (g_paint_count > 0 || g_present_count > 0)) {
            std::cout << "[Stats] WM_PAINT: " << g_paint_count
                      << "/s, Present: " << g_present_count << "/s" << std::endl;
        }
        g_paint_count = 0;
        g_present_count = 0;
        g_last_stats_time = now;
    }
}

// 缓存窗口大小，用于检测虚假的大小变化
static std::unordered_map<HWND, RECT> g_window_rects;

static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto proc_it = g_original_wndprocs.find(hwnd);

    if (proc_it == g_original_wndprocs.end()) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // 调试输出
    if (g_debug_messages) {
        const char* name = GetMessageName(msg);
        if (name) {
            std::cout << "[WndProc] " << name << " (0x" << std::hex << msg << std::dec << ")" << std::endl;
        }
    }

    // 查找关联的 Window 对象
    auto window_it = g_hwnd_to_window.find(hwnd);
    Window* window = (window_it != g_hwnd_to_window.end()) ? window_it->second : nullptr;

    switch (msg) {
        case WM_ERASEBKGND:
            // 阻止 Windows 擦除背景，避免闪烁
            return 1;

        case WM_WINDOWPOSCHANGING: {
            // 关键修复：阻止虚拟机中的虚假大小变化
            WINDOWPOS* wp = (WINDOWPOS*)lParam;

            if (g_debug_messages && !(wp->flags & SWP_NOSIZE)) {
                auto rect_it = g_window_rects.find(hwnd);
                if (rect_it != g_window_rects.end()) {
                    RECT& cached = rect_it->second;
                    int cached_width = cached.right - cached.left;
                    int cached_height = cached.bottom - cached.top;
                    std::cout << "[WndProc] WINDOWPOSCHANGING: cached=" << cached_width << "x" << cached_height
                              << " new=" << wp->cx << "x" << wp->cy
                              << " flags=0x" << std::hex << wp->flags << std::dec << std::endl;
                }
            }

            // 获取缓存的窗口位置
            auto rect_it = g_window_rects.find(hwnd);
            if (rect_it != g_window_rects.end()) {
                RECT& cached = rect_it->second;
                int cached_width = cached.right - cached.left;
                int cached_height = cached.bottom - cached.top;

                // 如果大小没有真正变化，阻止它（包括相同大小的情况）
                if (!(wp->flags & SWP_NOSIZE)) {
                    int dw = abs(wp->cx - cached_width);
                    int dh = abs(wp->cy - cached_height);
                    // 阻止所有小于等于 5 像素的变化（虚假变化）
                    if (dw <= 5 && dh <= 5) {
                        wp->flags |= SWP_NOSIZE;
                        if (g_debug_messages && (dw > 0 || dh > 0)) {
                            std::cout << "[WndProc] Blocked resize: dw=" << dw << ", dh=" << dh << std::endl;
                        }
                    }
                }
            }
            break;
        }

        case WM_WINDOWPOSCHANGED: {
            // 更新缓存的窗口大小
            WINDOWPOS* wp = (WINDOWPOS*)lParam;
            if (!(wp->flags & SWP_NOSIZE)) {
                RECT rect;
                GetWindowRect(hwnd, &rect);
                g_window_rects[hwnd] = rect;
            }
            break;
        }

        case WM_NCACTIVATE: {
            // 尝试：阻止非客户区激活导致的重绘循环
            if (g_debug_messages) {
                std::cout << "[WndProc] WM_NCACTIVATE: wParam=" << wParam << std::endl;
            }
            // 关键：使用 lParam = -1 告诉系统不要重绘非客户区
            // 但仍然调用 DefWindowProc 来更新窗口状态
            return DefWindowProc(hwnd, msg, wParam, (LPARAM)-1);
        }

        case WM_NCPAINT: {
            // 关键：完全接管非客户区绘制
            // 先让系统绘制标题栏
            LRESULT result = CallWindowProc(proc_it->second, hwnd, msg, wParam, lParam);
            // 然后阻止后续的客户区重绘
            ValidateRect(hwnd, NULL);
            return result;
        }

        case WM_PAINT: {
            g_paint_count++;
            PrintStats();

            // 检查是否使用 PaintMode 后端
            if (window && window->GetPaintModeBackend()) {
                // PaintMode: 在 WM_PAINT 中实际绘制
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                window->GetPaintModeBackend()->OnPaint(hdc, &ps.rcPaint);
                EndPaint(hwnd, &ps);
                return 0;
            }

            // 其他后端: 验证窗口区域但不绘制，我们的渲染循环会处理
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SETREDRAW:
            // 忽略 SetRedraw 调用，防止闪烁
            return 0;

        case WM_SYNCPAINT:
            // 忽略同步绘制请求
            return 0;

        case WM_SIZE: {
            // 关键：检测循环并阻止
            static DWORD last_size_time = 0;
            static int size_count = 0;
            DWORD now = GetTickCount();

            if (now - last_size_time < 100) {
                size_count++;
                if (size_count > 2) {
                    // 在 100ms 内收到超过 2 次 WM_SIZE，可能是循环
                    if (g_debug_messages) {
                        std::cout << "[WndProc] Blocked WM_SIZE loop (count=" << size_count << ")" << std::endl;
                    }
                    return 0;  // 不传递给 SDL
                }
            } else {
                size_count = 1;
            }
            last_size_time = now;
            break;
        }

        case WM_NCDESTROY:
            // 清理缓存
            g_window_rects.erase(hwnd);
            break;
    }

    return CallWindowProc(proc_it->second, hwnd, msg, wParam, lParam);
}
#endif

// SDL 事件过滤器：过滤掉可能导致闪烁的事件
// 返回 true 表示保留事件，返回 false 表示丢弃事件
static bool SDLCALL SDLEventFilter(void* userdata, SDL_Event* event) {
    (void)userdata;
    // 过滤掉 EXPOSED 事件，避免在 Windows 上触发闪烁
    if (event->type == SDL_EVENT_WINDOW_EXPOSED) {
        return false;  // 丢弃此事件
    }
    return true;  // 保留其他事件
}

Window::Window(const WindowConfig& config) : config_(config) {
    InitSDL();
    CreateSDLWindow();

#ifdef _WIN32
    // 检查调试环境变量
    if (getenv("LIGHTUI_DEBUG_MESSAGES")) {
        g_debug_messages = true;
        std::cout << "[Window] Message debugging enabled" << std::endl;
    }

    // Windows: 子类化窗口以拦截 WM_PAINT 和 WM_ERASEBKGND，防止闪烁
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdl_window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
        WNDPROC original = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SubclassWndProc);
        g_original_wndprocs[hwnd] = original;
        g_hwnd_to_window[hwnd] = this;

        // 初始化窗口大小缓存，用于检测虚假的大小变化
        RECT rect;
        GetWindowRect(hwnd, &rect);
        g_window_rects[hwnd] = rect;
    }
#endif

    // 根据配置选择渲染后端
    if (config_.backend == RenderBackend::AUTO) {
        // 自动模式：先尝试 GPU，失败则降级到 CPU
        try {
            InitOpenGL();
            InitSkia();
            CreateSkiaSurface();
            actual_backend_ = RenderBackend::OPENGL;
            std::cout << "[Window] Using GPU (OpenGL) rendering backend" << std::endl;
        } catch (const std::exception& e) {
            // GPU 初始化失败，降级到 CPU 软件渲染
            std::cerr << "GPU rendering failed: " << e.what() << ", falling back to CPU" << std::endl;
            InitCPURendering();
            actual_backend_ = RenderBackend::CPU;
            std::cout << "[Window] Using CPU rendering backend" << std::endl;
        }
    } else if (config_.backend == RenderBackend::OPENGL) {
        // 仅 GPU 模式
        InitOpenGL();
        InitSkia();
        CreateSkiaSurface();
        actual_backend_ = RenderBackend::OPENGL;
        std::cout << "[Window] Using GPU (OpenGL) rendering backend (forced)" << std::endl;
    } else if (config_.backend == RenderBackend::CPU) {
        // 仅 CPU 模式
        InitCPURendering();
        actual_backend_ = RenderBackend::CPU;
        std::cout << "[Window] Using CPU rendering backend (forced)" << std::endl;
    }

    // 初始化动画时间轴
    animation_timeline_ = std::make_unique<AnimationTimeline>();

    // 初始化动画控制器
    animation_controller_ = std::make_unique<AnimationController>();

    // 初始化 Taffy CSS 布局引擎
    layout_engine_ = std::make_unique<LayoutEngine>();
}

Window::~Window() {
    // 关键修复：在释放 Skia 资源之前，先激活 OpenGL 上下文
    // Skia 的 GrContext 在释放时需要调用 OpenGL 清理函数
    if (gl_context_ && sdl_window_) {
        SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    }

    // 释放 DisplayBackend（在销毁窗口之前）
    if (display_backend_) {
        display_backend_->Shutdown();
        display_backend_.reset();
    }

    // 释放Skia资源
    surface_.reset();
    gr_context_.reset();

    // 注意：sdl_surface_ 不需要手动销毁，它由 SDL_DestroyWindow 自动处理
    sdl_surface_ = nullptr;

#ifdef _WIN32
    // 移除窗口子类化
    if (sdl_window_) {
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdl_window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwnd) {
            auto proc_it = g_original_wndprocs.find(hwnd);
            if (proc_it != g_original_wndprocs.end()) {
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)proc_it->second);
                g_original_wndprocs.erase(proc_it);
            }
            g_hwnd_to_window.erase(hwnd);
        }
    }
#endif

    // 销毁OpenGL上下文
    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }

    // 销毁SDL窗口
    if (sdl_window_) {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }

    // 清理SDL（如果是最后一个窗口）
    sdl_init_count--;
    if (sdl_init_count == 0) {
        SDL_Quit();
    }
}

void Window::Show() {
    if (sdl_window_) {
        SDL_ShowWindow(sdl_window_);
    }
}

void Window::Hide() {
    if (sdl_window_) {
        SDL_HideWindow(sdl_window_);
    }
}

std::string Window::GetTitle() const {
    return config_.title;
}

void Window::SetTitle(const std::string& title) {
    config_.title = title;
    if (sdl_window_) {
        // title 应该已经是 UTF-8 编码，SDL 需要 UTF-8
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    }
}

void Window::SetSize(int width, int height) {
    config_.width = width;
    config_.height = height;
    if (sdl_window_) {
        SDL_SetWindowSize(sdl_window_, width, height);
        OnResize();
    }
}

void Window::GetSize(int* width, int* height) const {
    if (sdl_window_) {
        SDL_GetWindowSize(sdl_window_, width, height);
    } else {
        if (width) *width = config_.width;
        if (height) *height = config_.height;
    }
}

void Window::SetPosition(int x, int y) {
    config_.x = x;
    config_.y = y;
    if (sdl_window_) {
        SDL_SetWindowPosition(sdl_window_, x, y);
    }
}

void Window::GetPosition(int* x, int* y) const {
    if (sdl_window_) {
        SDL_GetWindowPosition(sdl_window_, x, y);
    } else {
        if (x) *x = config_.x;
        if (y) *y = config_.y;
    }
}

void Window::Minimize() {
    if (sdl_window_) {
        SDL_MinimizeWindow(sdl_window_);
    }
}

void Window::Maximize() {
    if (sdl_window_) {
        SDL_MaximizeWindow(sdl_window_);
    }
}

void Window::Restore() {
    if (sdl_window_) {
        SDL_RestoreWindow(sdl_window_);
    }
}

void Window::SetFullscreen(bool fullscreen) {
    config_.fullscreen = fullscreen;
    if (sdl_window_) {
        SDL_SetWindowFullscreen(sdl_window_, fullscreen);
    }
}

void Window::SetResizable(bool resizable) {
    config_.resizable = resizable;
    if (sdl_window_) {
        SDL_SetWindowResizable(sdl_window_, resizable);
    }
}

void Window::SetBorderless(bool borderless) {
    config_.borderless = borderless;
    if (sdl_window_) {
        SDL_SetWindowBordered(sdl_window_, !borderless);
    }
}

void Window::SetAlwaysOnTop(bool on_top) {
    config_.always_on_top = on_top;
    if (sdl_window_) {
        SDL_SetWindowAlwaysOnTop(sdl_window_, on_top);
    }
}

SkCanvas* Window::GetCanvas() const {
    return surface_ ? surface_->getCanvas() : nullptr;
}

PaintModeDisplayBackend* Window::GetPaintModeBackend() const {
#ifdef _WIN32
    if (display_backend_ && display_backend_->GetType() == DisplayBackendType::PAINT_MODE) {
        return static_cast<PaintModeDisplayBackend*>(display_backend_.get());
    }
#endif
    return nullptr;
}

void Window::SwapBuffers() {
    if (!sdl_window_) {
        return;
    }

    if (actual_backend_ == RenderBackend::OPENGL && gr_context_) {
        // GPU 模式：刷新 Skia 命令并交换 OpenGL 缓冲区
        gr_context_->flush();
        SDL_GL_SwapWindow(sdl_window_);
    }
    else if (actual_backend_ == RenderBackend::CPU && surface_) {
        // CPU 模式：使用 DisplayBackend 显示像素
        SkPixmap pixmap;
        if (!surface_->peekPixels(&pixmap)) {
            std::cerr << "Failed to peek pixels from Skia surface" << std::endl;
            return;
        }

        if (display_backend_) {
#ifdef _WIN32
            g_present_count++;
            PrintStats();
#endif
            // 使用局部更新优化 CPU 模式性能
            if (has_dirty_bounds_ && !last_dirty_bounds_.isEmpty()) {
                // 有脏区域边界：只更新脏区域
                int dirty_x = static_cast<int>(last_dirty_bounds_.left());
                int dirty_y = static_cast<int>(last_dirty_bounds_.top());
                int dirty_width = static_cast<int>(last_dirty_bounds_.width());
                int dirty_height = static_cast<int>(last_dirty_bounds_.height());
                
                // 边界检查
                int surface_width = static_cast<int>(pixmap.width());
                int surface_height = static_cast<int>(pixmap.height());
                if (dirty_x < 0) dirty_x = 0;
                if (dirty_y < 0) dirty_y = 0;
                if (dirty_x + dirty_width > surface_width) dirty_width = surface_width - dirty_x;
                if (dirty_y + dirty_height > surface_height) dirty_height = surface_height - dirty_y;
                
                if (dirty_width > 0 && dirty_height > 0) {
                    display_backend_->PresentPartial(
                        pixmap.addr(),
                        surface_width,
                        surface_height,
                        static_cast<int>(pixmap.rowBytes()),
                        dirty_x, dirty_y, dirty_width, dirty_height
                    );
                }
            } else {
                // 无脏区域边界或全量渲染：更新整个 surface
                display_backend_->Present(
                    pixmap.addr(),
                    static_cast<int>(pixmap.width()),
                    static_cast<int>(pixmap.height()),
                    static_cast<int>(pixmap.rowBytes())
                );
            }
            has_dirty_bounds_ = false;
        }
    }
}

void Window::OnResize() {
    if (!sdl_window_) return;

    // 获取客户区大小（像素，不包括标题栏和边框）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 同时更新config中的窗口大小（逻辑大小）
    int logical_width, logical_height;
    SDL_GetWindowSize(sdl_window_, &logical_width, &logical_height);
    config_.width = logical_width;
    config_.height = logical_height;

    std::cout << "[Window::OnResize] physical=" << width << "x" << height 
              << ", logical=" << logical_width << "x" << logical_height 
              << ", render_tree_valid_=" << render_tree_valid_ << std::endl;

    // 重新创建Skia渲染表面（使用客户区像素大小）
    if (actual_backend_ == RenderBackend::OPENGL) {
        // 更新 OpenGL viewport
        glViewport(0, 0, width, height);
        CreateSkiaSurface();
    } else if (actual_backend_ == RenderBackend::CPU) {
        // CPU 模式：重新创建 Skia Raster 表面
        SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
        surface_ = SkSurfaces::Raster(info);

        // 通知 DisplayBackend 窗口大小变化
        if (display_backend_) {
            display_backend_->OnResize(width, height);
        }
    }

    // 触发resize回调（使用逻辑大小）
    if (on_resize_callback_) {
        on_resize_callback_(logical_width, logical_height);
    }
}

void Window::InitSDL() {
    // 只在第一次调用时初始化SDL
    if (sdl_init_count == 0) {
        // 设置 SDL 提示，禁用 Windows 上可能导致问题的行为
        // 禁用 Windows 消息循环中的某些处理
        SDL_SetHint(SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP, "1");
        // 禁用屏幕保护程序（可选）
        SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            throw std::runtime_error(std::string("Failed to initialize SDL: ") + SDL_GetError());
        }

        // 设置事件过滤器，过滤掉可能导致闪烁的 EXPOSED 事件
        SDL_SetEventFilter(SDLEventFilter, nullptr);
    }
    sdl_init_count++;
}

void Window::CreateSDLWindow() {
    // 构建窗口标志
    SDL_WindowFlags flags = 0;

    // 所有模式都使用 OpenGL 窗口
    // CPU 模式也通过 OpenGL 纹理显示，利用 VSync 避免闪烁
    flags |= SDL_WINDOW_OPENGL;

    if (config_.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (config_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (config_.borderless) flags |= SDL_WINDOW_BORDERLESS;
    if (config_.maximized) flags |= SDL_WINDOW_MAXIMIZED;
    if (config_.minimized) flags |= SDL_WINDOW_MINIMIZED;
    if (config_.hidden) flags |= SDL_WINDOW_HIDDEN;
    if (config_.always_on_top) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (config_.high_dpi) flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    // 创建窗口
    // config_.title 应该已经是 UTF-8 编码，SDL 需要 UTF-8
    sdl_window_ = SDL_CreateWindow(
        config_.title.c_str(),
        config_.width,
        config_.height,
        flags
    );

    if (!sdl_window_) {
        throw std::runtime_error(std::string("Failed to create SDL window: ") + SDL_GetError());
    }

    // 设置窗口位置（如果指定）
    if (config_.x >= 0 && config_.y >= 0) {
        SDL_SetWindowPosition(sdl_window_, config_.x, config_.y);
    }
}

void Window::InitOpenGL() {
    if (!sdl_window_) {
        throw std::runtime_error("Cannot initialize OpenGL: window not created");
    }

    // 设置OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // 创建OpenGL上下文
    gl_context_ = SDL_GL_CreateContext(sdl_window_);
    if (!gl_context_) {
        throw std::runtime_error(std::string("Failed to create OpenGL context: ") + SDL_GetError());
    }

    // 激活上下文
    if (!SDL_GL_MakeCurrent(sdl_window_, gl_context_)) {
        throw std::runtime_error(std::string("Failed to make OpenGL context current: ") + SDL_GetError());
    }

    // 设置 VSync
    if (config_.vsync) {
        SDL_GL_SetSwapInterval(1);
    } else {
        SDL_GL_SetSwapInterval(0);
    }
}

void Window::InitSkia() {
    if (!gl_context_) {
        throw std::runtime_error("Cannot initialize Skia: OpenGL context not created");
    }

    // 创建Skia OpenGL接口
    auto interface = GrGLMakeNativeInterface();
    if (!interface) {
        throw std::runtime_error("Failed to create Skia OpenGL interface");
    }

    // 创建Skia上下文 (使用新的 API)
    gr_context_ = GrDirectContexts::MakeGL(interface);
    if (!gr_context_) {
        throw std::runtime_error("Failed to create Skia context");
    }
}

void Window::CreateSkiaSurface() {
    if (!gr_context_) {
        throw std::runtime_error("Cannot create Skia surface: Skia context not initialized");
    }

    // 获取窗口大小（像素）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 创建OpenGL帧缓冲信息
    GrGLFramebufferInfo framebuffer_info;
    framebuffer_info.fFBOID = 0;  // 0表示默认帧缓冲
    framebuffer_info.fFormat = GL_RGBA8;

    // 创建后端渲染目标
    GrBackendRenderTarget backend_render_target =
        GrBackendRenderTargets::MakeGL(width, height, 0, 8, framebuffer_info);

    // 创建Skia表面
    surface_ = SkSurfaces::WrapBackendRenderTarget(
        gr_context_.get(),
        backend_render_target,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr,
        nullptr
    );

    if (!surface_) {
        throw std::runtime_error("Failed to create Skia surface");
    }
}

void Window::InitCPURendering() {
    // CPU 软件渲染模式 - 使用 DisplayBackend 进行无闪烁显示

    // 使用物理像素大小创建渲染表面（支持高 DPI）
    int width, height;
    SDL_GetWindowSizeInPixels(sdl_window_, &width, &height);

    // 创建 Skia Raster 表面（CPU 渲染）- 使用物理像素大小
    // 注意：使用 BGRA 格式以匹配显示后端
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    surface_ = SkSurfaces::Raster(info);

    if (!surface_) {
        throw std::runtime_error("Failed to create CPU rendering surface");
    }

    // 创建最佳显示后端（按优先级：OpenGL → LayeredWindow → GDI → SDL_Surface）
    display_backend_ = DisplayBackend::CreateBest(sdl_window_, width, height);
    if (!display_backend_) {
        std::cerr << "Warning: Failed to create display backend, falling back to SDL Surface" << std::endl;
        // 如果 CreateBest 失败，尝试 SDL Surface 作为最后回退
        display_backend_ = DisplayBackend::Create(DisplayBackendType::SDL_SURFACE);
        if (display_backend_) {
            display_backend_->Initialize(sdl_window_, width, height);
        }
    }
}

bool Window::HandleSDLEvent(const SDL_Event& event) {
    // 只处理与此窗口相关的事件
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
        if (event.window.windowID != SDL_GetWindowID(sdl_window_)) {
            return false;  // 不是此窗口的事件
        }

        switch (event.type) {
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                int new_width = event.window.data1;
                int new_height = event.window.data2;

                // 检查是否真的改变了大小（避免重复处理）
                static int last_processed_width = 0, last_processed_height = 0;

                if (new_width == last_processed_width && new_height == last_processed_height) {
                    return true;  // 大小没变，跳过
                }

                last_processed_width = new_width;
                last_processed_height = new_height;

                // 直接处理 resize，不做节流
                OnResize();
                InvalidateRenderTree();  // 窗口大小改变，需要用新尺寸重建渲染树和布局
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, new_width, new_height));
                return true;
            }

            case SDL_EVENT_WINDOW_MOVED: {
                int x = event.window.data1;
                int y = event.window.data2;
                if (on_move_callback_) {
                    on_move_callback_(x, y);
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::MOVE, x, y));
                return true;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                if (on_focus_callback_) {
                    on_focus_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::FOCUS));
                return true;
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                if (on_blur_callback_) {
                    on_blur_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::BLUR));
                return true;
            }

            case SDL_EVENT_WINDOW_MINIMIZED: {
                DispatchWindowEvent(WindowEvent(WindowEventType::MINIMIZE));
                return true;
            }

            case SDL_EVENT_WINDOW_MAXIMIZED: {
                // 窗口最大化时需要触发重绘
                // 注意：不在这里调用 InvalidateRenderTree()，因为此时窗口尺寸可能还未更新
                // RESIZED 事件会随后触发，届时会正确处理渲染树重建
                std::cout << "[Window] MAXIMIZED event received" << std::endl;
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::MAXIMIZE));
                return true;
            }

            case SDL_EVENT_WINDOW_RESTORED: {
                // 窗口还原时需要触发重绘
                // 注意：不在这里调用 InvalidateRenderTree()，因为此时窗口尺寸可能还未更新
                // RESIZED 事件会随后触发，届时会正确处理渲染树重建
                SetNeedsRepaint();
                DispatchWindowEvent(WindowEvent(WindowEventType::RESTORE));
                return true;
            }

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                should_close_ = true;
                if (on_close_callback_) {
                    on_close_callback_();
                }
                DispatchWindowEvent(WindowEvent(WindowEventType::CLOSE));
                return true;
            }

            case SDL_EVENT_WINDOW_SHOWN: {
                DispatchWindowEvent(WindowEvent(WindowEventType::SHOWN));
                return true;
            }

            case SDL_EVENT_WINDOW_HIDDEN: {
                DispatchWindowEvent(WindowEvent(WindowEventType::HIDDEN));
                return true;
            }

            case SDL_EVENT_WINDOW_EXPOSED: {
                // 完全忽略 EXPOSED 事件
                // 在 Windows 上，SDL_RenderPresent 会触发 EXPOSED 事件，形成无限循环
                // 我们的渲染由 needs_repaint_ 标志控制，不需要响应 EXPOSED 事件
                static bool debug_events = std::getenv("LIGHTUI_DEBUG_EVENTS") != nullptr;
                if (debug_events) {
                    std::cout << "[Window] Ignoring EXPOSED event" << std::endl;
                }
                return true;
            }

            case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
                // 忽略显示缩放变化事件，避免可能的循环
                static bool debug_events = std::getenv("LIGHTUI_DEBUG_EVENTS") != nullptr;
                if (debug_events) {
                    std::cout << "[Window] Ignoring DISPLAY_SCALE_CHANGED event" << std::endl;
                }
                return true;
            }

            case SDL_EVENT_WINDOW_OCCLUDED: {
                // 窗口被遮挡，不需要处理
                return true;
            }

            case SDL_EVENT_WINDOW_MOUSE_ENTER: {
                DispatchWindowEvent(WindowEvent(WindowEventType::ENTER));
                return true;
            }

            case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
                DispatchWindowEvent(WindowEvent(WindowEventType::LEAVE));
                return true;
            }

            default:
                return false;
        }
    }

    return false;
}

void Window::DispatchWindowEvent(const WindowEvent& event) {
    auto it = event_listeners_.find(event.GetType());
    if (it != event_listeners_.end()) {
        for (const auto& listener : it->second) {
            listener(event);
        }
    }
}

void Window::AddEventListener(WindowEventType type, std::function<void(const WindowEvent&)> listener) {
    event_listeners_[type].push_back(listener);
}

void Window::RemoveEventListeners(WindowEventType type) {
    event_listeners_.erase(type);
}

// ========== 渲染集成 ==========

void Window::SetDocument(std::shared_ptr<Document> document) {
    // 移除旧文档的观察者
    if (document_ && dom_observer_) {
        document_->RemoveObserver(dom_observer_.get());
    }

    document_ = document;

    // 创建渲染器（如果还没有）
    if (!renderer_ && surface_) {
        renderer_ = std::make_unique<Renderer>(surface_);
    }

    // 创建并注册 DOM 观察者
    if (document_) {
        dom_observer_ = std::make_unique<WindowDOMObserver>(this);
        document_->AddObserver(dom_observer_.get());
    }

    // 标记需要重绘
    SetNeedsRepaint();
}

void Window::Render() {
    DEBUG_LOG("[Window::Render] Called, needs_repaint_=" << needs_repaint_
              << ", render_tree_valid_=" << render_tree_valid_);

    // 处理待处理的 resize（节流期间被跳过的最后一次 resize）
    if (has_pending_resize_) {
        has_pending_resize_ = false;
        OnResize();
        InvalidateRenderTree();
        SetNeedsRepaint();
        DispatchWindowEvent(WindowEvent(WindowEventType::RESIZE, pending_resize_width_, pending_resize_height_));
    }

    if (!document_ || !surface_) {
        return;
    }

    // =========================================================================
    // P0优化：按需渲染快速路径
    // 如果没有任何变化，直接跳过整个渲染流程
    // =========================================================================
    bool has_active_animations = false;
    if (animation_timeline_) {
        has_active_animations = animation_timeline_->HasRunningTransitions();
    }
    if (!has_active_animations && animation_controller_) {
        has_active_animations = !animation_controller_->GetRunningAnimations().empty();
    }
    
    // 快速路径：无需重绘且无活动动画时直接返回
    if (!needs_repaint_ && !has_active_animations && dirty_rects_.empty() && render_tree_valid_) {
        // 静态场景：完全跳过渲染
        static int skip_count = 0;
        if (++skip_count % 60 == 0) {
            std::cout << "[Render] Skipped " << skip_count << " frames (no changes)" << std::endl;
        }
        return;
    }

    // 更新动画
    static Uint64 start_time = SDL_GetPerformanceCounter();
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    double timestamp_sec = static_cast<double>(current_time - start_time) / frequency;
    UpdateAnimations(timestamp_sec);

    // 获取画布
    SkCanvas* canvas = surface_->getCanvas();
    if (!canvas) {
        return;
    }

    // 获取物理像素大小
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

    // 获取 DPI 缩放比
    float dpi_scale = GetDisplayScale();

    // 调试：检查 surface 大小是否与窗口大小一致
    if (surface_) {
        int surface_width = surface_->width();
        int surface_height = surface_->height();
        if (surface_width != physical_width || surface_height != physical_height) {
            std::cout << "[Window::Render] SIZE MISMATCH! surface=" << surface_width << "x" << surface_height
                      << ", window=" << physical_width << "x" << physical_height << std::endl;
        }
    }

    // 计算逻辑大小（CSS 像素）- 像浏览器一样
    int width = static_cast<int>(physical_width / dpi_scale);
    int height = static_cast<int>(physical_height / dpi_scale);

    // 检查 DevTools 是否打开，如果打开则调整主应用区域
    auto& devtools = DevToolsManager::GetInstance();
    float app_x = 0, app_y = 0;
    float app_width = static_cast<float>(width);
    float app_height = static_cast<float>(height);
    
    if (devtools.IsOpen()) {
        devtools.GetMainAppBounds(static_cast<float>(width), static_cast<float>(height),
                                   app_x, app_y, app_width, app_height);
    }

    // 设置视口尺寸（用于 body 元素滚动条计算）- 使用调整后的尺寸
    RenderObject::SetViewportSize(app_width, app_height);

    // Step 1: 构建或复用渲染树
    auto body = document_->GetBody();
    if (!body) {
        return;
    }

    // 检查窗口大小或 DevTools 状态是否改变（需要全量重建）
    static int last_width = 0, last_height = 0;
    static float last_app_width = 0, last_app_height = 0;
    bool size_changed = (width != last_width || height != last_height);
    bool app_size_changed = (app_width != last_app_width || app_height != last_app_height);
    
    std::cout << "[Window::Render] render_tree_valid_=" << render_tree_valid_ 
              << ", size_changed=" << size_changed
              << ", current=" << width << "x" << height
              << ", last=" << last_width << "x" << last_height << std::endl;
    
    if (size_changed || app_size_changed) {
        std::cout << "[Window::Render] Size changed: " << last_width << "x" << last_height 
                  << " -> " << width << "x" << height << std::endl;
        last_width = width;
        last_height = height;
        last_app_width = app_width;
        last_app_height = app_height;
        render_tree_valid_ = false;  // 窗口大小或 DevTools 状态改变，需要全量重建
    }

    if (!render_tree_valid_ || !cached_render_tree_) {
        // 在重建渲染树前，保存旧渲染树的滚动位置
        std::unordered_map<Node*, std::pair<float, float>> scroll_positions;
        if (cached_render_tree_) {
            SaveScrollPositions(cached_render_tree_.get(), scroll_positions);
        }

        // 渲染树无效，需要重建（全量渲染）
        DEBUG_LOG("[Window::Render] Full render - rebuilding render tree...");
        RenderTreeBuilder builder;
        builder.SetDocument(document_.get());
        cached_render_tree_ = builder.BuildRenderTree(body, nullptr);
        render_tree_valid_ = true;

        if (!cached_render_tree_) {
            DEBUG_LOG("[Window::Render] Failed to build render tree!");
            return;
        }

        // 恢复滚动位置
        if (!scroll_positions.empty()) {
            RestoreScrollPositions(cached_render_tree_.get(), scroll_positions);
        }

        // 新渲染树需要完整布局（使用调整后的尺寸）
        DEBUG_LOG("[Window::Render] Full layout: " << app_width << "x" << app_height);

        // 使用 Taffy 布局引擎计算布局
        Uint64 layout_start = SDL_GetTicks();
        if (layout_engine_) {
            layout_engine_->BuildLayoutTree(cached_render_tree_);
            layout_engine_->ComputeLayout(app_width, app_height);
            layout_engine_->GetLayoutInfo(cached_render_tree_);
        } else {
            // 降级到传统布局
            cached_render_tree_->Layout(app_width, app_height);
        }
        Uint64 layout_end = SDL_GetTicks();
        std::cout << "[Window::Render] Layout took " << (layout_end - layout_start) << "ms" << std::endl;

        // 获取 body 的背景色并清空画布
        SkColor clear_color = SK_ColorWHITE;
        const auto& body_style = cached_render_tree_->GetComputedStyle();
        if (!body_style.background_color.empty() && body_style.background_color != "transparent") {
            clear_color = Color::Parse(body_style.background_color);
        }
        canvas->clear(clear_color);

        // 应用 DPI 缩放到 canvas
        canvas->save();
        canvas->scale(dpi_scale, dpi_scale);

        // 如果 DevTools 打开，裁剪到主应用区域
        if (devtools.IsOpen()) {
            canvas->clipRect(SkRect::MakeXYWH(app_x, app_y, app_width, app_height));
        }

        // 绘制（使用逻辑坐标）
        DEBUG_LOG("[Window::Render] Mode A: Full rebuild and repaint");
        g_paint_total_calls = 0;
        g_paint_culled_calls = 0;
        RenderObject::ResetPaintTimingStats();
        Uint64 paint_start = SDL_GetTicks();
        cached_render_tree_->Paint(canvas);
        Uint64 paint_end = SDL_GetTicks();
        std::cout << "[Window::Render] Paint took " << (paint_end - paint_start) << "ms"
                  << ", total_calls=" << g_paint_total_calls.load()
                  << ", culled=" << g_paint_culled_calls.load() << std::endl;
        RenderObject::PrintPaintTimingStats();

        // 更新并绘制 select 下拉菜单（在所有内容之上）
        auto& dropdown_manager = SelectDropdownManager::Instance();
        if (dropdown_manager.IsDropdownOpen()) {
            dropdown_manager.UpdatePositionFromRenderTree(cached_render_tree_);
        }
        dropdown_manager.Paint(canvas);

        canvas->restore();

        // 全量渲染后，清除所有脏标记
        Uint64 clear_start = SDL_GetTicks();
        ClearDirtyFlags(body.get());
        ClearRenderObjectDirtyFlags(cached_render_tree_.get());
        Uint64 clear_end = SDL_GetTicks();
        std::cout << "[Window::Render] ClearFlags took " << (clear_end - clear_start) << "ms" << std::endl;
        
        // 全量渲染时，清除脏区域标记（SwapBuffers 将使用全量更新）
        has_dirty_bounds_ = false;
    } else {
        // 渲染树有效，尝试增量渲染

        // 获取背景色
        SkColor clear_color = SK_ColorWHITE;
        const auto& body_style = cached_render_tree_->GetComputedStyle();
        if (!body_style.background_color.empty() && body_style.background_color != "transparent") {
            clear_color = Color::Parse(body_style.background_color);
        }

        // 应用 DPI 缩放
        canvas->save();
        canvas->scale(dpi_scale, dpi_scale);

        // 如果 DevTools 打开，裁剪到主应用区域
        if (devtools.IsOpen()) {
            canvas->clipRect(SkRect::MakeXYWH(app_x, app_y, app_width, app_height));
        }

        // 调试输出控制（仅在设置环境变量时输出）
        static bool debug_render = std::getenv("LIGHTUI_DEBUG_RENDER") != nullptr;

        // 检查是否强制全屏重绘或禁用增量渲染
        if (force_full_repaint_ || !enable_incremental_render_) {
            // 模式 B：使用缓存的渲染树，但全屏重绘（不做局部裁剪）
            DEBUG_LOG("[Window::Render] Mode B: Full repaint (incremental disabled or forced)");
            if (debug_render) {
                std::cout << "[Render] Mode B: Full repaint (force=" << force_full_repaint_ 
                          << ", incremental=" << enable_incremental_render_ << ")" << std::endl;
            }

            // 增量布局：仅布局脏子树
            MarkRenderObjectsDirty(body.get(), cached_render_tree_.get());
            LayoutDirtySubtree(cached_render_tree_.get(), app_width, app_height);

            // 全屏重绘
            canvas->clear(clear_color);
            cached_render_tree_->Paint(canvas);
            
            // 全屏重绘时，清除脏区域标记（SwapBuffers 将使用全量更新）
            has_dirty_bounds_ = false;

            // 清除脏标记
            ClearDirtyFlags(body.get());
            ClearRenderObjectDirtyFlags(cached_render_tree_.get());
        } else {
            // 模式 C：基于脏区域的增量渲染
            DEBUG_LOG("[Window::Render] Mode C: Incremental rendering");

            // 步骤1：从渲染树收集脏区域（基于 RenderObject::NeedsPaint）
            // 这样可以捕获滚动、动画等仅修改 RenderObject 的更新
            CollectDirtyRectsFromRenderTree(cached_render_tree_.get());

            // 步骤2：收集最终的脏区域
            std::vector<SkRect> combined_dirty_rects;

            if (!dirty_rects_.empty()) {
                // 优先使用已收集的脏区域（包含 DOM 来源 + RenderObject 来源）
                // P2优化：使用自适应脏区域合并
                DirtyRegion dirty_region;
                for (const auto& rect : dirty_rects_) {
                    dirty_region.AddRect(rect);
                }
                dirty_region.OptimizeAdaptive(app_width, app_height);
                combined_dirty_rects = dirty_region.GetRegions();
            } else {
                // 回退：使用 DirtyRegionCollector 从 DOM 扫描
                DirtyRegionCollector collector;
                collector.SetViewportSize(static_cast<float>(width), static_cast<float>(height));
                DirtyRegion dirty_region;
                if (collector.CollectFromDOM(body.get(), dirty_region)) {
                    dirty_region.OptimizeAdaptive(app_width, app_height);
                    combined_dirty_rects = dirty_region.GetRegions();
                }
            }

            if (!combined_dirty_rects.empty()) {
                // 有脏区域：增量渲染
                DEBUG_LOG("[Window::Render] Mode C: Incremental rendering " << combined_dirty_rects.size() << " dirty rects");
                if (debug_render) {
                    std::cout << "[Render] Mode C: " << combined_dirty_rects.size() << " dirty rects" << std::endl;
                }

                // 同步 DOM 脏标记到 RenderObject 并执行增量布局
                MarkRenderObjectsDirty(body.get(), cached_render_tree_.get());
                LayoutDirtySubtree(cached_render_tree_.get(), app_width, app_height);

                // 关键修复：布局更新后，位置可能发生了变化。
                // 我们需要再次收集脏区域，以捕获元素的新位置。
                CollectDirtyRectsFromRenderTree(cached_render_tree_.get());
                
                // 更新 combined_dirty_rects
                if (!dirty_rects_.empty()) {
                    combined_dirty_rects = dirty_rects_;
                }

                // 局部绘制 - 恢复到之前正常工作的版本
                // 计算所有脏区域的边界（用于 CPU 模式局部更新）
                SkRect dirty_bounds = SkRect::MakeEmpty();
                
                for (const auto& rect : combined_dirty_rects) {
                    canvas->save();

                    // 裁剪到脏区域（扩大一点以包含边缘元素）
                    SkRect expanded_rect = rect.makeOutset(50, 50);
                    dirty_bounds.join(expanded_rect);
                    canvas->clipRect(expanded_rect);

                    // 清除扩大后的脏区域
                    SkPaint clear_paint;
                    clear_paint.setColor(clear_color);
                    canvas->drawRect(expanded_rect, clear_paint);

                    // 绘制整棵渲染树（会被裁剪到扩大后的脏区域）
                    cached_render_tree_->Paint(canvas);

                    canvas->restore();
                }
                
                // 保存脏区域边界（用于 SwapBuffers 的局部更新）
                // 注意：dirty_bounds 是逻辑坐标，需要转换为物理像素坐标
                last_dirty_bounds_ = SkRect::MakeLTRB(
                    dirty_bounds.left() * dpi_scale,
                    dirty_bounds.top() * dpi_scale,
                    dirty_bounds.right() * dpi_scale,
                    dirty_bounds.bottom() * dpi_scale
                );
                has_dirty_bounds_ = true;

                // 清除脏标记（DOM 和 RenderObject）
                ClearDirtyFlags(body.get());
                ClearRenderObjectDirtyFlags(cached_render_tree_.get());
            } else {
                // 无脏区域但需要重绘：全量绘制（如窗口被遮挡后恢复）
                DEBUG_LOG("[Window::Render] Full repaint (no dirty rects but needs_repaint)");
                canvas->clear(clear_color);
                cached_render_tree_->Paint(canvas);

                // 清除脏标记
                ClearRenderObjectDirtyFlags(cached_render_tree_.get());
            }
        }

        // 更新并绘制 select 下拉菜单
        auto& dropdown_manager = SelectDropdownManager::Instance();
        if (dropdown_manager.IsDropdownOpen()) {
            dropdown_manager.UpdatePositionFromRenderTree(cached_render_tree_);
        }
        dropdown_manager.Paint(canvas);

        canvas->restore();
    }

    // 渲染 DevTools（在所有内容之上）
    RenderDevTools(canvas, static_cast<float>(width), static_cast<float>(height));

    // 刷新
    if (gr_context_) {
        gr_context_->flush();
    }

    // 清除重绘标记和脏区域
    needs_repaint_ = false;
    dirty_rects_.clear();
    
    // 关键修复：重置强制全屏重绘标志
    // 这样下一帧可以恢复增量渲染模式，避免大窗口时的性能问题
    force_full_repaint_ = false;
}

void Window::RenderDevTools(SkCanvas* canvas, float width, float height) {
    auto& devtools = DevToolsManager::GetInstance();
    
    if (!devtools.IsOpen()) {
        return;
    }
    
    // 获取 DPI 缩放比
    float dpi_scale = GetDisplayScale();
    
    // 注意：传入的 width 和 height 已经是逻辑尺寸（CSS 像素）
    // 不需要再除以 dpi_scale
    
    // 获取主应用区域
    float app_x, app_y, app_width, app_height;
    devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);
    
    // 先渲染元素高亮覆盖层（在主应用区域内）
    canvas->save();
    canvas->scale(dpi_scale, dpi_scale);
    canvas->clipRect(SkRect::MakeXYWH(app_x, app_y, app_width, app_height));
    devtools.RenderHighlight(canvas);
    canvas->restore();
    
    // 再渲染 DevTools 面板
    canvas->save();
    canvas->scale(dpi_scale, dpi_scale);
    devtools.Render(canvas, width, height);
    canvas->restore();
}

// 辅助函数：规范化文本内容（与 RenderTreeBuilder::CreateRenderObjectForText 保持一致）
// 将连续的空白字符（空格、制表符、换行符）折叠为单个空格
static std::string NormalizeTextContent(const std::string& text_data, const ComputedStyle* parent_style) {
    // 检查父元素的 white-space 属性，决定是否保留换行符
    bool preserve_newlines = false;
    if (parent_style) {
        const std::string& ws = parent_style->white_space;
        preserve_newlines = (ws == "pre" || ws == "pre-wrap" || ws == "pre-line");
    }

    std::string final_text;
    if (preserve_newlines) {
        // 保留换行符，但根据 white-space 的不同处理空格/制表符
        const std::string& ws = parent_style ? parent_style->white_space : "normal";
        if (ws == "pre") {
            // 完全保留原始文本
            final_text = text_data;
        } else {
            // pre-wrap 或 pre-line: 保留换行，合并连续空格
            bool in_space = false;
            for (char c : text_data) {
                if (c == '\n') {
                    final_text += c;
                    in_space = false;
                } else if (c == ' ' || c == '\t' || c == '\r') {
                    if (!in_space) {
                        final_text += ' ';
                        in_space = true;
                    }
                } else {
                    final_text += c;
                    in_space = false;
                }
            }
        }
    } else {
        // 规范化空白字符：将连续的空白字符（包括换行）替换为单个空格
        bool in_whitespace = false;
        for (char c : text_data) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!in_whitespace) {
                    final_text += ' ';
                    in_whitespace = true;
                }
            } else {
                final_text += c;
                in_whitespace = false;
            }
        }
    }
    return final_text;
}

void Window::MarkRenderObjectsDirty(Node* dom_node, RenderObject* render_obj) {
    if (!dom_node || !render_obj) {
        return;
    }

    // 检查DOM节点是否有布局脏标记
    if (dom_node->IsLayoutDirty()) {
        render_obj->MarkNeedsLayout();
    }

    // 获取父样式（用于继承）
    const ComputedStyle* parent_style = nullptr;
    auto parent = render_obj->GetParent();
    if (parent) {
        parent_style = &parent->GetComputedStyle();
    }

    // 检查DOM节点是否有绘制脏标记或样式脏标记（包括伪类变化如:focus）
    if (dom_node->IsPaintDirty() || dom_node->IsStyleDirty()) {
        render_obj->MarkNeedsPaint();

        // 对于 Text 节点，需要同步更新 RenderText 的文本内容和样式
        if (dom_node->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = static_cast<Text*>(dom_node);
            auto render_text = dynamic_cast<RenderText*>(render_obj);
            if (text_node && render_text) {
                // 关键修复：使用规范化后的文本进行比较
                // RenderText 中存储的是规范化后的文本，所以比较时也需要规范化
                std::string raw_text = text_node->GetData();
                std::string normalized_text = NormalizeTextContent(raw_text, parent_style);
                if (render_text->GetText() != normalized_text) {
                    render_text->SetText(normalized_text);
                    render_obj->MarkNeedsLayout();  // 文本改变需要重新布局
                    
                    // Update content version for incremental layout optimization
                    // **Feature: incremental-layout-optimization**
                    // **Validates: Requirements 1.1**
                    if (layout_engine_) {
                        layout_engine_->UpdateContentVersion(render_obj);
                    }
                }

                // 更新 Text 节点的样式（从父元素继承可继承属性）
                if (parent_style) {
                    ComputedStyle text_style = render_obj->GetComputedStyle();
                    text_style.color = parent_style->color;
                    text_style.font_family = parent_style->font_family;
                    text_style.font_size = parent_style->font_size;
                    text_style.font_weight = parent_style->font_weight;
                    text_style.font_style = parent_style->font_style;
                    text_style.line_height = parent_style->line_height;
                    text_style.text_align = parent_style->text_align;
                    text_style.text_decoration = parent_style->text_decoration;
                    render_obj->SetComputedStyle(text_style);
                }
            }
        }

        // 重新计算 Element 样式（处理伪类变化如:focus, :hover等）
        if (dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
            // 使用 StyleResolver 重新计算样式
            StyleResolver resolver;
            if (document_ && document_->GetStyleManager()) {
                resolver.SetStyleManager(document_->GetStyleManager());
            }
            auto element = std::static_pointer_cast<Element>(dom_node->shared_from_this());
            auto new_style = resolver.ResolveStyle(element, parent_style);

            render_obj->SetComputedStyle(new_style);

            // 关键修复：通知布局引擎样式已更新
            // 这确保 left/top 等位置属性的变化能正确触发重新布局
            if (layout_engine_) {
                layout_engine_->UpdateStyle(render_obj, new_style);
            }
        }
    }

    // 递归处理子节点
    // 优化：使用哈希表加速查找，避免O(n²)复杂度
    const auto& dom_children = dom_node->GetChildNodes();
    const auto& render_children = render_obj->GetChildren();

    // 构建DOM节点指针到节点的映射（O(n)）
    std::unordered_map<Node*, std::shared_ptr<Node>> dom_map;
    dom_map.reserve(dom_children.size());
    for (const auto& dom_child : dom_children) {
        dom_map[dom_child.get()] = dom_child;
    }

    // 获取当前节点的新样式（用于子节点继承）
    const ComputedStyle* current_style = &render_obj->GetComputedStyle();

    // 遍历渲染子节点并查找对应的DOM节点（O(n)）
    for (const auto& render_child : render_children) {
        auto render_child_node = render_child->GetNode();
        if (!render_child_node) {
            continue;
        }

        // O(1)查找
        auto it = dom_map.find(render_child_node.get());
        if (it != dom_map.end()) {
            Node* child_dom_node = it->second.get();
            RenderObject* child_render_obj = render_child.get();

            // 如果当前节点样式改变，子节点的可继承样式也需要更新
            bool current_is_dirty = dom_node->IsPaintDirty() || dom_node->IsStyleDirty();
            if (current_is_dirty) {
                // 对于 Text 子节点，更新继承的样式
                if (child_dom_node->GetNodeType() == NodeType::TEXT_NODE) {
                    auto render_text = dynamic_cast<RenderText*>(child_render_obj);
                    if (render_text) {
                        // 同步文本内容
                        // 关键修复：使用规范化后的文本进行比较
                        auto text_node = static_cast<Text*>(child_dom_node);
                        std::string raw_text = text_node->GetData();
                        std::string normalized_text = NormalizeTextContent(raw_text, current_style);
                        if (render_text->GetText() != normalized_text) {
                            render_text->SetText(normalized_text);
                            child_render_obj->MarkNeedsLayout();
                            
                            // Update content version for incremental layout optimization
                            // **Feature: incremental-layout-optimization**
                            // **Validates: Requirements 1.1**
                            if (layout_engine_) {
                                layout_engine_->UpdateContentVersion(child_render_obj);
                            }
                        }

                        // 更新继承的样式
                        ComputedStyle text_style = child_render_obj->GetComputedStyle();
                        text_style.color = current_style->color;
                        text_style.font_family = current_style->font_family;
                        text_style.font_size = current_style->font_size;
                        text_style.font_weight = current_style->font_weight;
                        text_style.font_style = current_style->font_style;
                        text_style.line_height = current_style->line_height;
                        text_style.text_align = current_style->text_align;
                        text_style.text_decoration = current_style->text_decoration;
                        child_render_obj->SetComputedStyle(text_style);
                        child_render_obj->MarkNeedsPaint();
                    }
                }
                // 对于 Element 子节点，重新计算样式（会自动继承父样式）
                else if (child_dom_node->GetNodeType() == NodeType::ELEMENT_NODE) {
                    StyleResolver resolver;
                    if (document_ && document_->GetStyleManager()) {
                        resolver.SetStyleManager(document_->GetStyleManager());
                    }
                    auto child_element = std::static_pointer_cast<Element>(it->second);
                    auto new_style = resolver.ResolveStyle(child_element, current_style);
                    child_render_obj->SetComputedStyle(new_style);
                    child_render_obj->MarkNeedsPaint();

                    // 关键修复：通知布局引擎样式已更新
                    // 这确保 LayoutNode 的样式与 RenderObject 一致
                    if (layout_engine_) {
                        layout_engine_->UpdateStyle(child_render_obj, new_style);
                    }
                }
            }

            // 递归处理
            MarkRenderObjectsDirty(child_dom_node, child_render_obj);
        }
    }
}

bool Window::LayoutDirtySubtree(RenderObject* render_obj, float parent_width, float parent_height) {
    if (!render_obj) {
        return false;
    }

    // 优先使用 NativeLayoutEngine 的增量布局
    if (layout_engine_) {
        // 首先标记需要布局的 RenderObject
        std::function<void(RenderObject*)> markDirty = [&](RenderObject* obj) {
            if (!obj) return;
            if (obj->NeedsLayout()) {
                layout_engine_->MarkNeedsLayout(obj);
            }
            for (const auto& child : obj->GetChildren()) {
                markDirty(child.get());
            }
        };
        markDirty(render_obj);

        // 执行增量布局
        bool did_layout = layout_engine_->ComputeIncrementalLayout(parent_width, parent_height);

        if (did_layout) {
            // 更新 RenderObject 的布局信息
            layout_engine_->GetLayoutInfo(cached_render_tree_);
        }

        return did_layout;
    }

    // 回退到传统的递归布局
    bool needs_layout = render_obj->NeedsLayout();
    bool any_child_laid_out = false;

    // 检查子节点是否需要布局
    const auto& children = render_obj->GetChildren();
    for (const auto& child : children) {
        if (LayoutDirtySubtree(child.get(), parent_width, parent_height)) {
            any_child_laid_out = true;
            needs_layout = true;  // 子节点布局改变，父节点也需要重新布局
        }
    }

    // 如果当前节点或任何子节点需要布局，执行布局
    if (needs_layout) {
        render_obj->Layout(parent_width, parent_height);
        render_obj->ClearNeedsLayout();
        return true;
    }

    return false;
}

void Window::ClearDirtyFlags(Node* node) {
    if (!node) {
        return;
    }

    // 清除当前节点的脏标记
    node->ClearDirty(DirtyType::ALL);

    // 递归清除子节点
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        ClearDirtyFlags(child.get());
    }
}

void Window::ClearRenderObjectDirtyFlags(RenderObject* render_obj) {
    if (!render_obj) {
        return;
    }

    // 清除当前渲染对象的脏标记
    render_obj->ClearDirtyFlags();

    // 递归清除子节点
    const auto& children = render_obj->GetChildren();
    for (const auto& child : children) {
        ClearRenderObjectDirtyFlags(child.get());
    }
}

bool Window::HasDirtyLayoutNodes(Node* node) {
    if (!node) {
        return false;
    }

    // 检查当前节点是否需要布局
    if (node->IsLayoutDirty()) {
        return true;
    }

    // 检查关联的 RenderObject
    if (auto render_obj = node->GetRenderObject()) {
        if (render_obj->NeedsLayout()) {
            return true;
        }
    }

    // 递归检查子节点
    const auto& children = node->GetChildNodes();
    for (const auto& child : children) {
        if (HasDirtyLayoutNodes(child.get())) {
            return true;
        }
    }

    return false;
}

void Window::CollectDirtyRectsFromRenderTree(RenderObject* root) {
    if (!root) {
        return;
    }

    // 使用迭代方式代替递归，避免深层嵌套时栈溢出
    std::vector<RenderObject*> stack;
    stack.push_back(root);
    
    while (!stack.empty()) {
        RenderObject* obj = stack.back();
        stack.pop_back();
        
        if (!obj) {
            continue;
        }

        // 如果该渲染对象需要重绘，收集其边界框
        if (obj->NeedsPaint()) {
            SkRect rect = obj->GetBoundingRect();
            if (!rect.isEmpty()) {
                AddDirtyRect(rect);
            }
        }

        // 将子节点加入栈（逆序以保持遍历顺序）
        const auto& children = obj->GetChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            stack.push_back(it->get());
        }
    }
}

void Window::Clear(uint32_t color) {
    if (!surface_) {
        return;
    }

    SkCanvas* canvas = surface_->getCanvas();
    if (canvas) {
        // 将 uint32_t 转换为 SkColor (ARGB)
        SkColor sk_color = SkColorSetARGB(
            (color >> 24) & 0xFF,  // A
            (color >> 16) & 0xFF,  // R
            (color >> 8) & 0xFF,   // G
            color & 0xFF           // B
        );
        canvas->clear(sk_color);
    }
}

void Window::AddDirtyRect(const SkRect& rect) {
    // 忽略空矩形
    if (rect.isEmpty()) {
        return;
    }

    // 自动膨胀脏区域以容纳抗锯齿、子像素偏移和阴影
    SkRect inflated_rect = rect.makeOutset(2.0f, 2.0f);

    // 合并策略：如果新矩形与已有矩形重叠或距离较近，合并它们
    constexpr float kMergeThreshold = 10.0f;

    for (auto& existing : dirty_rects_) {
        // 扩展现有矩形检测重叠
        SkRect expanded = existing.makeOutset(kMergeThreshold, kMergeThreshold);
        if (expanded.intersects(inflated_rect)) {
            // 合并矩形
            existing.join(inflated_rect);
            return;
        }
    }

    // 没有重叠，添加新矩形
    dirty_rects_.push_back(inflated_rect);

    // 如果脏区域太多，尝试合并相邻的区域
    constexpr size_t kMaxDirtyRects = 100;  // 提高阈值
    if (dirty_rects_.size() > kMaxDirtyRects) {
        // 策略：找到最近的两个矩形并合并它们
        // 重复这个过程直到数量降到阈值以下
        while (dirty_rects_.size() > kMaxDirtyRects) {
            float min_distance = std::numeric_limits<float>::max();
            size_t merge_i = 0, merge_j = 1;
            
            // 找到距离最近的两个矩形
            for (size_t i = 0; i < dirty_rects_.size(); ++i) {
                for (size_t j = i + 1; j < dirty_rects_.size(); ++j) {
                    const auto& r1 = dirty_rects_[i];
                    const auto& r2 = dirty_rects_[j];
                    
                    // 计算两个矩形中心点的距离
                    float cx1 = (r1.left() + r1.right()) / 2.0f;
                    float cy1 = (r1.top() + r1.bottom()) / 2.0f;
                    float cx2 = (r2.left() + r2.right()) / 2.0f;
                    float cy2 = (r2.top() + r2.bottom()) / 2.0f;
                    
                    float dx = cx2 - cx1;
                    float dy = cy2 - cy1;
                    float distance = dx * dx + dy * dy;  // 不需要开方，比较大小即可
                    
                    if (distance < min_distance) {
                        min_distance = distance;
                        merge_i = i;
                        merge_j = j;
                    }
                }
            }
            
            // 合并最近的两个矩形
            dirty_rects_[merge_i].join(dirty_rects_[merge_j]);
            dirty_rects_.erase(dirty_rects_.begin() + merge_j);
        }
        
        std::cout << "[AddDirtyRect] Merged nearby rects, now have " << dirty_rects_.size() << " rects" << std::endl;
    }
}

void Window::UpdateAnimations(double current_time) {
    // 更新 CSS Transition 动画
    bool has_active_animations = false;
    if (animation_timeline_) {
        animation_timeline_->Update(current_time);
        // TODO: 检查是否有活跃的动画
    }

    // 更新 CSS Animation 动画
    if (animation_controller_) {
        animation_controller_->Update(current_time);
        // TODO: 检查是否有活跃的动画
    }

    // 只在有活跃动画时才标记需要重绘
    // 注意：不要在这里无条件调用 SetNeedsRepaint()，否则会导致无限重绘循环
    (void)has_active_animations;
}

float Window::GetDisplayScale() const {
    if (!sdl_window_) {
        return 1.0f;
    }

    // 方法 1: 尝试使用 SDL_GetWindowDisplayScale (SDL3)
    float scale = SDL_GetWindowDisplayScale(sdl_window_);
    if (scale > 1.0f) {
        return scale;
    }

    // 方法 2: 通过物理像素和逻辑像素的比值计算
    int logical_width, logical_height;
    SDL_GetWindowSize(sdl_window_, &logical_width, &logical_height);

    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);

    if (logical_width > 0 && physical_width != logical_width) {
        float calculated_scale = static_cast<float>(physical_width) / static_cast<float>(logical_width);
        if (calculated_scale > 1.0f) {
            return calculated_scale;
        }
    }

    // 方法 3: 使用 SDL_GetDisplayContentScale
    SDL_DisplayID display_id = SDL_GetDisplayForWindow(sdl_window_);
    if (display_id != 0) {
        float content_scale = SDL_GetDisplayContentScale(display_id);
        if (content_scale > 1.0f) {
            return content_scale;
        }
    }

    return 1.0f;
}

void Window::EnsureRenderTree() {
    // 减少日志输出
    // std::cout << "[EnsureRenderTree] Called, render_tree_valid_=" << render_tree_valid_ << std::endl;
    
    if (render_tree_valid_ && cached_render_tree_) {
        // std::cout << "[EnsureRenderTree] Tree already valid, skipping" << std::endl;
        return;  // 渲染树已经有效
    }

    if (!document_) {
        return;
    }

    auto body = document_->GetBody();
    if (!body) {
        return;
    }

    // 在重建渲染树前，保存旧渲染树的滚动位置
    std::unordered_map<Node*, std::pair<float, float>> scroll_positions;
    if (cached_render_tree_) {
        SaveScrollPositions(cached_render_tree_.get(), scroll_positions);
    }

    // 构建渲染树
    if (!render_tree_builder_) {
        render_tree_builder_ = std::make_shared<RenderTreeBuilder>();
    }
    render_tree_builder_->SetDocument(document_.get());
    cached_render_tree_ = render_tree_builder_->BuildRenderTree(body, nullptr);

    if (!cached_render_tree_) {
        return;
    }

    // Phase 3: 初始化渲染树增量更新器
    if (!render_tree_updater_) {
        render_tree_updater_ = std::make_unique<RenderTreeUpdater>();
    }
    render_tree_updater_->SetDocument(document_);
    render_tree_updater_->SetRenderTreeBuilder(render_tree_builder_);
    if (layout_engine_) {
        render_tree_updater_->SetLayoutEngine(std::shared_ptr<LayoutEngine>(
            layout_engine_.get(), [](LayoutEngine*) {}));  // 非拥有指针
    }

    // 恢复滚动位置
    if (!scroll_positions.empty()) {
        RestoreScrollPositions(cached_render_tree_.get(), scroll_positions);
    }

    // 获取窗口尺寸
    int physical_width, physical_height;
    SDL_GetWindowSizeInPixels(sdl_window_, &physical_width, &physical_height);
    
    // 获取 DPI 缩放比
    float dpi_scale = GetDisplayScale();
    
    // 计算逻辑大小
    float width = static_cast<float>(physical_width) / dpi_scale;
    float height = static_cast<float>(physical_height) / dpi_scale;
    
    // 检查 DevTools 是否打开，如果打开则调整主应用区域
    auto& devtools = DevToolsManager::GetInstance();
    float app_width = width;
    float app_height = height;
    
    if (devtools.IsOpen()) {
        float app_x, app_y;
        devtools.GetMainAppBounds(width, height, app_x, app_y, app_width, app_height);
    }
    
    // 设置视口尺寸
    RenderObject::SetViewportSize(app_width, app_height);

    // 使用 Taffy 布局引擎计算布局（使用调整后的尺寸）
    if (layout_engine_) {
        layout_engine_->BuildLayoutTree(cached_render_tree_);
        layout_engine_->ComputeLayout(app_width, app_height);
        layout_engine_->GetLayoutInfo(cached_render_tree_);
    } else {
        cached_render_tree_->Layout(app_width, app_height);
    }

    render_tree_valid_ = true;
}

void Window::SaveScrollPositions(RenderObject* render_obj,
                                  std::unordered_map<Node*, std::pair<float, float>>& scroll_positions) {
    if (!render_obj) {
        return;
    }

    // 如果有滚动偏移，保存它
    float scroll_x = render_obj->GetScrollX();
    float scroll_y = render_obj->GetScrollY();
    if (scroll_x != 0.0f || scroll_y != 0.0f) {
        auto node = render_obj->GetNode();
        if (node) {
            scroll_positions[node.get()] = {scroll_x, scroll_y};
        }
    }

    // 递归处理子节点
    for (const auto& child : render_obj->GetChildren()) {
        SaveScrollPositions(child.get(), scroll_positions);
    }
}

void Window::RestoreScrollPositions(RenderObject* render_obj,
                                     const std::unordered_map<Node*, std::pair<float, float>>& scroll_positions) {
    if (!render_obj) {
        return;
    }

    // 查找是否有保存的滚动位置
    auto node = render_obj->GetNode();
    if (node) {
        auto it = scroll_positions.find(node.get());
        if (it != scroll_positions.end()) {
            render_obj->SetScrollX(it->second.first);
            render_obj->SetScrollY(it->second.second);
        }
    }

    // 递归处理子节点
    for (const auto& child : render_obj->GetChildren()) {
        RestoreScrollPositions(child.get(), scroll_positions);
    }
}

} // namespace lightui


