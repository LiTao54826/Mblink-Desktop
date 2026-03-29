/**
 * @file lexbor_document.h
 * @brief Lexbor Document C++ 包装类
 * 
 * 提供友好的 C++ 接口来使用 Lexbor 的 HTML/DOM 功能
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>

// Lexbor C 头文件
#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

namespace mbink {

// 前向声明
class LexborElement;
class LexborText;

/**
 * @brief Lexbor Document 包装类
 *
 * 管理 Lexbor HTML 文档的生命周期，提供 C++ 风格的 API
 */
class LexborDocument {
    friend class LexborElement;  // 允许 LexborElement 访问私有成员

public:
    /**
     * @brief 构造函数
     */
    LexborDocument();
    
    /**
     * @brief 析构函数 - 自动清理 Lexbor 资源
     */
    ~LexborDocument();
    
    // 禁止拷贝
    LexborDocument(const LexborDocument&) = delete;
    LexborDocument& operator=(const LexborDocument&) = delete;
    
    // 允许移动
    LexborDocument(LexborDocument&& other) noexcept;
    LexborDocument& operator=(LexborDocument&& other) noexcept;
    
    /**
     * @brief 解析 HTML 字符串
     * @param html HTML 字符串
     * @return 是否成功
     */
    bool ParseHTML(const std::string& html);

    /**
     * @brief 从文件解析 HTML
     * @param file_path 文件路径
     * @return 是否成功
     */
    bool ParseHTMLFile(const std::string& file_path);

    /**
     * @brief 获取 body 元素
     * @return body 元素指针，如果不存在返回 nullptr
     */
    LexborElement* GetBody();
    
    /**
     * @brief 获取 head 元素
     * @return head 元素指针，如果不存在返回 nullptr
     */
    LexborElement* GetHead();
    
    /**
     * @brief 获取 document element (html 元素)
     * @return html 元素指针
     */
    LexborElement* GetDocumentElement();
    
    /**
     * @brief 根据 ID 查找元素
     * @param id 元素 ID
     * @return 元素指针，如果不存在返回 nullptr
     */
    LexborElement* GetElementById(const std::string& id);
    
    /**
     * @brief 使用 CSS 选择器查找第一个匹配的元素
     * @param selector CSS 选择器字符串
     * @return 元素指针，如果不存在返回 nullptr
     */
    LexborElement* QuerySelector(const std::string& selector);
    
    /**
     * @brief 使用 CSS 选择器查找所有匹配的元素
     * @param selector CSS 选择器字符串
     * @return 元素指针列表
     */
    std::vector<LexborElement*> QuerySelectorAll(const std::string& selector);
    
    /**
     * @brief 附加 CSS 样式表
     * @param css CSS 字符串
     * @return 是否成功
     */
    bool AttachStyleSheet(const std::string& css);
    
    /**
     * @brief 创建元素
     * @param tag_name 标签名
     * @return 新创建的元素指针
     */
    LexborElement* CreateElement(const std::string& tag_name);
    
    /**
     * @brief 创建文本节点
     * @param text 文本内容
     * @return 新创建的文本节点指针
     */
    LexborText* CreateTextNode(const std::string& text);
    
    /**
     * @brief 获取底层的 Lexbor 文档指针
     * @return lxb_html_document_t 指针
     */
    lxb_html_document_t* GetNativeDocument() { return document_; }
    
    /**
     * @brief 获取 CSS 解析器
     * @return lxb_css_parser_t 指针
     */
    lxb_css_parser_t* GetCSSParser() { return css_parser_; }
    
    /**
     * @brief 获取选择器引擎
     * @return lxb_selectors_t 指针
     */
    lxb_selectors_t* GetSelectors() { return selectors_; }
    
    /**
     * @brief 序列化 DOM 树为 HTML 字符串
     * @return HTML 字符串
     */
    std::string SerializeToHTML();

