/**
 * @file scrollbar_controller.h
 * @brief 滚动条控制器 - 负责滚动条的交互逻辑（非绘制）
 * 
 * 功能：
 * - 滚动条命中测试
 * - 滚动条拖动状态管理
 * - 滚动位置计算
 * 
 * 从 render_object.cpp 提取的滚动条逻辑代码
 */

#pragma once

#include <string>

namespace lightui {

/**
 * @brief 滚动条区域类型
 */
enum class ScrollbarHitArea {
    None,            ///< 不在滚动条区域
    HorizontalTrack, ///< 水平滚动条轨道
    HorizontalThumb, ///< 水平滚动条滑块
    VerticalTrack,   ///< 垂直滚动条轨道
    VerticalThumb    ///< 垂直滚动条滑块
};

/**
 * @brief 滚动条命中测试参数
 * 
 * 封装滚动条命中测试所需的所有参数
 */
struct ScrollbarHitTestParams {
    // 可见区域尺寸
    float visible_width = 0.0f;
    float visible_height = 0.0f;
    
    // 内容尺寸
    float content_width = 0.0f;
    float content_height = 0.0f;
    
    // 边框宽度
    float border_left = 0.0f;
    float border_right = 0.0f;
    float border_top = 0.0f;
    float border_bottom = 0.0f;
    
    // overflow 设置
    std::string overflow_x;
    std::string overflow_y;
};

/**
 * @brief 滚动条拖动参数
 * 
 * 封装滚动条拖动计算所需的参数
 */
struct ScrollbarDragParams {
    // 可见区域尺寸
    float visible_width = 0.0f;
    float visible_height = 0.0f;
    
    // 内容尺寸
    float content_width = 0.0f;
    float content_height = 0.0f;
    
    // 边框宽度
    float border_left = 0.0f;
    float border_right = 0.0f;
    float border_top = 0.0f;
    float border_bottom = 0.0f;
};

/**
 * @brief 滚动条控制器
 * 
 * 负责滚动条的交互逻辑，包括：
 * - 命中测试：检测鼠标是否在滚动条区域
 * - 拖动管理：处理滚动条拖动状态
 * - 滚动计算：根据拖动计算新的滚动位置
 */
class ScrollbarController {
public:
    /// 滚动条宽度常量
    static constexpr float kScrollbarWidth = 12.0f;
    
    /// 最小滑块尺寸
    static constexpr float kMinThumbSize = 30.0f;

    /**
     * @brief 构造函数
     */
    ScrollbarController() = default;

    /**
     * @brief 检测点是否在滚动条区域内
     * 
     * @param local_x 相对于元素的 X 坐标
     * @param local_y 相对于元素的 Y 坐标
     * @param params 命中测试参数
     * @return 滚动条区域类型
     */
    ScrollbarHitArea HitTestScrollbar(float local_x, float local_y, 
                                       const ScrollbarHitTestParams& params) const;

    /**
     * @brief 开始拖动滚动条
     * 
     * @param area 滚动条区域类型
     * @param mouse_x 鼠标 X 坐标
     * @param mouse_y 鼠标 Y 坐标
     * @param current_scroll_x 当前水平滚动位置
     * @param current_scroll_y 当前垂直滚动位置
     */
    void StartDrag(ScrollbarHitArea area, float mouse_x, float mouse_y,
                   float current_scroll_x, float current_scroll_y);

    /**
     * @brief 更新滚动条拖动
     * 
     * @param mouse_x 鼠标 X 坐标
     * @param mouse_y 鼠标 Y 坐标
     * @param params 拖动参数
     * @param out_scroll_x 输出：新的水平滚动位置
     * @param out_scroll_y 输出：新的垂直滚动位置
     * @return 是否有滚动位置变化
     */
    bool UpdateDrag(float mouse_x, float mouse_y,
                    const ScrollbarDragParams& params,
                    float& out_scroll_x, float& out_scroll_y);

    /**
     * @brief 结束滚动条拖动
     */
    void EndDrag();

    /**
     * @brief 检查是否正在拖动滚动条
     */
    bool IsDragging() const { return dragging_area_ != ScrollbarHitArea::None; }

    /**
     * @brief 获取正在拖动的滚动条类型
     */
    ScrollbarHitArea GetDraggingArea() const { return dragging_area_; }

    /**
     * @brief 获取滚动条宽度
     */
    static constexpr float GetScrollbarWidth() { return kScrollbarWidth; }

    /**
     * @brief 检查是否需要水平滚动条
     * 
     * @param content_width 内容宽度
     * @param visible_width 可见区域宽度
     * @param overflow_x overflow-x 样式值
     * @return 是否需要水平滚动条
     */
    static bool NeedsHorizontalScrollbar(float content_width, float visible_width,
                                          const std::string& overflow_x);

    /**
     * @brief 检查是否需要垂直滚动条
     * 
     * @param content_height 内容高度
     * @param visible_height 可见区域高度
     * @param overflow_y overflow-y 样式值
     * @return 是否需要垂直滚动条
     */
    static bool NeedsVerticalScrollbar(float content_height, float visible_height,
                                        const std::string& overflow_y);

private:
    /**
     * @brief 计算滑块尺寸和轨道长度
     * 
     * @param track_size 轨道尺寸
     * @param content_size 内容尺寸
     * @param visible_size 可见区域尺寸
     * @param out_thumb_size 输出：滑块尺寸
     * @param out_track_length 输出：滑块可移动的轨道长度
     */
    void CalculateThumbMetrics(float track_size, float content_size, float visible_size,
                               float& out_thumb_size, float& out_track_length) const;

    // 拖动状态
    ScrollbarHitArea dragging_area_ = ScrollbarHitArea::None;
    float drag_start_scroll_ = 0.0f;  ///< 拖动开始时的滚动位置
    float drag_start_mouse_ = 0.0f;   ///< 拖动开始时的鼠标位置
};

} // namespace lightui
