#include "ui_dev_control.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <nlohmann/json.hpp>

#include "core/dom/document.h"
#include "core/dom/elements/html_select_element.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/render/objects/select_dropdown.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/utils/encoding_utils.h"
#include "core/window/window.h"
#include "ui_dev_snapshot.h"

namespace mblink::ui_dev {
namespace {
namespace fs = std::filesystem;

fs::path Utf8PathToFsPathLocal(const std::string& path) {
#ifdef _WIN32
    return fs::path(utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string NormalizeFsPathLocal(const fs::path& path) { return path.lexically_normal().generic_string(); }

void ClearBody(Document* document) {
    if (!document) return;
    auto body = document->GetBody();
    if (!body) return;
    while (body->GetFirstChild()) body->RemoveChild(body->GetFirstChild());
}

std::string NewRuntimeEpoch() {
    const auto tick = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return "rt-" + std::to_string(tick);
}

std::string JsonLiteral(const nlohmann::json& value) { return value.dump(); }

class DocumentBatchScope {
public:
    explicit DocumentBatchScope(Document* document) : document_(document) {
        if (document_) {
            document_->BeginBatch();
        }
    }

    ~DocumentBatchScope() {
        if (document_) {
            document_->EndBatch();
        }
    }

    DocumentBatchScope(const DocumentBatchScope&) = delete;
    DocumentBatchScope& operator=(const DocumentBatchScope&) = delete;

private:
    Document* document_;
};

void ForceWindowFrame(QuickJSRuntime* runtime, Window* window, int passes = 2) {
    if (!window) return;
    if (passes < 1) passes = 1;
    for (int i = 0; i < passes; ++i) {
        if (runtime) {
            runtime->RunEventLoop(1);
            runtime->ProcessMicrotasks();
        }
        if (auto* pipeline = window->GetRenderPipeline()) {
            pipeline->ForceRasterize();
        }
        window->SetNeedsRepaint();
        window->Render();
        window->SwapBuffers();
        if (runtime) {
            runtime->RunEventLoop(1);
            runtime->ProcessMicrotasks();
        }
    }
}

bool ExportSnapshotFromFreshFrame(QuickJSRuntime* runtime,
                                  Window* window,
                                  Document* document,
                                  const std::string& snapshot_file,
                                  const SnapshotExportOptions& options,
                                  std::string* error) {
    if (!window) {
        if (error) *error = "window is null";
        return false;
    }
    if (runtime) {
        runtime->RunEventLoop(1);
        runtime->ProcessMicrotasks();
    }
    if (auto* pipeline = window->GetRenderPipeline()) {
        pipeline->ForceRasterize();
    }
    window->SetNeedsRepaint();
    window->Render();

    const bool ok = ExportUiDevSnapshot(window, document, snapshot_file, options, error);
    window->SwapBuffers();
    return ok;
}

bool WriteJsonFileAtomic(const fs::path& path, const nlohmann::json& value) {
    const auto parent = path.parent_path();
    if (!parent.empty()) fs::create_directories(parent);
    const auto tmp_path = path.string() + ".tmp";
    std::ofstream ofs(tmp_path, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << value.dump(2);
    ofs.close();
    if (!ofs.good()) return false;

    std::error_code ec;
    fs::rename(tmp_path, path, ec);
    if (ec) {
        fs::remove(path, ec);
        ec.clear();
        fs::rename(tmp_path, path, ec);
    }
    return !ec;
}

std::string BuildUiDevDomScript(const std::string& type, const nlohmann::json& cmd, bool perform_action = true) {
    const std::string selector = JsonLiteral(cmd.value("selector", std::string{}));
    std::ostringstream js;
    js << R"JS((() => {
const selector = )JS" << selector << R"JS(;
const performAction = )JS" << (perform_action ? "true" : "false") << R"JS(;
const q = () => {
  if (selector === 'body') return document.body || document.querySelector('body');
  if (selector === 'html') return document.documentElement || document.querySelector('html');
  return document.querySelector(selector);
};
const stalePostAction = () => ({ selector, post_action_found: false, post_action_stale: true });
const attrsOf = (el) => { const out = {}; const attrs = el && el.attributes ? el.attributes : []; const len = attrs.length || 0; for (let i = 0; i < len; ++i) { const a = attrs[i]; if (a && a.name) out[a.name] = a.value === undefined ? '' : String(a.value); } return out; };
const rectOf = (el) => { const r = el.getBoundingClientRect(); return { x: r.x, y: r.y, w: r.width, h: r.height, top: r.top, right: r.right, bottom: r.bottom, left: r.left }; };
const lightRectOf = () => ({ x: 0, y: 0, w: 0, h: 0, top: 0, right: 0, bottom: 0, left: 0 });
const scrollOf = (el) => { const sw = Number(el.scrollWidth || 0); const sh = Number(el.scrollHeight || 0); const cw = Number(el.clientWidth || 0); const ch = Number(el.clientHeight || 0); return { x: Number(el.scrollLeft || 0), y: Number(el.scrollTop || 0), max_x: cw > 0 ? Math.max(0, sw - cw) : 0, max_y: ch > 0 ? Math.max(0, sh - ch) : 0 }; };
const visibleOf = (el) => { const s = getComputedStyle(el); const r = el.getBoundingClientRect(); return s.display !== 'none' && s.visibility !== 'hidden' && Number(s.opacity || 1) !== 0 && r.width >= 0 && r.height >= 0; };
const interactiveOf = (el) => { const t = ((el.tagName || '') + '').toLowerCase(); return !!(el.onclick || t === 'button' || t === 'input' || t === 'select' || t === 'textarea' || t === 'option' || t === 'a'); };
const textPreviewOf = (el, full) => { const raw = el.textContent == null ? null : String(el.textContent); if (raw == null) return null; return full || raw.length <= 160 ? raw : raw.slice(0, 160); };
const summaryOf = (el, fullText, light) => { const text = textPreviewOf(el, !!fullText); return { tag: ((el.tagName || '') + '').toLowerCase(), id: el.id || '', class_name: el.className || '', text, text_length: text == null ? 0 : text.length, attrs: attrsOf(el), rect: light ? lightRectOf() : rectOf(el), interactive: interactiveOf(el), visible: light ? true : visibleOf(el), value: el.value === undefined ? null : el.value, scroll: light ? { x: 0, y: 0, max_x: 0, max_y: 0 } : scrollOf(el) }; };
const pickStyle = (el) => { const s = getComputedStyle(el); return { display: s.display || '', position: s.position || '', width: s.width || '', height: s.height || '', color: s.color || '', background_color: s.backgroundColor || '', opacity: s.opacity || '', overflow_x: s.overflowX || '', overflow_y: s.overflowY || '', z_index: s.zIndex || '', flex: s.flex || '' }; };
const dispatchSimple = (el, type) => { if (typeof Event !== 'function') return false; try { el.dispatchEvent(new Event(type)); return true; } catch (_) { return false; } };
const devClick = (el) => {
  const tag = ((el.tagName || '') + '').toLowerCase();
  const inputType = tag === 'input' ? (((el.type || el.getAttribute('type') || '') + '').toLowerCase()) : '';
  const isCheckable = inputType === 'checkbox' || inputType === 'radio';
  const beforeChecked = isCheckable ? !!el.checked : false;
  if (el.click) el.click();
  if (isCheckable && !!el.checked === beforeChecked) {
    if (inputType === 'checkbox') {
      el.checked = !beforeChecked;
    } else if (!beforeChecked) {
      el.checked = true;
    }
    if (!!el.checked !== beforeChecked) {
      dispatchSimple(el, 'input');
      dispatchSimple(el, 'change');
    }
  }
};
)JS";
    if (type == "query_element") {
        js << "const limit = Math.max(0, Number(" << JsonLiteral(cmd.value("limit", 50)) << " || 50)); const nodes = selector === 'body' && document.body ? [document.body] : selector === 'html' && document.documentElement ? [document.documentElement] : document.querySelectorAll(selector); const matches = []; const n = Math.min(nodes.length, limit); for (let i = 0; i < n; ++i) matches.push(summaryOf(nodes[i], false, false)); return { selector, count: nodes.length, returned: matches.length, truncated: nodes.length > matches.length, limit, matches };";
    } else if (type == "inspect") {
        js << "const el = q(); if (!el) throw new Error('element not found: ' + selector); return { selector, found: true, element: summaryOf(el, true), computed_style: pickStyle(el), outer_html: el.outerHTML === undefined ? null : String(el.outerHTML) };";
    } else if (type == "click") {
        js << "const el = q(); if (!el) { if (!performAction) return stalePostAction(); throw new Error('element not found: ' + selector); } if (performAction) { devClick(el); const latest = q(); if (!latest) return { selector, clicked: true, post_action_found: false, post_action_stale: true }; return { selector, clicked: true, element: summaryOf(latest, false), post_action_found: true }; } return { selector, clicked: true, element: summaryOf(el, false), post_action_found: true };";
    } else if (type == "input_text") {
        js << "const el = q(); if (!el) { if (!performAction) return stalePostAction(); throw new Error('element not found: ' + selector); } if (el.value === undefined) throw new Error('target does not support value: ' + selector); if (performAction) { const nextValue = " << JsonLiteral(cmd.value("text", std::string{})) << "; if (el.focus) el.focus(); el.value = nextValue; if (typeof Event === 'function') { try { el.dispatchEvent(new Event('input')); } catch (_) {} try { el.dispatchEvent(new Event('change')); } catch (_) {} } const latest = q(); if (!latest) return { selector, value: null, post_action_found: false, post_action_stale: true }; return { selector, value: latest.value === undefined ? null : latest.value, element: summaryOf(latest, false), post_action_found: true }; } return { selector, value: el.value === undefined ? null : el.value, element: summaryOf(el, false), post_action_found: true };";
    } else if (type == "scroll") {
        js << "const el = q(); if (!el) { if (!performAction) return stalePostAction(); throw new Error('element not found: ' + selector); } const x = " << (cmd.contains("x") ? JsonLiteral(cmd["x"]) : std::string("null")) << "; const y = " << (cmd.contains("y") ? JsonLiteral(cmd["y"]) : std::string("null")) << "; if (performAction) { if (x !== null) el.scrollLeft = x; if (y !== null) el.scrollTop = y; const latest = q(); if (!latest) return { selector, scroll: null, post_action_found: false, post_action_stale: true }; return { selector, scroll: scrollOf(latest), element: summaryOf(latest, false), post_action_found: true }; } return { selector, scroll: scrollOf(el), element: summaryOf(el, false), post_action_found: true };";
    } else if (type == "highlight") {
        js << "const el = q(); if (!el) { if (!performAction) return stalePostAction(); throw new Error('element not found: ' + selector); } const color = " << JsonLiteral(cmd.value("color", std::string("#ff4d4f"))) << "; const previous = el.style && el.style.outline ? String(el.style.outline) : ''; if (performAction) { if (el.style) el.style.outline = '2px solid ' + color; const latest = q(); if (!latest) return { selector, highlighted: true, color, previous_outline: previous, post_action_found: false, post_action_stale: true }; return { selector, highlighted: true, color, previous_outline: previous, element: summaryOf(latest, false), post_action_found: true }; } return { selector, highlighted: true, color, element: summaryOf(el, false), post_action_found: true };";
    }
    js << R"JS(
})())JS";
    return js.str();
}

std::shared_ptr<Element> QueryUiDevElement(Document* document, const std::string& selector) {
    if (!document || selector.empty()) return nullptr;
    if (selector == "body") return document->GetBody();
    if (selector == "html") return document->GetDocumentElement();
    auto body = document->GetBody();
    return body ? body->QuerySelector(selector) : nullptr;
}

nlohmann::json RectToJson(const SkRect& rect) {
    return nlohmann::json{{"x", rect.x()},
                          {"y", rect.y()},
                          {"w", rect.width()},
                          {"h", rect.height()},
                          {"top", rect.top()},
                          {"right", rect.right()},
                          {"bottom", rect.bottom()},
                          {"left", rect.left()}};
}

bool OpenSelectDropdownForUiDev(const std::shared_ptr<Element>& element) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(element);
    if (!select || select->GetDisabled()) return false;