    /**
     * @brief 序列化单个节点为 HTML 字符串
     * @param node Lexbor 节点指针
     * @return HTML 字符串
     */
    std::string SerializeNode(lxb_dom_node_t* node);

    /**
     * @brief 遍历 DOM 树
     * @param callback 回调函数，接收每个元素
     */
    void Walk(std::function<void(LexborElement*)> callback);

    /**
     * @brief 检查是否有解析错误
     * @return 是否有错误
     */
    bool HasErrors() const { return !errors_.empty(); }

    /**
     * @brief 获取所有错误信息
     * @return 错误信息列表
     */
    const std::vector<std::string>& GetErrors() const { return errors_; }

    /**
     * @brief 清空错误信息
     */
    void ClearErrors() { errors_.clear(); }

    /**
     * @brief 获取警告信息
     * @return 警告信息列表
     */
    const std::vector<std::string>& GetWarnings() const { return warnings_; }

    /**
     * @brief 检查是否有警告
     * @return 是否有警告
     */
    bool HasWarnings() const { return !warnings_.empty(); }

    /**
     * @brief 获取文档模式（quirks mode / standards mode）
     * @return 文档模式字符串
     */
    std::string GetDocumentMode() const;

    /**
     * @brief 检查是否为 quirks mode
     * @return 是否为 quirks mode
     */
    bool IsQuirksMode() const;

    /**
     * @brief 获取 DOCTYPE 信息
     * @return DOCTYPE 字符串
     */
    std::string GetDoctype() const;

private:
    /**
     * @brief 初始化 CSS 支持
     */
    bool InitializeCSS();
    
    /**
     * @brief 初始化选择器引擎
     */
    bool InitializeSelectors();
    
    /**
     * @brief 清理资源
     */
    void Cleanup();

private:
    /**
     * @brief 添加错误信息
     */
    void AddError(const std::string& error) { errors_.push_back(error); }

    /**
     * @brief 添加警告信息
     */
    void AddWarning(const std::string& warning) { warnings_.push_back(warning); }

private:
    lxb_html_document_t* document_;      // Lexbor HTML 文档
    lxb_css_parser_t* css_parser_;       // CSS 解析器
    lxb_selectors_t* selectors_;         // 选择器引擎
    bool css_initialized_;               // CSS 是否已初始化
    bool selectors_initialized_;         // 选择器是否已初始化
    std::vector<std::string> errors_;    // 错误信息列表
    std::vector<std::string> warnings_;  // 警告信息列表
};

/**
 * @brief Lexbor Element 包装类
 */
class LexborElement {
public:
    /**
     * @brief 构造函数
     * @param element Lexbor 元素指针
     * @param document 所属文档
     */
    LexborElement(lxb_dom_element_t* element, LexborDocument* document);
    
    /**
     * @brief 获取标签名
     * @return 标签名（小写）
     */
    std::string GetTagName() const;
    
    /**
     * @brief 获取属性值
     * @param name 属性名
     * @return 属性值，如果不存在返回空字符串
     */
    std::string GetAttribute(const std::string& name) const;
    
    /**
     * @brief 设置属性
     * @param name 属性名
     * @param value 属性值
     */
    void SetAttribute(const std::string& name, const std::string& value);
    
    /**
     * @brief 检查是否有某个属性
     * @param name 属性名
     * @return 是否存在
     */
    bool HasAttribute(const std::string& name) const;
    
    /**
     * @brief 移除属性
     * @param name 属性名
     */
    void RemoveAttribute(const std::string& name);
    
    /**
     * @brief 获取 ID
     * @return ID 值
     */
    std::string GetId() const { return GetAttribute("id"); }
    
    /**
     * @brief 设置 ID
     * @param id ID 值
     */
    void SetId(const std::string& id) { SetAttribute("id", id); }
    
    /**
     * @brief 获取 class 属性
     * @return class 值
     */
    std::string GetClassName() const { return GetAttribute("class"); }
    
    /**
     * @brief 设置 class 属性
     * @param class_name class 值
     */
    void SetClassName(const std::string& class_name) { SetAttribute("class", class_name); }
    
