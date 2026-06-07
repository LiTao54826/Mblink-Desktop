/**
 * @file document.cpp
 * @brief Document 类实现
 */

#include "document.h"
#include "bindings/native_data_binding.h"
#include "selection/range.h"
#include "elements/html_input_element.h"
#include "elements/html_audio_element.h"
#include "elements/html_textarea_element.h"
#include "elements/html_button_element.h"
#include "elements/html_form_element.h"
#include "elements/html_select_element.h"
#include "elements/html_option_element.h"
#include "elements/html_anchor_element.h"
#include "elements/html_label_element.h"
#include "elements/html_image_element.h"
#include "elements/html_canvas_element.h"
#include "elements/html_div_element.h"
#include "elements/html_span_element.h"
#include "elements/html_paragraph_element.h"
#include "elements/html_heading_element.h"
#include "elements/html_ulist_element.h"
#include "elements/html_olist_element.h"
#include "elements/html_li_element.h"
#include "elements/html_table_element.h"
#include "elements/html_form_controls.h"
#include "elements/html_head_element.h"
#include "elements/html_style_element.h"
#include "elements/html_script_element.h"
#include "elements/html_link_element.h"
#include "elements/html_template_element.h"
#include "elements/svg_element.h"
#include "elements/terminal/html_terminal_element.h"
#include "elements/logview/html_logview_element.h"
#include "core/window/window.h"
#include "core/lexbor/lexbor_document.h"
#include "core/lexbor/style_manager.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/utils/encoding_utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>

namespace fs = std::filesystem;

