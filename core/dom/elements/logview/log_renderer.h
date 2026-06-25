/**
 * @file log_renderer.h
 * @brief 日志渲染器
 *
 * 继承自 VirtualScrollRenderer，实现日志专用渲染。
 */

#ifndef MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_
#define MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_

#include "log_buffer.h"
#include "log_search.h"
#include "../virtual_text/selection_manager.h"
#include "../virtual_text/virtual_scroll_renderer.h"
#include "core/render/utils/paint.h"

#include "include/core/SkColor.h"

#include <string_view>

namespace mblink {

/**
 * @brief 日志视图配置
 */
struct LogViewConfig {
    bool show_timestamp = true;   ///< 显示时间戳
    bool show_level = true;       ///< 显示级别
    bool show_source = true;      ///< 显示源名
    std::string timestamp_format = "%H:%M:%S.%f";

    /// 级别颜色
    SkColor level_colors[5] = {
        0xFF888888,  // DEBUG - 灰色
        0xFFCCCCCC,  // INFO - 浅灰
        0xFFFFCC00,  // WARN - 黄色
        0xFFFF4444,  // ERROR - 红色
        0xFFFF0000,  // FATAL - 亮红色
    };

    /// 背景色
    SkColor background_color = 0xFF1E1E1E;

    /// 选中背景色
    SkColor selection_color = 0xFF264F78;

    /// 搜索匹配背景色
    SkColor match_color = 0xFF5A5A00;

    /// 当前匹配背景色
    SkColor current_match_color = 0xFF806000;

    /// 时间戳颜色
    SkColor timestamp_color = 0xFF569CD6;

    /// 源名颜色
    SkColor source_color = 0xFF9CDCFE;
};

/**
 * @brief 日志渲染器
 *
 * 特点：
 * - 虚拟滚动：只渲染可见行
 * - 级别着色：不同级别不同颜色
 * - 搜索高亮：匹配文本高亮显示
 * - 时间戳格式化：可配置格式
 */
class LogRenderer : public VirtualScrollRenderer {
public:
    LogRenderer();
    ~LogRenderer() override = default;

    // === 配置 ===

    /**
     * @brief 设置配置
     */
    void SetConfig(const LogViewConfig& config);
    void SetFont(sk_sp<SkTypeface> typeface, float size);

    /**
     * @brief 获取配置
     */
    const LogViewConfig& config() const { return config_; }

    // === 数据绑定 ===

    /**
     * @brief 设置日志缓冲区
     */
    void SetBuffer(LogBuffer* buffer);

    /**
     * @brief 设置过滤后的索引
     */
    void SetFilteredIndices(const std::vector<size_t>* indices);
    void InvalidateWidthCache();
    void UpdateCachedWidthForEntry(size_t log_index);
    void UpdateScrollMetricsForBounds(const SkRect& bounds);
    void UpdateLineMetricsForBounds(const SkRect& bounds);
    void UpdateLineMetricsForBounds(const SkRect& bounds, float known_content_width);
    void UpdateLineMetricsForEntryBounds(const SkRect& bounds, size_t log_index);

    /**
     * @brief 设置搜索器（用于高亮）
     */
    void SetSearch(const LogSearch* search);

    /**
     * @brief 设置选择管理器（用于选中高亮）
     */
    void SetSelection(const virtual_text::SelectionManager* selection);

    // === 渲染 ===

    /**
     * @brief 渲染日志视图
     */
    void Render(SkCanvas* canvas, const SkRect& bounds) override;

    // === 查询 ===

    /**
     * @brief 获取显示的行数（考虑过滤）
     */
    int GetDisplayLineCount() const;

    /**
     * @brief 将显示行索引转换为日志索引
     */
    size_t DisplayIndexToLogIndex(int display_index) const;

private:
    struct LayoutResult {
        SkRect content_bounds = SkRect::MakeEmpty();
        bool need_vertical_scrollbar = false;
        bool need_horizontal_scrollbar = false;
    };

    LogBuffer* buffer_ = nullptr;
    const std::vector<size_t>* filtered_indices_ = nullptr;
    const LogSearch* search_ = nullptr;
    const virtual_text::SelectionManager* selection_ = nullptr;
    LogViewConfig config_;
    mutable float cached_max_content_width_ = 0.0f;
    mutable bool max_content_width_dirty_ = true;

    /**
     * @brief 渲染单行日志
     */
    void RenderLine(SkCanvas* canvas, size_t log_index,
                    const SkRect& line_rect, bool is_current_match);

    /**
     * @brief 渲染时间戳
     */
    float RenderTimestamp(SkCanvas* canvas, uint32_t timestamp,
                          float x, float y);

    /**
     * @brief 渲染级别标签
     */
    float RenderLevel(SkCanvas* canvas, LogLevel level,
                      float x, float y);

    /**
     * @brief 渲染源名
     */
    float RenderSource(SkCanvas* canvas, const std::string& source,
                       float x, float y);

    /**
     * @brief 渲染消息（带搜索高亮）
     */
    void RenderMessage(SkCanvas* canvas, size_t log_index,
                       std::string_view message, LogLevel level,
                       float x, float y, float max_width);

    float DrawTextRun(SkCanvas* canvas, std::string_view text,
                      float x, float y, const SkFont& font,
                      const Paint& paint) const;
    float MeasureTextRun(std::string_view text, const SkFont& font) const;

    /**
     * @brief 计算单条日志的渲染宽度
     */
    float ComputeEntryWidth(size_t log_index) const;

    /**
     * @brief 计算内容的最大渲染宽度
     */
    float ComputeMaxContentWidth() const;
    LayoutResult UpdateLayoutMetrics(const SkRect& bounds);

    /**
     * @brief 格式化时间戳
     */
    std::string FormatTimestamp(uint32_t timestamp) const;

    /**
     * @brief 渲染滚动条
     * @param canvas 画布
     * @param content_bounds 实际内容区域
     * @param has_horizontal_scrollbar 是否存在横向滚动条
     */
    void RenderScrollbar(SkCanvas* canvas, const SkRect& content_bounds,
                         bool has_horizontal_scrollbar);

    /**
     * @brief 渲染横向滚动条
     * @param canvas 画布
     * @param content_bounds 实际内容区域
     * @param has_vertical_scrollbar 是否存在纵向滚动条
     */
    void RenderHorizontalScrollbar(SkCanvas* canvas, const SkRect& content_bounds,
                                   bool has_vertical_scrollbar);
};

}  // namespace mblink

#endif  // MBLINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_