    auto& dropdown_manager = SelectDropdownManager::Instance();
    if (select->IsDropdownOpen()) {
        dropdown_manager.CloseDropdown();
        return true;
    }

    auto rect = select->GetBoundingClientRect();
    if (rect.width <= 0.0f || rect.height <= 0.0f) return false;

    select->SetDropdownOpen(true);
    dropdown_manager.OpenDropdown(select, SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height));
    return true;
}

void AddDropdownState(nlohmann::json& result) {
    auto& dropdown_manager = SelectDropdownManager::Instance();
    result["dropdown_open"] = dropdown_manager.IsDropdownOpen();
    if (dropdown_manager.IsDropdownOpen()) {
        result["dropdown_rect"] = RectToJson(dropdown_manager.GetDropdownRect());
    }
}

}  // namespace

bool TryHandleUiDevCommand(QuickJSRuntime* runtime,
                           Window* window,
                           Document* document,
                           const std::string& command_path,
                           const std::string& response_path,
                           std::string* last_command_id,
                           std::string* runtime_epoch,
                           const std::shared_ptr<std::atomic<bool>>& shutdown_requested,
                           bool* handled,
                           std::string* error) {
    if (handled) *handled = false;
    if (!runtime || !window || !document || command_path.empty() || response_path.empty() || !last_command_id) return false;
    if (shutdown_requested && shutdown_requested->load()) return false;
    fs::path command_fs_path(command_path);
    if (!fs::exists(command_fs_path)) return false;

    nlohmann::json cmd;
    try {
        std::ifstream ifs(command_fs_path, std::ios::binary);
        if (!ifs) return false;
        cmd = nlohmann::json::parse(ifs);
    } catch (...) {
        std::error_code ec;
        fs::remove(command_fs_path, ec);
        if (error) *error = "ui-dev command parse failed";
        return false;
    }

    const std::string id = cmd.value("id", "");
    if (id.empty() || id == *last_command_id) return false;

    nlohmann::json resp{{"ok", true}, {"id", id}, {"type", cmd.value("type", "")}};
    if (runtime_epoch) resp["runtime_epoch"] = *runtime_epoch;
    const std::string type = resp.value("type", "");
    try {
        if (shutdown_requested && shutdown_requested->load()) {
            resp["ok"] = false;
            resp["error"] = nlohmann::json{{"code", "shutdown_in_progress"}, {"message", "runtime is shutting down"}};
        } else if (type == "eval") {
            DocumentBatchScope batch_scope(document);
            resp["result"] = runtime->Eval(cmd.value("code", ""), "<mblink-ui-dev eval>");
        } else if (type == "reload_bundle") {
            const std::string bundle_path = cmd.value("bundle_path", "");
            if (bundle_path.empty()) throw std::runtime_error("reload_bundle missing bundle_path");
            fs::path abs_path = fs::absolute(Utf8PathToFsPathLocal(bundle_path));
            std::ifstream ifs(abs_path, std::ios::binary);
            if (!ifs) throw std::runtime_error("failed to open bundle: " + NormalizeFsPathLocal(abs_path));
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            const std::string bundle_code = buffer.str();
            const std::string normalized_path = NormalizeFsPathLocal(abs_path);
            ClearBody(document);
            runtime->SetBaseModulePath(normalized_path);
            const std::string next_epoch = NewRuntimeEpoch();
            {
                DocumentBatchScope batch_scope(document);
                resp["result"] = runtime->EvalModule(bundle_code, normalized_path + "#reload-" + next_epoch);
            }
            if (runtime_epoch) {
                *runtime_epoch = next_epoch;
                resp["runtime_epoch"] = *runtime_epoch;
            }
            ForceWindowFrame(runtime, window);
            resp["bundle_path"] = normalized_path;
            resp["mode"] = "reload_bundle";
        } else if (type == "snapshot") {
            const std::string snapshot_file = cmd.value("snapshot_file", "");
            if (snapshot_file.empty()) throw std::runtime_error("snapshot missing snapshot_file");
            SnapshotExportOptions options;
            options.runtime_epoch = runtime_epoch ? *runtime_epoch : std::string{};
            const int max_nodes = cmd.value("max_nodes", 2000);
            const int max_depth = cmd.value("max_depth", 64);
            options.max_nodes = max_nodes > 0 ? static_cast<size_t>(max_nodes) : 2000;
            options.max_depth = max_depth > 0 ? max_depth : 64;
            options.root_selector = cmd.value("root_selector", std::string{});
            options.include_screenshot = cmd.value("include_screenshot", false);
            options.inline_screenshot = cmd.value("inline_screenshot", false);
            options.screenshot_path = cmd.value("screenshot_file", std::string{});
            options.shutdown_requested = shutdown_requested;
            std::string snapshot_error;
            if (!ExportSnapshotFromFreshFrame(runtime, window, document, snapshot_file, options, &snapshot_error)) {
                throw std::runtime_error(snapshot_error.empty() ? "snapshot export failed" : snapshot_error);
            }
            resp["snapshot_file"] = snapshot_file;
            resp["include_screenshot"] = options.include_screenshot;
            resp["inline_screenshot"] = options.inline_screenshot;
            if (!options.screenshot_path.empty()) resp["screenshot_file"] = options.screenshot_path;
            resp["root_selector"] = options.root_selector;
            resp["max_nodes"] = options.max_nodes;
            resp["max_depth"] = options.max_depth;
        } else if (type == "query_element" || type == "inspect" || type == "click" || type == "input_text" || type == "scroll" || type == "highlight") {
            const auto selector = cmd.value("selector", std::string{});
            if (selector.empty()) throw std::runtime_error(type + " missing selector");
            const auto target_element = QueryUiDevElement(document, selector);
            const bool native_select_click =
                type == "click" &&
                target_element &&
                target_element->GetTagName() == "select" &&
                cmd.value("native", true);
            if (native_select_click) {
                if (!OpenSelectDropdownForUiDev(target_element)) {
                    throw std::runtime_error("failed to open select dropdown: " + selector);
                }
                resp["result"] = nlohmann::json{{"selector", selector},
                                                {"clicked", true},
                                                {"native_select", true},
                                                {"post_action_found", true}};
                ForceWindowFrame(runtime, window, 1);
                AddDropdownState(resp["result"]);
            } else
            {
                DocumentBatchScope batch_scope(document);
                resp["result"] = runtime->Eval(BuildUiDevDomScript(type, cmd), "<mblink-ui-dev dom>");
            }
            if (!native_select_click &&
                (type == "click" || type == "input_text" || type == "scroll" || type == "highlight")) {
                ForceWindowFrame(runtime, window, 1);
                auto latest = runtime->Eval(BuildUiDevDomScript(type, cmd, false), "<mblink-ui-dev dom result>");
                if (resp["result"].is_object() && latest.is_object()) {
                    for (auto it = latest.begin(); it != latest.end(); ++it) resp["result"][it.key()] = it.value();
                } else {
                    resp["result"] = latest;
                }
            }
        } else {
            resp["ok"] = false;
            resp["error"] = nlohmann::json{{"code", "unknown_command"}, {"message", "unknown ui-dev command"}};
        }
    } catch (const std::exception& e) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", type.empty() ? "ui_dev_command_failed" : type + "_failed"}, {"message", e.what()}};
    } catch (...) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", type.empty() ? "ui_dev_command_failed" : type + "_failed"}, {"message", "unknown exception"}};
    }

    try {
        if (!WriteJsonFileAtomic(fs::path(response_path), resp)) {
            if (error) *error = "write ui-dev response failed";
            return false;
        }
    } catch (...) {
        if (error) *error = "write ui-dev response failed";
        return false;
    }

    {
        std::error_code ec;
        fs::remove(command_fs_path, ec);
    }
    *last_command_id = id;
    if (handled) *handled = true;
    return true;
}

}  // namespace mblink::ui_dev
