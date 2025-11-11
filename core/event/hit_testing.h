/**
 * @file hit_testing.h
 * @brief Hit Testing - 检测鼠标点击位置对应的 DOM 元素
 */

#pragma once

#include <memory>
#include <vector>

namespace lightui {

// 前向声明
class Element;
class RenderObject;
class Document;

/**
 * @brief Hit Testing 结果
 */
struct HitTestResult {
    std::shared_ptr<Element> element;  // 被点击的元素
    float local_x;                      // 相对于元素的 X 坐标
    float local_y;                      // 相对于元素的 Y 坐标
    
    HitTestResult() : element(nullptr), local_x(0), local_y(0) {}
    
    bool IsValid() const { return element != nullptr; }
};

/**
 * @brief Hit Testing 引擎
 * 
 * 功能：
 * 1. 根据鼠标坐标查找对应的 DOM 元素
 * 2. 考虑元素层叠顺序（z-index）
 * 3. 考虑元素边界和布局
 */
class HitTesting {
public:
    /**
     * @brief 构造函数
     */
    HitTesting() = default;
    
    /**
     * @brief 析构函数
     */
    ~HitTesting() = default;
    
    /**
     * @brief 执行 Hit Testing
     * 
     * @param document 文档对象
     * @param x 鼠标 X 坐标（相对于视口）
     * @param y 鼠标 Y 坐标（相对于视口）
     * @return Hit Testing 结果
     */
    HitTestResult HitTest(std::shared_ptr<Document> document, float x, float y);
    
    /**
     * @brief 在渲染对象上执行 Hit Testing
     * 
     * @param render_object 渲染对象
     * @param x 鼠标 X 坐标（相对于视口）
     * @param y 鼠标 Y 坐标（相对于视口）
     * @param offset_x 累积的 X 偏移
     * @param offset_y 累积的 Y 偏移
     * @return Hit Testing 结果
     */
    HitTestResult HitTestRenderObject(
        std::shared_ptr<RenderObject> render_object,
        float x, float y,
        float offset_x = 0.0f,
        float offset_y = 0.0f);
    
    /**
     * @brief 检查点是否在元素边界内
     * 
     * @param render_object 渲染对象
     * @param x 鼠标 X 坐标（相对于视口）
     * @param y 鼠标 Y 坐标（相对于视口）
     * @param offset_x 累积的 X 偏移
     * @param offset_y 累积的 Y 偏移
     * @return true 如果点在元素边界内
     */
    bool IsPointInBounds(
        std::shared_ptr<RenderObject> render_object,
        float x, float y,
        float offset_x,
        float offset_y);

private:
    /**
     * @brief 递归查找最深层的被点击元素（渲染树版本）
     *
     * @param render_object 当前渲染对象
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param offset_x 累积的 X 偏移
     * @param offset_y 累积的 Y 偏移
     * @param result 输出结果
     * @return true 如果找到元素
     */
    bool HitTestRecursive(
        std::shared_ptr<RenderObject> render_object,
        float x, float y,
        float offset_x,
        float offset_y,
        HitTestResult& result);

    /**
     * @brief 递归查找最深层的被点击元素（DOM 树版本）
     *
     * @param element 当前元素
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param offset_x 累积的 X 偏移
     * @param offset_y 累积的 Y 偏移
     * @param result 输出结果
     * @return true 如果找到元素
     */
    bool HitTestElement(
        std::shared_ptr<Element> element,
        float x, float y,
        float offset_x,
        float offset_y,
        HitTestResult& result);
};

} // namespace lightui

