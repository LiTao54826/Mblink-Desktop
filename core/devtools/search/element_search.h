/**
 * @file element_search.h
 * @brief 元素搜索组件
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "core/dom/document.h"
#include "core/dom/element.h"

namespace mblink {

/**
 * @brief 搜索类型
 */
enum class SearchType {
    CSSSelector,  // CSS 选择器搜索
    Text          // 文本搜索（tag, id, class）
};

/**
 * @brief 搜索结果
 */
struct SearchResult {
    std::vector<std::shared_ptr<Element>> elements;
    int current_index = -1;

    bool HasResults() const { return !elements.empty(); }
    size_t Count() const { return elements.size(); }
};

/**
 * @brief 元素搜索组件
 */
class ElementSearch {
public:
    explicit ElementSearch(Document* document);
    ~ElementSearch();

    /**
     * @brief 执行搜索
     */
    SearchResult Search(const std::string& query);

    /**
     * @brief 导航到下一个结果
     */
    void NextResult();

    /**
     * @brief 导航到上一个结果
     */
    void PreviousResult();

    /**
     * @brief 获取当前结果
     */
    std::shared_ptr<Element> GetCurrentResult() const;

    /**
     * @brief 获取当前搜索结果
     */
    const SearchResult& GetSearchResult() const { return current_result_; }

    /**
     * @brief 设置搜索类型
     */
    void SetSearchType(SearchType type) { search_type_ = type; }

    /**
     * @brief 获取搜索类型
     */
    SearchType GetSearchType() const { return search_type_; }

    /**
     * @brief 清除搜索结果
     */
    void Clear();

private:
    Document* document_;
    SearchType search_type_ = SearchType::CSSSelector;
    SearchResult current_result_;

    std::vector<std::shared_ptr<Element>> SearchBySelector(const std::string& selector);
    std::vector<std::shared_ptr<Element>> SearchByText(const std::string& text);
};

} // namespace mblink
