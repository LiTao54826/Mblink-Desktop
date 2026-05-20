/**
 * @file html_logview_element.h
 * @brief 日志视图 DOM 元素
 *
 * 提供 <logview> 标签的完整实现。
 */

#ifndef MBINK_DOM_ELEMENTS_LOGVIEW_HTML_LOGVIEW_ELEMENT_H_
#define MBINK_DOM_ELEMENTS_LOGVIEW_HTML_LOGVIEW_ELEMENT_H_

#include "log_buffer.h"
#include "log_filter.h"
#include "log_renderer.h"
#include "log_search.h"
#include "../virtual_text/selection_manager.h"
#include "core/dom/element.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// 前向声明
class SkCanvas;

namespace mbink {

/**
 * @brief 日志视图元素
 *
 * HTML 用法:
 * ```html
 * <logview max-entries="100000" auto-scroll="true"></logview>
 * ```
 *
 * JavaScript API:
 * ```javascript
 * const log = document.querySelector('logview');
 * log.append('INFO', 'app', 'Message');
 * log.setFilter({ levels: ['ERROR', 'WARN'] });
 * log.search('error');
 * ```
 */
class HTMLLogViewElement : public Element {
public:
    HTMLLogViewElement();
    ~HTMLLogViewElement() override;

    // === DOM 属性 ===

    int max_entries() const;
    void set_max_entries(int value);

    bool auto_scroll() const { return auto_scroll_; }
    void set_auto_scroll(bool value) { auto_scroll_ = value; }

    bool show_timestamp() const;
    void set_show_timestamp(bool value);

    bool show_level() const;
    void set_show_level(bool value);

    bool show_source() const;
    void set_show_source(bool value);

    // === 日志操作 ===

    /**
     * @brief 追加日志
     * @param level 级别字符串 (DEBUG/INFO/WARN/ERROR/FATAL)
     * @param source 源名
     * @param message 消息内容
     */
    void Append(const std::string& level, const std::string& source,
                const std::string& message);

    /**
     * @brief 追加日志（使用枚举级别）
     */
    void Append(LogLevel level, const std::string& source,
                const std::string& message);

    /**
     * @brief 清空所有日志
     */
    void Clear();

    /**
     * @brief 获取日志数量
     */
    size_t GetLogCount() const;

    int scroll_offset_for_test() const { return renderer_->scroll_offset(); }
    int max_scroll_offset_for_test() const { return renderer_->max_scroll_offset(); }
    int max_horizontal_scroll_offset_for_test() const {
        return renderer_->max_horizontal_scroll_offset();
    }

    // === 滚动控制 ===

    /**
     * @brief 滚动到指定行
     */
    void ScrollTo(int line);

    /**
     * @brief 滚动到底部
     */
    void ScrollToBottom();

    /**
     * @brief 滚动到顶部
     */
    void ScrollToTop();

    // === 过滤 ===

    /**
     * @brief 设置级别过滤
     * @param levels 允许的级别列表
     */
    void SetLevelFilter(const std::vector<std::string>& levels);

    /**
     * @brief 设置源名过滤
     * @param sources 允许的源名列表
     */
    void SetSourceFilter(const std::vector<std::string>& sources);

    /**
     * @brief 清除所有过滤
     */
    void ClearFilter();

    // === 搜索 ===

    /**
     * @brief 执行搜索
     * @param query 搜索关键词
     * @param use_regex 是否使用正则表达式
     * @return 匹配数量
     */
    int Search(const std::string& query, bool use_regex = false);

    /**
     * @brief 跳转到下一个匹配
     */
    void NextMatch();

    /**
     * @brief 跳转到上一个匹配
     */
    void PrevMatch();

    /**
     * @brief 清除搜索
     */
    void ClearSearch();

    /**
     * @brief 获取匹配数量
     */
    int GetMatchCount() const;

    /**
     * @brief 获取当前匹配索引
     */
    int GetCurrentMatch() const;

    // === 选择 ===

    /**
     * @brief 获取选中的文本
     */
    std::string GetSelectedText() const;

    /**
     * @brief 复制选中内容到剪贴板
     */
    void CopySelection();

    /**
     * @brief 全选
     */
    void SelectAll();

    // === 导出 ===

    /**
     * @brief 导出日志
     * @param format 格式 ("text" 或 "json")
     * @return 导出的内容
     */
    std::string Export(const std::string& format = "text") const;

    // === 渲染 ===

    /**
     * @brief 渲染日志视图
     * @param canvas Skia 画布
     * @param x 左上角 X
     * @param y 左上角 Y
     * @param width 宽度
     * @param height 高度
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    /**
     * @brief 设置字体
     */
    void SetFont(const std::string& family, float size);

    // === 事件处理 ===

    /**
     * @brief 处理鼠标按下
     */
    void OnMouseDown(float x, float y, int button, int click_count);

    /**
     * @brief 处理鼠标移动
     */
    void OnMouseMove(float x, float y);

    /**
     * @brief 处理鼠标释放
     */
    void OnMouseUp(float x, float y, int button);

    /**
     * @brief 处理滚轮
     */
    void OnWheel(float delta_y, bool horizontal = false);

    /**
     * @brief 处理键盘事件
     */
    void OnKeyDown(const std::string& key, bool ctrl, bool shift);

    /**
     * @brief 检查是否正在选择
     */
    bool IsSelecting() const { return selection_.IsSelecting(); }

    // === 回调 ===

    using SelectionCallback = std::function<void(const std::string&)>;
    void SetSelectionCallback(SelectionCallback callback);

private:
    std::unique_ptr<LogBuffer> buffer_;
    std::unique_ptr<LogRenderer> renderer_;
    std::unique_ptr<LogFilter> filter_;
    std::unique_ptr<LogSearch> search_;
    virtual_text::SelectionManager selection_;

    bool auto_scroll_ = true;
    std::vector<size_t> filtered_indices_;
    bool filter_dirty_ = false;

    // 视图状态
    float view_x_ = 0;
    float view_y_ = 0;
    float view_width_ = 0;
    float view_height_ = 0;
    bool is_dragging_horizontal_scrollbar_ = false;
    float drag_start_x_ = 0.0f;
    int drag_start_horizontal_offset_ = 0;
    int last_drag_horizontal_offset_ = -1;

    SelectionCallback selection_callback_;

    /**
     * @brief 更新过滤索引
     */
    void UpdateFilteredIndices();

    /**
     * @brief 检查是否在底部
     */
    bool IsAtBottom();

    /**
     * @brief 坐标转换：屏幕坐标到行列
     */
    std::pair<int, int> ScreenToLineCol(float x, float y) const;
};

}  // namespace mbink

#endif  // MBINK_DOM_ELEMENTS_LOGVIEW_HTML_LOGVIEW_ELEMENT_H_
