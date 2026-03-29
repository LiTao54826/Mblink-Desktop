/**
 * @file clipboard_manager.h
 * @brief 剪贴板管理器
 *
 * 功能：
 * - 管理系统剪贴板访问
 * - 处理复制、剪切、粘贴操作
 * - 分发剪贴板事件
 *
 * 参考：
 * - W3C Clipboard API
 * - MDN Web Docs - Clipboard
 */

#pragma once

#include <memory>
#include <string>

namespace mbink {

// 前向声明
class Document;
class Element;
class Node;
class SelectionManager;
class ContentEditableHandler;

/**
 * @brief 剪贴板管理器
 *
 * 负责处理剪贴板相关操作，包括复制、剪切、粘贴，
 * 以及与系统剪贴板的交互。
 */
class ClipboardManager {
public:
    /**
     * @brief 构造函数
     * @param selection_manager 选择管理器
     * @param editable_handler 可编辑内容处理器
     */
    ClipboardManager(SelectionManager* selection_manager,
                     ContentEditableHandler* editable_handler);

    /**
     * @brief 析构函数
     */
    ~ClipboardManager();

    // ========== 剪贴板操作 ==========

    /**
     * @brief 复制选中内容到剪贴板
     * @param document 文档
     * @return true 如果复制成功
     */
    bool Copy(std::shared_ptr<Document> document);

    /**
     * @brief 剪切选中内容到剪贴板
     * @param document 文档
     * @return true 如果剪切成功
     */
    bool Cut(std::shared_ptr<Document> document);

    /**
     * @brief 从剪贴板粘贴内容
     * @param document 文档
     * @return true 如果粘贴成功
     */
    bool Paste(std::shared_ptr<Document> document);

    // ========== 系统剪贴板访问 ==========

    /**
     * @brief 获取剪贴板文本
     * @return 剪贴板中的文本内容
     */
    std::string GetText() const;

    /**
     * @brief 设置剪贴板文本
     * @param text 要设置的文本
     */
    void SetText(const std::string& text);

    // ========== 事件处理 ==========

    /**
     * @brief 处理复制事件
     * @param target 目标元素
     * @return true 如果事件被处理
     */
    bool HandleCopyEvent(std::shared_ptr<Element> target);

    /**
     * @brief 处理剪切事件
     * @param target 目标元素
     * @return true 如果事件被处理
     */
    bool HandleCutEvent(std::shared_ptr<Element> target);

    /**
     * @brief 处理粘贴事件
     * @param target 目标元素
     * @return true 如果事件被处理
     */
    bool HandlePasteEvent(std::shared_ptr<Element> target);

    // ========== 键盘快捷键处理 ==========

    /**
     * @brief 处理键盘快捷键
     * @param document 文档
     * @param key_code 键码
     * @param ctrl_key 是否按下 Ctrl
     * @param meta_key 是否按下 Meta (Cmd)
     * @return true 如果快捷键被处理
     */
    bool HandleKeyboardShortcut(std::shared_ptr<Document> document,
                                int key_code,
                                bool ctrl_key,
                                bool meta_key);

private:
    /**
     * @brief 分发剪贴板事件
     * @param target 目标元素
     * @param type 事件类型（"copy", "cut", "paste"）
     * @return true 如果事件未被取消
     */
    bool DispatchClipboardEvent(std::shared_ptr<Element> target,
                                const std::string& type);

    /**
     * @brief 获取选中的文本
     * @param document 文档
     * @return 选中的文本
     */
    std::string GetSelectedText(std::shared_ptr<Document> document);

    /**
     * @brief 查找包含节点的可编辑元素
     * @param node 节点
     * @return 可编辑元素，如果没有返回 nullptr
     */
    std::shared_ptr<Element> FindEditableElement(std::shared_ptr<Node> node);

private:
    SelectionManager* selection_manager_;
    ContentEditableHandler* editable_handler_;

    /// 内部剪贴板缓存（用于不支持系统剪贴板的平台）
    std::string clipboard_text_;
};

} // namespace mbink