    /**
     * @brief 检查是否有某个 class
     * @param class_name class 名称
     * @return 是否存在
     */
    bool HasClass(const std::string& class_name) const;
    
    /**
     * @brief 添加 class
     * @param class_name class 名称
     */
    void AddClass(const std::string& class_name);
    
    /**
     * @brief 移除 class
     * @param class_name class 名称
     */
    void RemoveClass(const std::string& class_name);
    
    /**
     * @brief 获取 innerHTML
     * @return 元素内部的 HTML 字符串
     */
    std::string GetInnerHTML() const;

    /**
     * @brief 设置 innerHTML
     * @param html HTML 字符串
     */
    void SetInnerHTML(const std::string& html);

    /**
     * @brief 获取 textContent
     * @return 元素的文本内容
     */
    std::string GetTextContent() const;

    /**
     * @brief 设置 textContent
     * @param text 文本内容
     */
    void SetTextContent(const std::string& text);
    
    /**
     * @brief 获取父元素
     * @return 父元素指针，如果没有返回 nullptr
     */
    LexborElement* GetParentElement();
    
    /**
     * @brief 获取子元素列表
     * @return 子元素列表
     */
    std::vector<LexborElement*> GetChildren();
    
    /**
     * @brief 获取第一个子元素
     * @return 第一个子元素指针，如果没有返回 nullptr
     */
    LexborElement* GetFirstChild();
    
    /**
     * @brief 获取最后一个子元素
     * @return 最后一个子元素指针，如果没有返回 nullptr
     */
    LexborElement* GetLastChild();
    
    /**
     * @brief 添加子元素
     * @param child 子元素
     */
    void AppendChild(LexborElement* child);
    
    /**
     * @brief 移除子元素
     * @param child 子元素
     */
    void RemoveChild(LexborElement* child);
    
    /**
     * @brief 使用选择器查找子元素
     * @param selector CSS 选择器
     * @return 第一个匹配的元素，如果没有返回 nullptr
     */
    LexborElement* QuerySelector(const std::string& selector);
    
    /**
     * @brief 使用选择器查找所有子元素
     * @param selector CSS 选择器
     * @return 匹配的元素列表
     */
    std::vector<LexborElement*> QuerySelectorAll(const std::string& selector);
    
    /**
     * @brief 检查是否匹配选择器
     * @param selector CSS 选择器
     * @return 是否匹配
     */
    bool Matches(const std::string& selector);
    
    /**
     * @brief 查找最近的匹配选择器的祖先元素
     * @param selector CSS 选择器
     * @return 匹配的元素，如果没有返回 nullptr
     */
    LexborElement* Closest(const std::string& selector);
    
    /**
     * @brief 获取底层的 Lexbor 元素指针
     * @return lxb_dom_element_t 指针
     */
    lxb_dom_element_t* GetNativeElement() { return element_; }
    
    /**
     * @brief 获取所属文档
     * @return 文档指针
     */
    LexborDocument* GetDocument() { return document_; }

private:
    lxb_dom_element_t* element_;    // Lexbor 元素指针
    LexborDocument* document_;      // 所属文档
};

/**
 * @brief Lexbor Text 包装类
 */
class LexborText {
public:
    /**
     * @brief 构造函数
     * @param text Lexbor 文本节点指针
     * @param document 所属文档
     */
    LexborText(lxb_dom_text_t* text, LexborDocument* document);
    
    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    std::string GetData() const;
    
    /**
     * @brief 设置文本内容
     * @param data 文本内容
     */
    void SetData(const std::string& data);
    
    /**
     * @brief 获取底层的 Lexbor 文本节点指针
     * @return lxb_dom_text_t 指针
     */
    lxb_dom_text_t* GetNativeText() { return text_; }

private:
    lxb_dom_text_t* text_;          // Lexbor 文本节点指针
    LexborDocument* document_;      // 所属文档
};

} // namespace mbink

