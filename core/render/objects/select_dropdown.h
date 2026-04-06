/**
 * @file select_dropdown.h
 * @brief Select 下拉菜单管理器
 */

#pragma once

#include "core/dom/elements/html_select_element.h"
#include "render_object.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkRect.h"
#include <memory>

namespace mbink {

/**
 * @brief Select 下拉菜单信息
 */
struct SelectDropdownInfo {
    std::weak_ptr<HTMLSelectElement> select_element;  // 关联的 select 元素
    SkRect trigger_rect;                              // 触发器区域（select 元素的位置）
    SkRect dropdown_rect;                             // 下拉菜单区域
    SkRect viewport_rect;                             // 视口区域
    float content_height = 0.0f;                      // 下拉内容总高度
    float scroll_offset = 0.0f;                       // 当前滚动偏移
    bool open_above = false;                          // 是否向上展开
    bool is_open = false;                             // 是否打开
};

/**
 * @brief Select 下拉菜单管理器（单例）
 *
 * 管理所有 select 元素的下拉菜单状态和绘制
 */
class SelectDropdownManager {
public:
    /**
     * @brief 获取单例实例
     */
    static SelectDropdownManager& Instance();

    /**
     * @brief 打开下拉菜单
     * @param select select 元素
     * @param trigger_rect 触发器区域（select 元素的屏幕位置）
     */
    void OpenDropdown(std::shared_ptr<HTMLSelectElement> select, const SkRect& trigger_rect);

    /**
     * @brief 关闭当前打开的下拉菜单
     */
    void CloseDropdown();

    /**
     * @brief 检查是否有下拉菜单打开
     */
    bool IsDropdownOpen() const { return current_dropdown_.is_open; }

    /**
     * @brief 获取当前打开的 select 元素
     */
    std::shared_ptr<HTMLSelectElement> GetActiveSelect() const;

    /**
     * @brief 绘制下拉菜单
     * @param canvas 画布
     */
    void Paint(SkCanvas* canvas);

    /**
     * @brief 处理鼠标移动（更新悬停状态）
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @return 是否在下拉菜单区域内
     */
    bool HandleMouseMove(float x, float y);

    /**
     * @brief 处理鼠标点击
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @return 是否处理了点击（点击在下拉菜单内）
     */
    bool HandleClick(float x, float y);

    /**
     * @brief 处理滚轮滚动
     * @param delta_y 滚轮 Y 增量
     * @return 是否消费事件
     */
    bool HandleWheel(float delta_y);

    /**
     * @brief 检查点是否在下拉菜单区域内
     * @param x X 坐标
     * @param y Y 坐标
     */
    bool HitTest(float x, float y) const;

    /**
     * @brief 获取下拉菜单区域
     */
    const SkRect& GetDropdownRect() const { return current_dropdown_.dropdown_rect; }

    /**
     * @brief 更新下拉菜单位置（跟踪 select 元素位置变化）
     * @param new_trigger_rect 新的触发器区域
     */
    void UpdatePosition(const SkRect& new_trigger_rect);

    /**
     * @brief 从渲染树更新下拉菜单位置
     * @param root_render 渲染树根节点
     */
    void UpdatePositionFromRenderTree(std::shared_ptr<RenderObject> root_render);

private:
    SelectDropdownManager() = default;
    ~SelectDropdownManager() = default;
    SelectDropdownManager(const SelectDropdownManager&) = delete;
    SelectDropdownManager& operator=(const SelectDropdownManager&) = delete;

    SelectDropdownInfo current_dropdown_;

    // 下拉菜单样式常量 (Chrome 风格)
    static constexpr float ITEM_HEIGHT = 20.0f;
    static constexpr float ITEM_PADDING_X = 12.0f;
    static constexpr float OPTGROUP_INDENT = 16.0f;
    static constexpr float DROPDOWN_MAX_HEIGHT = 320.0f;
    static constexpr float DROPDOWN_SHADOW_BLUR = 12.0f;
    static constexpr float SCROLLBAR_WIDTH = 8.0f;
    static constexpr float VIEWPORT_MARGIN = 4.0f;
};

} // namespace mbink

