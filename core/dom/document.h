/**
 * @file document.h
 * @brief Document 类
 *
 * 功能：
 * - 表示整个 DOM 文档
 * - 提供创建节点的工厂方法
 * - 管理全局 ID 映射
 * - 提供查询方法
 */

#pragma once

#include "node.h"
#include "element.h"
#include "text.h"
#include "dom_observer.h"
#include <string>
#include <memory>
#include <unordered_map>

// 前向声明
namespace lightui {
    class LexborDocument;
}

namespace lightui {

/**
 * @brief Document 类
 */
class Document : public Node {
public:
    /**
     * @brief 构造函数
     */
    Document();

    /**
     * @brief 析构函数
     */
    ~Document() override;

    /**
     * @brief 初始化文档（创建基本 DOM 结构）
     * 必须在对象被 shared_ptr 管理后调用
     */
    void Initialize();

    // ========== 工厂方法 ==========

    /**
     * @brief 创建元素节点
     * @param tag_name 标签名
     * @return 元素节点
     */
    std::shared_ptr<Element> CreateElement(const std::string& tag_name);

    /**
     * @brief 创建文本节点
     * @param data 文本数据
     * @return 文本节点
     */
    std::shared_ptr<Text> CreateTextNode(const std::string& data);

    // ========== 文档属性 ==========

    /**
     * @brief 获取文档元素（根元素）
     * @return 文档元素
     */
    std::shared_ptr<Element> GetDocumentElement() const { return document_element_; }

    /**
     * @brief 获取 body 元素
     * @return body 元素
     */
    std::shared_ptr<Element> GetBody() const { return body_; }

    /**
     * @brief 设置 body 元素
     * @param body body 元素
     */
    void SetBody(std::shared_ptr<Element> body);

    // ========== 查询方法 ==========

    /**
     * @brief 根据 ID 查找元素
     * @param id 元素 ID
     * @return 元素，如果未找到返回 nullptr
     */
    std::shared_ptr<Element> GetElementById(const std::string& id);

    /**
     * @brief 根据标签名查找所有元素
     * @param tag_name 标签名
     * @return 元素列表
     */
    std::vector<std::shared_ptr<Element>> GetElementsByTagName(const std::string& tag_name);

    /**
     * @brief 根据 class 名查找所有元素
     * @param class_name class 名
     * @return 元素列表
     */
    std::vector<std::shared_ptr<Element>> GetElementsByClassName(const std::string& class_name);

    // ========== Lexbor 集成 ==========

    /**
     * @brief 从 HTML 字符串加载文档
     * @param html HTML 字符串
     * @return 是否成功
     */
    bool LoadHTML(const std::string& html);

    /**
     * @brief 从 HTML 文件加载文档
     * @param file_path 文件路径
     * @return 是否成功
     */
    bool LoadHTMLFile(const std::string& file_path);

    /**
     * @brief 保存文档为 HTML 字符串
     * @return HTML 字符串
     */
    std::string SaveHTML();

    /**
     * @brief 从 Lexbor DOM 同步到 MBink DOM
     */
    void SyncFromLexbor();

    /**
     * @brief 从 MBink DOM 同步到 Lexbor DOM
     */
    void SyncToLexbor();

    /**
     * @brief 标记 Lexbor DOM 为脏（需要同步）
     */
    void MarkLexborDirty() { lexbor_dirty_ = true; }

    /**
     * @brief 获取 Lexbor 文档实例
     * @return Lexbor 文档指针
     */
    LexborDocument* GetLexborDocument() { return lexbor_doc_.get(); }

    // ========== ID 映射管理 ==========

    /**
     * @brief 注册元素 ID
     * @param id 元素 ID
     * @param element 元素
     */
    void RegisterElementId(const std::string& id, std::shared_ptr<Element> element);

    /**
     * @brief 注销元素 ID
     * @param id 元素 ID
     */
    void UnregisterElementId(const std::string& id);

    // ========== DOM 观察者 ==========

    /**
     * @brief 获取观察者管理器
     * @return 观察者管理器引用
     */
    DOMObserverManager& GetObserverManager() { return observer_manager_; }

    /**
     * @brief 添加 DOM 观察者
     * @param observer 观察者指针
     */
    void AddObserver(DOMObserver* observer) {
        observer_manager_.AddObserver(observer);
    }

    /**
     * @brief 移除 DOM 观察者
     * @param observer 观察者指针
     */
    void RemoveObserver(DOMObserver* observer) {
        observer_manager_.RemoveObserver(observer);
    }

    // ========== Node 接口实现 ==========

    /**
     * @brief 克隆节点
     * @param deep 是否深度克隆
     * @return 克隆的节点
     */
    std::shared_ptr<Node> CloneNode(bool deep) override;

private:
    /**
     * @brief 递归收集指定标签名的元素
     */
    void CollectElementsByTagName(std::shared_ptr<Node> node,
                                   const std::string& tag_name,
                                   std::vector<std::shared_ptr<Element>>& result);

    /**
     * @brief 递归收集指定 class 名的元素
     */
    void CollectElementsByClassName(std::shared_ptr<Node> node,
                                     const std::string& class_name,
                                     std::vector<std::shared_ptr<Element>>& result);

    /**
     * @brief 重建 ID 映射表
     */
    void RebuildIdMap(std::shared_ptr<Element> root);

private:
    std::shared_ptr<Element> document_element_;
    std::shared_ptr<Element> body_;
    std::unordered_map<std::string, std::weak_ptr<Element>> id_map_;
    DOMObserverManager observer_manager_;

    // Lexbor 集成
    std::unique_ptr<LexborDocument> lexbor_doc_;
    bool lexbor_dirty_;  // 标记 MBink DOM 是否已修改，需要同步到 Lexbor
};

} // namespace lightui
