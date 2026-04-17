#include "ui_dev_snapshot.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/node.h"
#include "core/dom/text.h"
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

nlohmann::json SerializeNode(const std::shared_ptr<mbink::Node>& node) {
    nlohmann::json j;
    if (!node) return j;

    j["node_id"] = NodeId(node);
    j["children"] = nlohmann::json::array();
    j["attrs"] = nlohmann::json::object();
    j["scroll"] = nlohmann::json{{"x", 0}, {"y", 0}, {"max_x", 0}, {"max_y", 0}};
    j["visible"] = true;
    j["interactive"] = false;

    if (node->GetNodeType() == mbink::NodeType::TEXT_NODE) {
        auto text = std::dynamic_pointer_cast<mbink::Text>(node);
        const std::string data = text ? text->GetData() : "";
        // 跳过纯空白文本
        bool ws = true;
        for (char c : data) {
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                ws = false;
                break;
            }
        }
        j["tag"] = "#text";
        j["text"] = ws ? nullptr : nlohmann::json(data);
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
            const auto r = el->GetBoundingClientRect();
            j["rect"] = nlohmann::json{{"x", r.x}, {"y", r.y}, {"w", r.width}, {"h", r.height}};
            j["visible"] = r.width > 0.0f && r.height > 0.0f;
        } else {
            j["rect"] = nlohmann::json{{"x", 0}, {"y", 0}, {"w", 0}, {"h", 0}};
        }

        for (const auto& child : node->GetChildNodes()) {
            auto cj = SerializeNode(child);
            if (!cj.is_object()) continue;
            // 丢弃空白 text
            if (cj.value("tag", "") == "#text" && cj["text"].is_null()) continue;
            j["children"].push_back(std::move(cj));
        }
        return j;
    }

    // 其他节点类型
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
    if (output_path.empty()) return false;
    if (!window || !document) {
        if (error) *error = "window/document 为空";
        return false;
    }

    auto body = document->GetBody();
    std::shared_ptr<mbink::Node> root = body ? std::static_pointer_cast<mbink::Node>(body)
                                             : std::static_pointer_cast<mbink::Node>(document->GetDocumentElement());

    auto tree = SerializeNode(root);
    const double viewport_width = tree.contains("rect") ? tree["rect"].value("w", 0.0) : 0.0;
    const double viewport_height = tree.contains("rect") ? tree["rect"].value("h", 0.0) : 0.0;

    nlohmann::json snapshot;
    snapshot["ok"] = true;
    snapshot["timestamp"] = "";
    snapshot["viewport"] = nlohmann::json{{"width", viewport_width}, {"height", viewport_height}, {"dpr", 1.0}};
    snapshot["screenshot_base64"] = "";
    snapshot["tree"] = std::move(tree);
    snapshot["note"] = "P0: runtime 导出的 DOM snapshot（rect/visible 为 best-effort）";

    try {
        std::filesystem::path p(output_path);
        std::filesystem::create_directories(p.parent_path());
        std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
        ofs << snapshot.dump(2);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

}  // namespace mbink::ui_dev
