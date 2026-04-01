/**
 * @file document.cpp
 * @brief Document 类实现
 */

#include "document.h"
#include "selection/range.h"
#include "elements/html_input_element.h"
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
#include "elements/svg_element.h"
#include "elements/terminal/html_terminal_element.h"
#include "elements/logview/html_logview_element.h"
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
        //       其他标签（title, meta, link, base, noscript, template 等）
        element = std::make_shared<Element>(tag_name);
    }

    // 设置 owner_document（使用 friend 访问权限）
    element->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());

    // 如果是 html 元素，设置为 documentElement
    if (tag_name == "html" && !document_element_) {
        document_element_ = element;
        AppendChild(element);
    }

    return element;
}

std::shared_ptr<Text> Document::CreateTextNode(const std::string& data) {
    auto text = std::make_shared<Text>(data);
    // 设置 owner_document
    text->owner_document_ = std::static_pointer_cast<Document>(shared_from_this());
    return text;
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

    // 清空当前 DOM 树
    child_nodes_.clear();
    document_element_ = nullptr;
    body_ = nullptr;
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

    // 获取所有 script 元素
    auto scripts = GetElementsByTagName("script");

    for (auto& script_elem : scripts) {
        auto script = std::dynamic_pointer_cast<HTMLScriptElement>(script_elem);
        if (!script) {
            continue;
        }

        // 跳过已执行的脚本
        if (script->IsExecuted()) {
            continue;
        }

        std::string code;
        std::string script_name;

        // 处理外部脚本
        if (script->IsExternal()) {
            std::string src = script->GetSrc();
            std::string resolved_src = ResolvePath(src);
            code = ReadExternalFile(src);

            if (code.empty()) {
                script->MarkExecuted();
                continue;
            }

            script_name = resolved_src.empty() ? src : NormalizeFsPath(Utf8PathToFsPath(resolved_src));
        } else {
            // 内联脚本
            code = script->GetScriptText();
            script_name = "<inline-script>";
        }

        if (code.empty()) {
            script->MarkExecuted();
            continue;
        }

        // 执行脚本
        try {
            std::string type = script->GetType();

            if (type == "module") {
                // ES6 模块
                if (script->IsExternal() && !script_name.empty()) {
                    runtime->SetBaseModulePath(script_name);
                }
                runtime->EvalModule(code, script_name);
            } else {
                // 普通脚本（text/javascript 或空）
                runtime->Eval(code, script_name);
            }

            script->MarkExecuted();
        } catch (const std::exception& e) {
            script->MarkExecuted();  // 标记为已执行，避免重复执行失败的脚本
        }
    }
}

// ========== 资源加载 ==========

std::string Document::ResolvePath(const std::string& path) const {
    if (path.empty()) {
        return "";
    }

    // 如果是绝对路径，直接返回
    fs::path p = Utf8PathToFsPath(path);
    if (p.is_absolute()) {
        return NormalizeFsPath(p);
    }

    // 如果有基础路径，拼接
    if (!base_path_.empty()) {
        fs::path base = Utf8PathToFsPath(base_path_);
        fs::path resolved = base / p;
        return NormalizeFsPath(resolved);
    }

    // 否则返回原路径
    return path;
}

std::string Document::ReadExternalFile(const std::string& path) const {
    // 1. 优先从嵌入资源加载
    if (asset_provider_) {
        std::vector<uint8_t> data;
        if (asset_provider_(path, data)) {
            return std::string(data.begin(), data.end());
        }
    }

    // 2. 回退到文件系统
    std::string resolved_path = ResolvePath(path);

    if (resolved_path.empty()) {
        return "";
    }

    // 检查文件是否存在
    fs::path resolved_fs_path = Utf8PathToFsPath(resolved_path);
    if (!fs::exists(resolved_fs_path)) {
        return "";
    }

    // 读取文件内容
    std::ifstream file(resolved_fs_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Document::LoadExternalStylesheets() {
    // 获取所有 link 元素
    auto links = GetElementsByTagName("link");

    for (auto& link_elem : links) {
        auto link = std::dynamic_pointer_cast<HTMLLinkElement>(link_elem);
        if (!link) {
            continue;
        }

        // 只处理样式表
        if (link->GetRel() != "stylesheet") {
            continue;
        }

        // 跳过已加载的
        if (link->IsLoaded()) {
            continue;
        }

        std::string href = link->GetHref();
        if (href.empty()) {
            link->MarkLoaded();
            continue;
        }

        // 读取外部 CSS 文件
        std::string css = ReadExternalFile(href);

        if (css.empty()) {
            link->MarkLoaded();
            continue;
        }

        // 解析 CSS 并添加到样式管理器
        if (style_manager_) {
            style_manager_->ParseCSSString(css, 50, "external-link");
        }

        link->MarkLoaded();
    }
}

} // namespace mbink
