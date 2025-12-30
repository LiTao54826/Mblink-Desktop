/**
 * @file log_renderer.h
 * @brief 日志渲染器
 *
 * 继承自 VirtualScrollRenderer，实现日志专用渲染。
 */

#ifndef MBINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_
#define MBINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_

#include "log_buffer.h"
#include "log_search.h"
#include "../virtual_text/selection_manager.h"
#include "../virtual_text/virtual_scroll_renderer.h"

#include "include/core/SkColor.h"

namespace lightui {

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
    LogBuffer* buffer_ = nullptr;
    const std::vector<size_t>* filtered_indices_ = nullptr;
    const LogSearch* search_ = nullptr;
    const virtual_text::SelectionManager* selection_ = nullptr;
    LogViewConfig config_;

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

    /**
     * @brief 格式化时间戳
     */
    std::string FormatTimestamp(uint32_t timestamp) const;

    /**
     * @brief 渲染滚动条
     */
    void RenderScrollbar(SkCanvas* canvas, const SkRect& bounds);
};

}  // namespace lightui

#endif  // MBINK_DOM_ELEMENTS_LOGVIEW_LOG_RENDERER_H_
