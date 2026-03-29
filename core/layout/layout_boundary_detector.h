/**
 * @file layout_boundary_detector.h
 * @brief 布局边界检测器
 *
 * 检测元素是否为布局边界，以及查找最近的布局边界祖先。
 * 布局边界内的变化不会影响外部元素的布局。
 *
 * 布局边界类型：
 * 1. 脱离文档流 (position: fixed/absolute)
 * 2. 滚动容器 (overflow: scroll/auto + 固定尺寸)
 * 3. 固定尺寸容器 (width + height 都是固定值)
 * 4. CSS Containment (contain: layout/strict/content)
 * 5. Flex 固定项 (flex: 0 0 <size>)
 */

#pragma once

#include <memory>

namespace mbink {

// 前向声明
class Element;
class Node;
class RenderObject;
struct ComputedStyle;

/**
 * @brief 布局边界检测器
 *
 * 提供静态方法检测元素是否为布局边界，
 * 以及查找最近的布局边界祖先。
 */
class LayoutBoundaryDetector {
public:
    /**
     * @brief 布局边界类型
     */
    enum class BoundaryType {
        None,            ///< 不是布局边界
        OutOfFlow,       ///< 脱离文档流 (fixed/absolute)
        ScrollContainer, ///< 滚动容器
        FixedSize,       ///< 固定尺寸容器
        CSSContainment,  ///< CSS Containment
        FlexFixed,       ///< Flex 固定项
    };

    /**
     * @brief 检测元素是否为布局边界
     * @param element 要检测的元素
     * @return 布局边界类型
     */
    static BoundaryType DetectBoundaryType(Element* element);

    /**
     * @brief 检测 RenderObject 是否为布局边界
     * @param render_object 要检测的渲染对象
     * @return 布局边界类型
     */
    static BoundaryType DetectBoundaryType(RenderObject* render_object);

    /**
     * @brief 检查样式是否表示脱离文档流
     * @param style 计算后的样式
     * @return 如果是 fixed 或 absolute 定位则返回 true
     */
    static bool IsOutOfFlow(const ComputedStyle& style);

    /**
     * @brief 检查样式是否表示滚动容器
     * @param style 计算后的样式
     * @return 如果 overflow 是 scroll 或 auto 则返回 true
     */
    static bool IsScrollContainer(const ComputedStyle& style);

    /**
     * @brief 检查样式是否有固定尺寸
     * @param style 计算后的样式
     * @return 如果 width 和 height 都是固定值则返回 true
     */
    static bool HasFixedSize(const ComputedStyle& style);

    /**
     * @brief 检查样式是否有 CSS Containment
     * @param style 计算后的样式
     * @return 如果有 layout containment 则返回 true
     */
    static bool HasLayoutContainment(const ComputedStyle& style);

    /**
     * @brief 检查 RenderObject 是否为 Flex 固定项
     * @param render_object 要检测的渲染对象
     * @return 如果是 flex: 0 0 <size> 则返回 true
     */
    static bool IsFlexFixedItem(RenderObject* render_object);

    /**
     * @brief 查找最近的布局边界祖先
     * @param node 起始节点
     * @return 最近的布局边界祖先，如果没有返回 nullptr
     */
    static Element* FindNearestLayoutBoundary(Node* node);

    /**
     * @brief 查找最近的布局边界 RenderObject
     * @param render_object 起始渲染对象
     * @return 最近的布局边界 RenderObject，如果没有返回 nullptr
     */
    static RenderObject* FindNearestLayoutBoundary(RenderObject* render_object);

    /**
     * @brief 将边界类型转换为字符串（用于调试）
     * @param type 边界类型
     * @return 类型的字符串表示
     */
    static const char* BoundaryTypeToString(BoundaryType type);
};

}  // namespace mbink
