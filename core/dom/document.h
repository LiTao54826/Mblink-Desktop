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
#include "dirty_node_tracker.h"
#include <string>
#include <memory>
#include <unordered_map>

// 前向声明
namespace lightui {
    class LexborDocument;
    class StyleManager;
    class QuickJSRuntime;
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

    /**
     * @brief 获取 head 元素
     * @return head 元素
     */
    std::shared_ptr<Element> GetHead() const { return head_; }

    /**
     * @brief 设置 head 元素
     * @param head head 元素
     */
    void SetHead(std::shared_ptr<Element> head);

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

    /**
     * @brief 递归注销元素及其所有后代的 ID
     * @param element 要注销的元素
     */
    void UnregisterElementAndDescendantIds(std::shared_ptr<Element> element);

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

    // ========== 脏节点追踪器 ==========

    /**
     * @brief 获取脏节点追踪器
     * @return 脏节点追踪器引用
     */
    DirtyNodeTracker& GetDirtyTracker() { return dirty_tracker_; }

    /**
     * @brief 获取脏节点追踪器（const 版本）
     * @return 脏节点追踪器常量引用
     */
    const DirtyNodeTracker& GetDirtyTracker() const { return dirty_tracker_; }

    // ========== 批量更新API (Week 2 - Task 2.3) ==========

    /**
     * @brief 开始批量更新
     *
     * 在批量更新期间，DOM变化不会立即触发重绘，
     * 而是累积脏区域，直到EndBatch()被调用。
     * 这对于执行多个DOM操作时非常有用，可以避免多次重绘。
     *
     * 示例:
     * @code
     * doc->BeginBatch();
     * for (int i = 0; i < 1000; i++) {
     *     element->SetStyle("width", std::to_string(i) + "px");
     * }
     * doc->EndBatch();  // 只触发一次重绘
     * @endcode
     */
    void BeginBatch();

    /**
     * @brief 结束批量更新
     *
     * 结束批量更新并触发单次重绘。
     * 如果没有调用BeginBatch()，此方法无效。
     */
    void EndBatch();

    /**
     * @brief 检查是否在批量更新中
     * @return true表示在批量更新中
     */
    bool IsInBatch() const { return batch_depth_ > 0; }

    /**
     * @brief 获取样式管理器
     * @return 样式管理器指针
     */
    StyleManager* GetStyleManager() const;

    // ========== JavaScript 运行时集成 ==========

    /**
     * @brief 设置 JavaScript 运行时
     * @param runtime QuickJS 运行时指针
     * 
     * 设置后，LoadHTML 会自动执行内联脚本
     */
    void SetJSRuntime(QuickJSRuntime* runtime) { js_runtime_ = runtime; }

    /**
     * @brief 获取 JavaScript 运行时
     * @return QuickJS 运行时指针
     */
    QuickJSRuntime* GetJSRuntime() const { return js_runtime_; }

    /**
     * @brief 执行文档中的所有内联脚本
     * 
     * 按照 DOM 顺序执行所有未执行的 <script> 标签
     * 支持 type="text/javascript" 和 type="module"
     */
    void ExecuteScripts();

    /**
     * @brief 执行文档中的所有内联脚本（使用指定运行时）
     * @param runtime QuickJS 运行时指针
     */
    void ExecuteScripts(QuickJSRuntime* runtime);

    // ========== 资源加载 ==========

    /**
     * @brief 设置文档的基础路径
     * @param base_path 基础路径（通常是 HTML 文件所在目录）
     *
     * 用于解析相对路径的外部资源（脚本、样式表、图片等）
     */
    void SetBasePath(const std::string& base_path) { base_path_ = base_path; }

    /**
     * @brief 获取文档的基础路径
     * @return 基础路径
     */
    const std::string& GetBasePath() const { return base_path_; }

    /**
     * @brief 解析资源路径
     * @param path 相对路径或绝对路径
     * @return 完整的文件路径
     */
    std::string ResolvePath(const std::string& path) const;

    /**
     * @brief 读取外部文件内容
     * @param path 文件路径（相对于基础路径或绝对路径）
     * @return 文件内容，读取失败返回空字符串
     */
    std::string ReadExternalFile(const std::string& path) const;

    /**
     * @brief 资源提供者类型
     */
    using FileAssetProvider = std::function<bool(const std::string&, std::vector<uint8_t>&)>;

    /**
     * @brief 设置资源提供者（用于从嵌入资源加载文件）
     */
    static void SetAssetProvider(FileAssetProvider provider);

    /**
     * @brief 获取资源提供者
     */
    static FileAssetProvider GetAssetProvider();

    /**
     * @brief 加载外部样式表
     *
     * 解析所有 <link rel="stylesheet"> 标签并加载外部 CSS 文件
     */
    void LoadExternalStylesheets();

    // ========== 焦点管理 ==========

    /**
     * @brief 获取当前活动元素（焦点元素）
     * @return 当前焦点元素，如果没有则返回 body
     */
    std::shared_ptr<Element> GetActiveElement() const;

    /**
     * @brief 设置当前活动元素
     * @param element 焦点元素（nullptr 表示清除焦点）
     */
    void SetActiveElement(std::shared_ptr<Element> element);

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
    std::shared_ptr<Element> head_;
    std::shared_ptr<Element> body_;
    std::unordered_map<std::string, std::weak_ptr<Element>> id_map_;
    DOMObserverManager observer_manager_;
    DirtyNodeTracker dirty_tracker_;  // 脏节点追踪器

    // Lexbor 集成
    std::unique_ptr<LexborDocument> lexbor_doc_;
    bool lexbor_dirty_;  // 标记 MBink DOM 是否已修改，需要同步到 Lexbor

    // 批量更新 (Week 2 - Task 2.3)
    int batch_depth_ = 0;  // 批量更新嵌套深度（支持嵌套BeginBatch/EndBatch）

    // 样式管理器
    std::unique_ptr<StyleManager> style_manager_;

    // JavaScript 运行时
    QuickJSRuntime* js_runtime_ = nullptr;

    // 焦点管理
    std::weak_ptr<Element> active_element_;

    // 脏区域收集（用于移动元素双区域标记优化）
    std::vector<SkRect> dirty_rects_;

    // 资源加载基础路径
    std::string base_path_;

    // 静态资源提供者
    static FileAssetProvider asset_provider_;

public:
    // ========== 脏区域管理 ==========

    /**
     * @brief 添加脏矩形区域
     * @param rect 需要重绘的区域
     * 
     * 用于元素移动时标记新旧两个位置都需要重绘。
     * 该方法会自动合并重叠区域以减少重绘开销。
     */
    void AddDirtyRect(const SkRect& rect);

    /**
     * @brief 获取合并后的脏矩形区域
     * @return 所有脏区域的并集
     */
    SkRect GetMergedDirtyRect() const;

    /**
     * @brief 获取所有脏区域列表
     * @return 脏区域列表的引用
     */
    const std::vector<SkRect>& GetDirtyRects() const { return dirty_rects_; }

    /**
     * @brief 清除脏区域
     */
    void ClearDirtyRects() { dirty_rects_.clear(); }
};

} // namespace lightui
