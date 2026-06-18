#include "ui_dev_snapshot.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
#include "core/render/objects/render_object.h"
#include "core/window/window.h"

namespace mbink::ui_dev {

namespace {

std::string NodeId(const std::shared_ptr<mbink::Node>& node) {
    std::ostringstream oss;
    oss << "node@" << node.get();
    return oss.str();
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool IsInteractiveTag(const std::string& tag) {
    static const std::unordered_set<std::string> tags = {
        "a", "button", "input", "select", "textarea", "label", "summary", "option"
    };
    return tags.count(tag) > 0;
}

bool HasInteractiveAttributes(const std::shared_ptr<mbink::Element>& element) {
    if (!element) return false;
    if (element->HasAnyEventListeners()) return true;

    const auto& attrs = element->GetAllAttributes();
    return attrs.count("onclick") > 0 ||
           attrs.count("onmousedown") > 0 ||
           attrs.count("onmouseup") > 0 ||
           attrs.count("onpointerdown") > 0 ||
           attrs.count("onpointerup") > 0 ||
           attrs.count("href") > 0 ||
           attrs.count("tabindex") > 0 ||
           attrs.count("contenteditable") > 0 ||
           attrs.count("role") > 0;
}

bool ShouldSkipElementTag(const std::string& tag) {
    static const std::unordered_set<std::string> skipped_tags = {"script"};
    return skipped_tags.count(tag) > 0;
}

std::shared_ptr<mbink::Node> ResolveSnapshotRoot(mbink::Document* document,
                                                 const std::string& root_selector,
                                                 std::string* error) {
    auto body = document ? document->GetBody() : nullptr;
    if (!root_selector.empty()) {
        std::shared_ptr<mbink::Element> selected;
        if (root_selector == "body") {
            selected = body;
        } else if (root_selector == "html") {
            selected = document->GetDocumentElement();
        } else if (body) {
            selected = body->QuerySelector(root_selector);
        }
        if (!selected) {
            if (error) *error = "root_selector_not_found";
            return nullptr;
        }
        return std::static_pointer_cast<mbink::Node>(selected);
    }
    return body ? std::static_pointer_cast<mbink::Node>(body)
                : std::static_pointer_cast<mbink::Node>(document->GetDocumentElement());
}

nlohmann::json CachedElementRect(const std::shared_ptr<mbink::Element>& element) {
    auto render_object = element ? element->GetRenderObject() : nullptr;
    if (!render_object) {
        return nlohmann::json{{"x", 0}, {"y", 0}, {"w", 0}, {"h", 0}};
    }

    const auto rect = render_object->GetViewportBoundingRect();
    return nlohmann::json{{"x", rect.x()}, {"y", rect.y()}, {"w", rect.width()}, {"h", rect.height()}};
}

nlohmann::json ElementScrollState(const std::shared_ptr<mbink::Element>& element) {
    auto render_object = element ? element->GetRenderObject() : nullptr;
    if (!render_object) {
        return nlohmann::json{{"x", 0}, {"y", 0}, {"max_x", 0}, {"max_y", 0}};
    }

    return nlohmann::json{{"x", render_object->GetScrollX()},
                          {"y", render_object->GetScrollY()},
                          {"max_x", render_object->GetMaxScrollX()},
                          {"max_y", render_object->GetMaxScrollY()}};
}

struct SnapshotTraversalState {
    size_t max_nodes = 2000;
    int max_depth = 64;
    size_t node_count = 0;
    bool truncated = false;
    std::string truncated_reason;
    std::shared_ptr<std::atomic<bool>> shutdown_requested;
};

bool ShouldAbort(const SnapshotTraversalState& state) {
    return state.shutdown_requested && state.shutdown_requested->load();
}

std::string Base64Encode(const std::vector<uint8_t>& bytes) {
    static constexpr char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((bytes.size() + 2) / 3) * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        const uint8_t b0 = bytes[i];
        const uint8_t b1 = i + 1 < bytes.size() ? bytes[i + 1] : 0;
        const uint8_t b2 = i + 2 < bytes.size() ? bytes[i + 2] : 0;
        output.push_back(kTable[(b0 >> 2) & 0x3F]);
        output.push_back(kTable[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)]);
        output.push_back(i + 1 < bytes.size() ? kTable[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=');
        output.push_back(i + 2 < bytes.size() ? kTable[b2 & 0x3F] : '=');
    }
    return output;
}

std::filesystem::path DefaultScreenshotPathForSnapshot(const std::filesystem::path& snapshot_path) {
    auto screenshot_path = snapshot_path;
    if (screenshot_path.has_extension()) {
        screenshot_path.replace_extension(".png");
    } else {
        screenshot_path += ".png";
    }
    return screenshot_path;
}

bool WriteFileAtomic(const std::filesystem::path& path,
                     const char* data,
                     size_t size,
                     std::string* error) {
    try {
        const auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        const auto tmp_path = path.string() + ".tmp";
        std::ofstream ofs(tmp_path, std::ios::binary | std::ios::trunc);
        if (size > 0) ofs.write(data, static_cast<std::streamsize>(size));
        ofs.close();
        if (!ofs.good()) {
            if (error) *error = "write_file_failed";
            return false;
        }

        std::error_code ec;
        std::filesystem::rename(tmp_path, path, ec);
        if (ec) {
            std::filesystem::remove(path, ec);
            ec.clear();
            std::filesystem::rename(tmp_path, path, ec);
        }
        if (ec) {
            if (error) *error = ec.message();
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

nlohmann::json SerializeNode(const std::shared_ptr<mbink::Node>& node,
                             SnapshotTraversalState* state,
                             int depth) {
    nlohmann::json j;
    if (!node) return j;
    if (state) {
        if (ShouldAbort(*state)) {
            state->truncated = true;
            state->truncated_reason = "shutdown_in_progress";
            return j;
        }
        if (state->node_count >= state->max_nodes) {
            state->truncated = true;
            if (state->truncated_reason.empty()) state->truncated_reason = "max_nodes";
            return j;
        }
        if (depth > state->max_depth) {
            state->truncated = true;
            if (state->truncated_reason.empty()) state->truncated_reason = "max_depth";
            return j;
        }
        ++state->node_count;
    }

    j["node_id"] = NodeId(node);
    j["children"] = nlohmann::json::array();
    j["attrs"] = nlohmann::json::object();
    j["scroll"] = nlohmann::json{{"x", 0}, {"y", 0}, {"max_x", 0}, {"max_y", 0}};
    j["visible"] = true;
    j["interactive"] = false;

    if (node->GetNodeType() == mbink::NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<mbink::Text>(node);
        const std::string data = text ? text->GetData() : "";
        bool whitespace = true;
        for (char c : data) {
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                whitespace = false;
                break;
            }
        }
        j["tag"] = "#text";
        j["text"] = whitespace ? nullptr : nlohmann::json(data);
        j["rect"] = nlohmann::json{{"x", 0}, {"y", 0}, {"w", 0}, {"h", 0}};
        return j;
    }

    if (node->GetNodeType() == mbink::NodeType::ELEMENT_NODE) {
        auto el = std::dynamic_pointer_cast<mbink::Element>(node);
        const std::string tag = el ? el->GetTagName() : "";
        const std::string normalized_tag = ToLowerAscii(tag);
        if (ShouldSkipElementTag(normalized_tag)) return nlohmann::json();

        j["tag"] = tag;
        j["text"] = nullptr;
        j["interactive"] = IsInteractiveTag(normalized_tag) || HasInteractiveAttributes(el);

        if (el) {
            for (const auto& [k, v] : el->GetAllAttributes()) j["attrs"][k] = v;
            auto rect = CachedElementRect(el);
            j["rect"] = rect;
            j["scroll"] = ElementScrollState(el);
            j["visible"] = rect.value("w", 0.0) > 0.0 && rect.value("h", 0.0) > 0.0;
        } else {
            j["rect"] = nlohmann::json{{"x", 0}, {"y", 0}, {"w", 0}, {"h", 0}};
        }

        for (const auto& child : node->GetChildNodes()) {
            auto child_json = SerializeNode(child, state, depth + 1);
            if (!child_json.is_object()) continue;
            if (child_json.value("tag", "") == "#text" && child_json["text"].is_null()) continue;
            j["children"].push_back(std::move(child_json));
        }
        return j;
    }

    j["tag"] = "#node";
    j["text"] = nullptr;
    j["rect"] = nlohmann::json{{"x", 0}, {"y", 0}, {"w", 0}, {"h", 0}};
    return j;
}

}  // namespace

bool ExportUiDevSnapshot(const std::shared_ptr<mbink::Window>& window,
                         const std::shared_ptr<mbink::Document>& document,
                         const std::string& output_path,
                         std::string* error) {
    return ExportUiDevSnapshot(window, document, output_path, SnapshotExportOptions{}, error);
}

bool ExportUiDevSnapshot(const std::shared_ptr<mbink::Window>& window,
                         const std::shared_ptr<mbink::Document>& document,
                         const std::string& output_path,
                         const SnapshotExportOptions& options,
                         std::string* error) {
    return ExportUiDevSnapshot(window.get(), document.get(), output_path, options, error);
}

bool ExportUiDevSnapshot(mbink::Window* window,
                         mbink::Document* document,
                         const std::string& output_path,
                         const SnapshotExportOptions& options,
                         std::string* error) {
    if (output_path.empty()) return false;
    if (!window || !document) {
        if (error) *error = "window/document is null";
        return false;
    }
    if (options.shutdown_requested && options.shutdown_requested->load()) {
        if (error) *error = "shutdown_in_progress";
        return false;
    }

    auto root = ResolveSnapshotRoot(document, options.root_selector, error);
    if (!root) return false;

    SnapshotTraversalState traversal;
    traversal.max_nodes = options.max_nodes == 0 ? 2000 : options.max_nodes;
    traversal.max_depth = options.max_depth <= 0 ? 64 : options.max_depth;
    traversal.shutdown_requested = options.shutdown_requested;

    auto tree = SerializeNode(root, &traversal, 0);
    if (options.shutdown_requested && options.shutdown_requested->load()) {
        if (error) *error = "shutdown_in_progress";
        return false;
    }

    const double viewport_width = tree.contains("rect") ? tree["rect"].value("w", 0.0) : 0.0;
    const double viewport_height = tree.contains("rect") ? tree["rect"].value("h", 0.0) : 0.0;
    const double dpr = static_cast<double>(window->GetDisplayScale());
    int physical_width = 0;
    int physical_height = 0;
    if (window) window->GetPhysicalSize(&physical_width, &physical_height);

    nlohmann::json screenshot = nlohmann::json{{"included", false}};
    std::string screenshot_base64;
    if (options.include_screenshot || options.inline_screenshot) {
        std::vector<uint8_t> png_bytes;
        int screenshot_width = 0;
        int screenshot_height = 0;
        std::string capture_error;
        if (!window->CaptureCurrentFramePng(&png_bytes, &screenshot_width, &screenshot_height, &capture_error)) {
            if (error) *error = capture_error.empty() ? "screenshot_capture_failed" : capture_error;
            return false;
        }
        const auto screenshot_path = options.screenshot_path.empty()
                                         ? DefaultScreenshotPathForSnapshot(std::filesystem::path(output_path))
                                         : std::filesystem::path(options.screenshot_path);
        if (!WriteFileAtomic(screenshot_path,
                             reinterpret_cast<const char*>(png_bytes.data()),
                             png_bytes.size(),
                             error)) {
            return false;
        }
        screenshot = nlohmann::json{{"included", true},
                                    {"format", "png"},
                                    {"mime_type", "image/png"},
                                    {"encoding", options.inline_screenshot ? "base64" : "file"},
                                    {"path", screenshot_path.string()},
                                    {"bytes", png_bytes.size()},
                                    {"width", screenshot_width},
                                    {"height", screenshot_height},
                                    {"dpr", dpr}};
        if (options.inline_screenshot) {
            screenshot_base64 = Base64Encode(png_bytes);
            screenshot["base64"] = screenshot_base64;
        }
    }

    nlohmann::json snapshot;
    snapshot["ok"] = true;
    snapshot["timestamp"] = "";
    snapshot["runtime_epoch"] = options.runtime_epoch;
    snapshot["node_count"] = traversal.node_count;
    snapshot["truncated"] = traversal.truncated;
    snapshot["truncated_reason"] = traversal.truncated ? traversal.truncated_reason : "";
    snapshot["limits"] = nlohmann::json{{"max_nodes", traversal.max_nodes}, {"max_depth", traversal.max_depth}};
    if (!options.root_selector.empty()) snapshot["root_selector"] = options.root_selector;
    snapshot["viewport"] = nlohmann::json{{"width", viewport_width},
                                           {"height", viewport_height},
                                           {"dpr", dpr},
                                           {"physical_width", physical_width},
                                           {"physical_height", physical_height}};
    snapshot["screenshot"] = screenshot;
    snapshot["screenshot_base64"] = screenshot_base64;
    snapshot["tree"] = std::move(tree);
    snapshot["note"] = "P0: runtime exported DOM snapshot; rect/visible are best-effort.";

    try {
        std::filesystem::path path(output_path);
        const auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        const auto payload = snapshot.dump(2);
        if (!WriteFileAtomic(path, payload.data(), payload.size(), error)) {
            if (error) *error = "write_snapshot_failed";
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

}  // namespace mbink::ui_dev