namespace mbink {

namespace {

fs::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return fs::path(utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string FsPathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    return utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::string NormalizeFsPath(const fs::path& path) {
    std::string result = FsPathToUtf8String(path.lexically_normal());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool IsSpecialResourcePath(const std::string& path) {
    return path.rfind("http://", 0) == 0 ||
           path.rfind("https://", 0) == 0 ||
           path.rfind("data:", 0) == 0 ||
           path.rfind("#", 0) == 0;
}

std::string ToAsciiLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

constexpr const char* kHtmlNamespaceUri = "http://www.w3.org/1999/xhtml";
constexpr const char* kSvgNamespaceUri = "http://www.w3.org/2000/svg";
constexpr const char* kMathMlNamespaceUri = "http://www.w3.org/1998/Math/MathML";

std::string ResolveLocalName(const std::string& qualified_name) {
    const auto colon = qualified_name.find(':');
    return colon == std::string::npos ? qualified_name : qualified_name.substr(colon + 1);
}

std::string ResolveElementNamespace(const std::shared_ptr<Element>& element) {
    if (std::dynamic_pointer_cast<SVGElement>(element)) {
        return kSvgNamespaceUri;
    }
    return kHtmlNamespaceUri;
}

std::string ResolveElementLocalName(const std::shared_ptr<Element>& element) {
    const auto& tag_name = element->GetTagName();
    if (std::dynamic_pointer_cast<SVGElement>(element)) {
        return tag_name;
    }
    return ToAsciiLower(tag_name);
}

std::string ResolveResourcePath(const std::string& path, const std::string& base_path) {
    if (path.empty() || IsSpecialResourcePath(path)) {
        return path;
    }

    fs::path p = Utf8PathToFsPath(path);
    if (p.is_absolute() || (!path.empty() && path.front() == '/')) {
        return NormalizeFsPath(p);
    }

    if (!base_path.empty()) {
        return NormalizeFsPath(Utf8PathToFsPath(base_path) / p);
    }

    return NormalizeFsPath(p);
}

std::string RewriteCssUrls(const std::string& css, const std::string& css_base_path) {
    if (css.empty() || css_base_path.empty()) {
        return css;
    }

    std::string rewritten;
    rewritten.reserve(css.size() + 32);

    size_t pos = 0;
    while (pos < css.size()) {
        size_t url_pos = css.find("url(", pos);
        if (url_pos == std::string::npos) {
            rewritten.append(css, pos, std::string::npos);
            break;
        }

        rewritten.append(css, pos, url_pos - pos);
        size_t value_begin = url_pos + 4;
        size_t value_end = css.find(')', value_begin);
        if (value_end == std::string::npos) {
            rewritten.append(css, url_pos, std::string::npos);
            break;
        }

        std::string inner = css.substr(value_begin, value_end - value_begin);
        size_t first = inner.find_first_not_of(" \t\r\n");
        size_t last = inner.find_last_not_of(" \t\r\n");
        if (first == std::string::npos || last == std::string::npos) {
            rewritten.append(css, url_pos, value_end - url_pos + 1);
            pos = value_end + 1;
            continue;
        }

        std::string value = inner.substr(first, last - first + 1);
        char quote = 0;
        if (!value.empty() && (value.front() == '\'' || value.front() == '"')) {
            quote = value.front();
            if (value.size() >= 2 && value.back() == quote) {
                value = value.substr(1, value.size() - 2);
            }
        }

        std::string resolved = IsSpecialResourcePath(value)
            ? value
            : ResolveResourcePath(value, css_base_path);

        rewritten += "url(";
        if (quote) rewritten.push_back(quote);
        rewritten += resolved;
        if (quote) rewritten.push_back(quote);
        rewritten.push_back(')');
        pos = value_end + 1;
    }

    return rewritten;
}

}

// 静态成员初始化
Document::FileAssetProvider Document::asset_provider_ = nullptr;

void Document::SetAssetProvider(FileAssetProvider provider) {
    asset_provider_ = provider;
}

Document::FileAssetProvider Document::GetAssetProvider() {
    return asset_provider_;
}

// ========== 构造函数 ==========

Document::Document()
    : Node(NodeType::DOCUMENT_NODE)
    , document_element_(nullptr)
    , head_(nullptr)
    , body_(nullptr)
    , id_map_()
    , lexbor_doc_(std::make_unique<LexborDocument>())
    , lexbor_dirty_(false)
    , style_manager_(std::make_unique<StyleManager>(this))
    , js_runtime_(nullptr) {
}

Document::~Document() = default;

StyleManager* Document::GetStyleManager() const {
    return style_manager_.get();
}

NativeDataBindingRuntime* Document::GetNativeDataBindingRuntime() {
    if (!state_manager_) {
        return nullptr;
    }
    if (!native_data_binding_runtime_) {
        native_data_binding_runtime_ = std::make_unique<NativeDataBindingRuntime>(*state_manager_);
    }
    return native_data_binding_runtime_.get();
}

void Document::Initialize() {
    // 创建基本的 HTML 结构
    document_element_ = CreateElement("html");
    
    // 创建并添加 head 元素
    head_ = CreateElement("head");
    document_element_->AppendChild(head_);
    
    // 创建并添加 body 元素
    body_ = CreateElement("body");
    document_element_->AppendChild(body_);
    
    AppendChild(document_element_);
}

// ========== 工厂方法 ==========

std::shared_ptr<Element> Document::CreateElement(const std::string& tag_name) {
    // 根据标签名创建特定类型的元素
    std::shared_ptr<Element> element;

    if (tag_name == "input") {
        element = std::make_shared<HTMLInputElement>();
    } else if (tag_name == "textarea") {
        element = std::make_shared<HTMLTextAreaElement>();
    } else if (tag_name == "button") {
        auto button = std::make_shared<HTMLButtonElement>();
        // 初始化伪类状态（必须在shared_ptr创建后）
        button->SetPseudoClass(":enabled", true);
        // 设置默认type属性
        button->Element::SetAttribute("type", "submit");
        element = button;
    } else if (tag_name == "form") {
        element = std::make_shared<HTMLFormElement>();
    } else if (tag_name == "select") {
        auto select = std::make_shared<HTMLSelectElement>();
        // 初始化伪类状态
        select->SetPseudoClass(":enabled", true);
        element = select;
    } else if (tag_name == "option") {
        auto option = std::make_shared<HTMLOptionElement>();
        // 初始化伪类状态
        option->SetPseudoClass(":enabled", true);
        element = option;
    } else if (tag_name == "a") {
        auto anchor = std::make_shared<HTMLAnchorElement>();
        // 不设置默认伪类，只有在有href时才设置:link
        element = anchor;
    } else if (tag_name == "label") {
        auto label = std::make_shared<HTMLLabelElement>();
        element = label;
    } else if (tag_name == "img") {
        auto img = std::make_shared<HTMLImageElement>();
        element = img;
    } else if (tag_name == "audio") {
        element = std::make_shared<HTMLAudioElement>();
    } else if (tag_name == "canvas") {
        auto canvas = std::make_shared<HTMLCanvasElement>();
        element = canvas;
    } else if (tag_name == "div") {
        auto div = std::make_shared<HTMLDivElement>();
        element = div;
    } else if (tag_name == "span") {
        auto span = std::make_shared<HTMLSpanElement>();
        element = span;
    } else if (tag_name == "p") {
        auto p = std::make_shared<HTMLParagraphElement>();
        element = p;
    } else if (tag_name == "h1") {
        auto h1 = std::make_shared<HTMLHeadingElement>(1);
        element = h1;
    } else if (tag_name == "h2") {
        auto h2 = std::make_shared<HTMLHeadingElement>(2);
        element = h2;
    } else if (tag_name == "h3") {
        auto h3 = std::make_shared<HTMLHeadingElement>(3);
        element = h3;
    } else if (tag_name == "h4") {
        auto h4 = std::make_shared<HTMLHeadingElement>(4);
        element = h4;
    } else if (tag_name == "h5") {
        auto h5 = std::make_shared<HTMLHeadingElement>(5);
        element = h5;
    } else if (tag_name == "h6") {
        auto h6 = std::make_shared<HTMLHeadingElement>(6);
        element = h6;
    } else if (tag_name == "ul") {
        element = std::make_shared<HTMLUListElement>();
    } else if (tag_name == "ol") {
        element = std::make_shared<HTMLOListElement>();
    } else if (tag_name == "li") {
        element = std::make_shared<HTMLLIElement>();
    }
    // ========== 表格标签 ==========
    else if (tag_name == "table") {
        element = std::make_shared<HTMLTableElement>();
    } else if (tag_name == "thead" || tag_name == "tbody" || tag_name == "tfoot") {
        element = std::make_shared<HTMLTableSectionElement>(tag_name);
    } else if (tag_name == "tr") {
        element = std::make_shared<HTMLTableRowElement>();
    } else if (tag_name == "td" || tag_name == "th") {
        element = std::make_shared<HTMLTableCellElement>(tag_name);
    } else if (tag_name == "caption") {
        element = std::make_shared<HTMLTableCaptionElement>();
    } else if (tag_name == "col" || tag_name == "colgroup") {
        element = std::make_shared<HTMLTableColElement>(tag_name);
    }
    // ========== 表单增强标签 ==========
    else if (tag_name == "fieldset") {
        element = std::make_shared<HTMLFieldSetElement>();
    } else if (tag_name == "legend") {
        element = std::make_shared<HTMLLegendElement>();
    } else if (tag_name == "optgroup") {
        element = std::make_shared<HTMLOptGroupElement>();
    } else if (tag_name == "datalist") {
        element = std::make_shared<HTMLDataListElement>();
    } else if (tag_name == "output") {
        element = std::make_shared<HTMLOutputElement>();
    } else if (tag_name == "progress") {
        element = std::make_shared<HTMLProgressElement>();
    } else if (tag_name == "meter") {
        element = std::make_shared<HTMLMeterElement>();
    } else if (tag_name == "dialog") {
        element = std::make_shared<HTMLDialogElement>();
    }
    // ========== SVG 元素 ==========
    else if (tag_name == "svg") {
        element = std::make_shared<SVGSVGElement>();
    } else if (tag_name == "path") {
        element = std::make_shared<SVGPathElement>();
    } else if (tag_name == "g") {
        element = std::make_shared<SVGGElement>();
    } else if (tag_name == "circle") {
        element = std::make_shared<SVGCircleElement>();
    } else if (tag_name == "rect") {
        element = std::make_shared<SVGRectElement>();
    } else if (tag_name == "ellipse") {
        element = std::make_shared<SVGEllipseElement>();
    } else if (tag_name == "line") {
        element = std::make_shared<SVGLineElement>();
    } else if (tag_name == "polyline") {
        element = std::make_shared<SVGPolylineElement>();
    } else if (tag_name == "polygon") {
        element = std::make_shared<SVGPolygonElement>();
    } else if (tag_name == "text") {
        element = std::make_shared<SVGTextElement>();
    } else if (tag_name == "defs" || tag_name == "linearGradient" || tag_name == "radialGradient" ||
               tag_name == "stop" || tag_name == "clipPath" || tag_name == "mask" ||
               tag_name == "pattern" || tag_name == "symbol" || tag_name == "use" ||
               tag_name == "image" || tag_name == "foreignObject" || tag_name == "tspan") {
        element = std::make_shared<SVGElement>(tag_name);
    }
    // ========== 文档结构标签 ==========
    else if (tag_name == "head") {
        element = std::make_shared<HTMLHeadElement>();
    } else if (tag_name == "style") {
        element = std::make_shared<HTMLStyleElement>();
    } else if (tag_name == "script") {
        element = std::make_shared<HTMLScriptElement>();
    } else if (tag_name == "link") {
        element = std::make_shared<HTMLLinkElement>();
    } else if (tag_name == "template") {
        element = std::make_shared<HTMLTemplateElement>();
    }
    // ========== 虚拟文本组件 ==========
    else if (tag_name == "terminal") {
        element = std::make_shared<HTMLTerminalElement>();
    } else if (tag_name == "logview") {
        element = std::make_shared<HTMLLogViewElement>();
    } else {
        // 所有其他标签使用通用 Element 类
        // 包括：语义化标签（header, footer, nav, section, article, aside, main, figure, figcaption）
        //       文本标签（strong, em, b, i, u, s, mark, code, kbd, pre, blockquote, etc.）
        //       列表标签（dl, dt, dd）
        //       其他标签（title, meta, link, base, noscript 等）
        element = std::make_shared<Element>(tag_name);
    }

    // 设置 owner_document（使用 friend 访问权限）
    element->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    if (auto template_element = std::dynamic_pointer_cast<HTMLTemplateElement>(element)) {
        if (auto content = template_element->GetContent()) {
            content->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
        }
    }
    element->SetLocalName(ResolveElementLocalName(element));
    element->SetNamespaceURI(ResolveElementNamespace(element));

    // 如果是 html 元素，设置为 documentElement
    if (tag_name == "html" && !document_element_) {
        document_element_ = element;
        AppendChild(element);
    }

    return element;
}

std::shared_ptr<Element> Document::CreateElementNS(const std::string& namespace_uri,
                                                   const std::string& qualified_name) {
    const std::string resolved_namespace = namespace_uri.empty() ? kHtmlNamespaceUri : namespace_uri;
    const std::string local_name = ResolveLocalName(qualified_name);

    std::shared_ptr<Element> element;
    if (resolved_namespace == kHtmlNamespaceUri) {
        element = CreateElement(local_name);
    } else if (resolved_namespace == kSvgNamespaceUri) {
        element = CreateElement(local_name);
        if (element) {
            element->SetNamespaceURI(kSvgNamespaceUri);
            element->SetLocalName(local_name);
        }
    } else {
        element = std::make_shared<Element>(local_name, local_name, resolved_namespace);
        element->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    }

    if (element) {
        element->SetNamespaceURI(resolved_namespace);
        element->SetLocalName(resolved_namespace == kHtmlNamespaceUri ? ToAsciiLower(local_name) : local_name);
    }

    return element;
}

std::shared_ptr<Text> Document::CreateTextNode(const std::string& data) {
    auto text = std::make_shared<Text>(data);
    // 设置 owner_document
    text->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    return text;
}

std::shared_ptr<Comment> Document::CreateComment(const std::string& data) {
    auto comment = std::make_shared<Comment>(data);
    comment->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    return comment;
}

std::shared_ptr<DocumentFragment> Document::CreateDocumentFragment() {
    auto fragment = std::make_shared<DocumentFragment>();
    // 设置 owner_document
    fragment->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    return fragment;
}

std::shared_ptr<Range> Document::CreateRange() {
    auto range = std::make_shared<Range>(std::static_pointer_cast<Document>(shared_from_this()));
    return range;
}

// ========== 文档属性 ==========

void Document::SetBody(std::shared_ptr<Element> body) {
    body_ = body;
}

void Document::SetHead(std::shared_ptr<Element> head) {
    head_ = head;
}

void Document::PostUiTask(std::function<void()> task) {
    if (!task) {
        return;
    }

    auto window = window_handle_.lock();
    if (!window) {
        return;
    }

    window->PostUiTask(std::move(task));
}

void Document::FlushUiTasks() {
    auto window = window_handle_.lock();
    if (!window) {
        return;
    }

    window->FlushUiTasks();
}

// ========== Lexbor 集成 ==========

bool Document::LoadHTML(const std::string& html) {
    if (!lexbor_doc_) {
        return false;
    }

    // 使用 Lexbor 解析 HTML
    if (!lexbor_doc_->ParseHTML(html)) {
        return false;
    }

    // 从 Lexbor DOM 同步到 MBink DOM
    SyncFromLexbor();
    AutoMountNativeDeclarativeBindings();

    // 解析所有 <style> 标签
    if (style_manager_) {
        auto style_elements = GetElementsByTagName("style");
        for (auto& style_elem : style_elements) {
            style_manager_->ParseStyleElement(style_elem.get());
        }
    }

    lexbor_dirty_ = false;
    return true;
}

bool Document::LoadHTMLFile(const std::string& file_path) {
    if (!lexbor_doc_) {
        return false;
    }

    // 使用 Lexbor 从文件解析 HTML
    if (!lexbor_doc_->ParseHTMLFile(file_path)) {
        return false;
    }

    // 从 Lexbor DOM 同步到 MBink DOM
    SyncFromLexbor();
    AutoMountNativeDeclarativeBindings();

    // 解析所有 <style> 标签
    if (style_manager_) {
        auto style_elements = GetElementsByTagName("style");
        for (auto& style_elem : style_elements) {
            style_manager_->ParseStyleElement(style_elem.get());
        }
    }

    lexbor_dirty_ = false;
    return true;
}

std::string Document::SaveHTML() {
    if (!lexbor_doc_) {
        return "";
    }

    // 如果 MBink DOM 已修改，先同步到 Lexbor
    if (lexbor_dirty_) {
        SyncToLexbor();
        lexbor_dirty_ = false;
    }

    // 使用 Lexbor 序列化 HTML
    return lexbor_doc_->SerializeToHTML();
}

void Document::SyncFromLexbor() {
    if (!lexbor_doc_) {
        return;
    }
    if (native_data_binding_runtime_) {
        native_data_binding_runtime_->clear();
    }

    // 清空当前 DOM 树
    child_nodes_.clear();
    document_element_ = nullptr;
    body_ = nullptr;
    head_ = nullptr;
    id_map_.clear();

    // 获取 Lexbor 文档元素
    LexborElement* lexbor_html = lexbor_doc_->GetDocumentElement();
    if (!lexbor_html) {
        return;
    }

    // 转换 Lexbor DOM 到 MBink DOM
    lxb_dom_node_t* lexbor_node = lxb_dom_interface_node(lexbor_html->GetNativeElement());
    auto doc_ptr = std::dynamic_pointer_cast<Document>(shared_from_this());
    std::shared_ptr<Node> html_node = Element::ConvertLexborNodeToNode(lexbor_node, doc_ptr);

    if (html_node) {
        auto html_elem = std::dynamic_pointer_cast<Element>(html_node);
        if (html_elem) {
            document_element_ = html_elem;
            AppendChild(html_elem);

            // 查找 head 和 body 元素
            for (const auto& child : html_elem->GetChildNodes()) {
                auto child_elem = std::dynamic_pointer_cast<Element>(child);
                if (child_elem) {
                    if (child_elem->GetTagName() == "head") {
                        head_ = child_elem;
                    } else if (child_elem->GetTagName() == "body") {
                        body_ = child_elem;
                    }
                }
            }

            // 重建 ID 映射
            RebuildIdMap(html_elem);
        }
    }
}

void Document::AutoMountNativeDeclarativeBindings() {
    AutoMountNativeDeclarativeBindings(body_ ? body_ : document_element_);
}

void Document::AutoMountNativeDeclarativeBindings(const std::shared_ptr<Element>& root) {
    auto* runtime = GetNativeDataBindingRuntime();
    if (!runtime) {
        return;
    }
    if (!root) {
        return;
    }
    runtime->mountDeclarative(root);
}

void Document::RefreshNativeDeclarativeBindings(const std::shared_ptr<Element>& root) {
    auto* runtime = GetNativeDataBindingRuntime();
    if (!runtime || !root) {
        return;
    }
    runtime->refreshDeclarative(root);
}

void Document::UnmountNativeDeclarativeBindings(const std::shared_ptr<Element>& root) {
    auto* runtime = GetNativeDataBindingRuntime();
    if (!runtime || !root) {
        return;
    }
    runtime->unmountDeclarative(root);
}

void Document::SyncToLexbor() {
    if (!lexbor_doc_ || !document_element_) {
        return;
    }

    // 序列化 MBink DOM 为 HTML
    std::string html = document_element_->GetOuterHTML();

    // 重新解析到 Lexbor
    lexbor_doc_->ParseHTML(html);
}

// ========== 查询方法 ==========

std::shared_ptr<Element> Document::GetElementById(const std::string& id) {
    auto it = id_map_.find(id);
    if (it != id_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::vector<std::shared_ptr<Element>> Document::GetElementsByTagName(const std::string& tag_name) {
    std::vector<std::shared_ptr<Element>> result;

    if (document_element_) {
        CollectElementsByTagName(document_element_, tag_name, result);
    }

    return result;
}

std::vector<std::shared_ptr<Element>> Document::GetElementsByClassName(const std::string& class_name) {
    std::vector<std::shared_ptr<Element>> result;

    if (document_element_) {
        CollectElementsByClassName(document_element_, class_name, result);
    }

    return result;
}

// ========== ID 映射管理 ==========

void Document::RegisterElementId(const std::string& id, std::shared_ptr<Element> element) {
    id_map_[id] = element;
}

void Document::UnregisterElementId(const std::string& id) {
    id_map_.erase(id);
}

void Document::UnregisterElementAndDescendantIds(std::shared_ptr<Element> element) {
    if (!element) {
        return;
    }

    // 注销当前元素的 ID
    std::string id = element->GetAttribute("id");
    if (!id.empty()) {
        UnregisterElementId(id);
    }

    // 递归注销所有后代元素的 ID
    for (const auto& child : element->GetChildNodes()) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_element = std::static_pointer_cast<Element>(child);
            UnregisterElementAndDescendantIds(child_element);
        }
    }
}

// ========== Node 接口实现 ==========

std::shared_ptr<Node> Document::CloneNode(bool deep) {
    auto cloned = std::make_shared<Document>();

    if (deep && document_element_) {
        auto cloned_element = std::dynamic_pointer_cast<Element>(document_element_->CloneNode(true));
        cloned->document_element_ = cloned_element;
        cloned->AppendChild(cloned_element);
    }

    return cloned;
}

// ========== 私有辅助方法 ==========

void Document::CollectElementsByTagName(std::shared_ptr<Node> node,
                                        const std::string& tag_name,
                                        std::vector<std::shared_ptr<Element>>& result) {
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        if (element->GetTagName() == tag_name) {
            result.push_back(element);
        }
    }

    // 递归遍历子节点
    for (const auto& child : node->GetChildNodes()) {
        CollectElementsByTagName(child, tag_name, result);
    }
}

void Document::CollectElementsByClassName(std::shared_ptr<Node> node,
                                          const std::string& class_name,
                                          std::vector<std::shared_ptr<Element>>& result) {
    auto element = std::dynamic_pointer_cast<Element>(node);
    if (element) {
        if (element->HasClass(class_name)) {
            result.push_back(element);
        }
    }

    // 递归遍历子节点
    for (const auto& child : node->GetChildNodes()) {
        CollectElementsByClassName(child, class_name, result);
    }
}

void Document::RebuildIdMap(std::shared_ptr<Element> root) {
    if (!root) {
        return;
    }

    // 如果元素有 ID，注册到映射表
    std::string id = root->GetAttribute("id");
    if (!id.empty()) {
        RegisterElementId(id, root);
    }

    // 递归处理子节点
    for (const auto& child : root->GetChildNodes()) {
        auto child_elem = std::dynamic_pointer_cast<Element>(child);
        if (child_elem) {
            RebuildIdMap(child_elem);
        }
    }
}

// ========== 批量更新API (Week 2 - Task 2.3) ==========

void Document::BeginBatch() {
    batch_depth_++;
}

void Document::EndBatch() {
    if (batch_depth_ <= 0) {
        return;
    }

    batch_depth_--;

    // 只在最外层批量结束时触发重绘
    if (batch_depth_ == 0) {
        // 通知观察者整个文档子树已修改
        // 这会触发Window的SetNeedsRepaint()
        observer_manager_.NotifySubtreeModified(this);
    }
}

// ========== 焦点管理 ==========

std::shared_ptr<Element> Document::GetActiveElement() const {
    auto element = active_element_.lock();
    // 如果没有焦点元素，返回 body（符合浏览器标准）
    return element ? element : body_;
}

void Document::SetActiveElement(std::shared_ptr<Element> element) {
    active_element_ = element;
}

// ========== 脏区域管理 ==========

void Document::AddDirtyRect(const SkRect& rect) {
    if (rect.isEmpty()) {
        return;
    }
    
    // 直接添加新区域，不进行合并
    // 这样可以保持每个脏区域的独立性，便于调试和优化
    dirty_rects_.push_back(rect);
}

SkRect Document::GetMergedDirtyRect() const {
    if (dirty_rects_.empty()) {
        return SkRect::MakeEmpty();
    }
    
    SkRect merged = dirty_rects_[0];
    for (size_t i = 1; i < dirty_rects_.size(); ++i) {
        merged.join(dirty_rects_[i]);
    }
    return merged;
}

// ========== JavaScript 脚本执行 ==========

void Document::ExecuteScripts() {
    if (js_runtime_) {
        ExecuteScripts(js_runtime_);
    }
}

void Document::ExecuteScripts(QuickJSRuntime* runtime) {
    if (!runtime) {
        return;
    }

    auto scripts = GetElementsByTagName("script");
    for (auto& script_elem : scripts) {
        auto script = std::dynamic_pointer_cast<HTMLScriptElement>(script_elem);
        if (!script || script->IsExecuted()) {
            continue;
        }

        try {
            std::string code;
            std::string script_name;
            std::string display_name;

            if (script->IsExternal()) {
                std::string src = script->GetSrc();
                std::string resolved_src = ResolvePath(src);
                code = ReadExternalFile(src);
                script_name = resolved_src.empty() ? src : NormalizeFsPath(Utf8PathToFsPath(resolved_src));
                display_name = script_name.empty() ? src : script_name;

                if (code.empty()) {
                    AppendLoadError("Failed to load script: " + display_name);
                    script->MarkExecuted();
                    continue;
                }
            } else {
                code = script->GetScriptText();
                script_name = "<inline-script>";
                display_name = script_name;
            }

            if (code.empty()) {
                script->MarkExecuted();
                continue;
            }

            std::string type = script->GetType();
            if (type == "module") {
                if (script->IsExternal() && !script_name.empty()) {
                    runtime->SetBaseModulePath(script_name);
                }
                runtime->EvalModule(code, script_name);
            } else {
                runtime->Eval(code, script_name);
            }

            script->MarkExecuted();
        } catch (const std::exception& e) {
            const bool is_module = script->GetType() == "module";
            const std::string target = script->IsExternal()
                ? (script->GetSrc().empty() ? std::string("<external-script>") : script->GetSrc())
                : std::string("<inline-script>");
            AppendLoadError(std::string("Failed to execute ") +
                            (is_module ? "module: " : "script: ") +
                            target + " - " + e.what());
            script->MarkExecuted();
        } catch (...) {
            const bool is_module = script->GetType() == "module";
            const std::string target = script->IsExternal()
                ? (script->GetSrc().empty() ? std::string("<external-script>") : script->GetSrc())
                : std::string("<inline-script>");
            AppendLoadError(std::string("Failed to execute ") +
                            (is_module ? "module: " : "script: ") +
                            target + " - unknown error");
            script->MarkExecuted();
        }
    }

    AutoMountNativeDeclarativeBindings();
}

std::string Document::ConsumeLoadErrors() {
    if (load_errors_.empty()) {
        return "";
    }

    std::string merged;
    for (size_t i = 0; i < load_errors_.size(); ++i) {
        if (i > 0) {
            merged += '\n';
        }
        merged += load_errors_[i];
    }
    load_errors_.clear();
    return merged;
}

void Document::AppendLoadError(const std::string& error) {
    if (error.empty()) {
        return;
    }
    load_errors_.push_back(error);
    std::cerr << "[MBink Document Error] " << error << std::endl;
}

// ========== 资源加载 ==========

std::string Document::ResolvePath(const std::string& path) const {
    return ResolveResourcePath(path, base_path_);
}

std::string Document::ReadExternalFile(const std::string& path) const {
    const std::string resolved_path = ResolvePath(path);

    if (asset_provider_) {
        std::vector<uint8_t> data;
        if (asset_provider_(path, data) ||
            (!resolved_path.empty() && resolved_path != path && asset_provider_(resolved_path, data))) {
            return std::string(data.begin(), data.end());
        }
    }

    if (resolved_path.empty() || IsSpecialResourcePath(resolved_path)) {
        return "";
    }

    fs::path resolved_fs_path = Utf8PathToFsPath(resolved_path);
    if (!fs::exists(resolved_fs_path)) {
        return "";
    }

    std::ifstream file(resolved_fs_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Document::LoadExternalStylesheets() {
    auto links = GetElementsByTagName("link");

    for (auto& link_elem : links) {
        auto link = std::dynamic_pointer_cast<HTMLLinkElement>(link_elem);
        if (!link || link->GetRel() != "stylesheet" || link->IsLoaded()) {
            continue;
        }

        std::string href = link->GetHref();
        if (href.empty()) {
            link->MarkLoaded();
            continue;
        }

        const std::string resolved_href = ResolvePath(href);
        const std::string display_name = resolved_href.empty() ? href : resolved_href;
        std::string css = ReadExternalFile(href);
        if (css.empty()) {
            AppendLoadError("Failed to load stylesheet: " + display_name);
            link->MarkLoaded();
            continue;
        }

        try {
            const std::string css_base_path = resolved_href.empty()
                ? ""
                : NormalizeFsPath(Utf8PathToFsPath(resolved_href).parent_path());
            css = RewriteCssUrls(css, css_base_path);

            if (style_manager_) {
                style_manager_->ParseCSSString(css, 50, resolved_href.empty() ? "external-link" : resolved_href);
            }
        } catch (const std::exception& e) {
            AppendLoadError("Failed to parse stylesheet: " + display_name + " - " + e.what());
        } catch (...) {
            AppendLoadError("Failed to parse stylesheet: " + display_name + " - unknown error");
        }

        link->MarkLoaded();
    }
}

} // namespace mbink
