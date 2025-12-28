/**
 * @file selection_manager.h
 * @brief 选择管理器 - 管理文本选择状态和光标
 *
 * 功能：
 * - 管理每个文档的 Selection 对象
 * - 处理鼠标和键盘选择操作
 * - 计算光标位置（hit testing）
 * - 渲染光标和选择高亮
 */

#pragma once

#include <memory>
#include <map>
#include <string>

// 前向声明 Skia 类型
class SkCanvas;

namespace lightui {

// 前向声明
class Document;
class Element;
class Node;
class Selection;

/**
 * @brief 光标位置信息
 */
struct CaretPosition {
    std::shared_ptr<Node> node;     ///< 光标所在节点
    int offset = 0;                  ///< 节点内偏移量
    float x = 0.0f;                  ///< 屏幕 X 坐标
    float y = 0.0f;                  ///< 屏幕 Y 坐标
    float height = 0.0f;             ///< 光标高度

    bool IsValid() const { return node != nullptr; }
};

/**
 * @brief 选择管理器
 *
 * 负责管理文本选择状态，处理用户的选择操作，
 * 以及渲染光标和选择高亮。
 */
class SelectionManager {
public:
    SelectionManager();
    ~SelectionManager();

    // ========== Selection 管理 ==========

    /**
     * @brief 获取文档的 Selection 对象
     * @param document 目标文档
     * @return Selection 对象
     */
    std::shared_ptr<Selection> GetSelection(std::shared_ptr<Document> document);

    /**
     * @brief 清除文档的选择
     * @param document 目标文档
     */
    void ClearSelection(std::shared_ptr<Document> document);

    // ========== 鼠标选择处理 ==========

    /**
     * @brief 处理鼠标按下事件
     * @param target 目标元素
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     */
    void HandleMouseDown(std::shared_ptr<Element> target, int x, int y);

    /**
     * @brief 处理鼠标移动事件（拖动选择）
     * @param target 目标元素
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     * @param is_dragging 是否正在拖动
     */
    void HandleMouseMove(std::shared_ptr<Element> target, int x, int y, bool is_dragging);

    /**
     * @brief 处理鼠标释放事件
     * @param target 目标元素
     * @param x 鼠标 X 坐标
     * @param y 鼠标 Y 坐标
     */
    void HandleMouseUp(std::shared_ptr<Element> target, int x, int y);

    // ========== 拖拽选择支持 ==========

    /**
     * @brief 开始拖拽选择
     * @param document 文档
     * @param start_node 起始节点
     * @param start_offset 起始偏移量
     */
    void StartDragSelection(
        std::shared_ptr<Document> document,
        std::shared_ptr<Node> start_node,
        int start_offset
    );

    /**
     * @brief 更新拖拽选择
     * @param document 文档
     * @param end_node 结束节点
     * @param end_offset 结束偏移量
     */
    void UpdateDragSelection(
        std::shared_ptr<Document> document,
        std::shared_ptr<Node> end_node,
        int end_offset
    );

    /**
     * @brief 结束拖拽选择
     * @param document 文档
     */
    void EndDragSelection(std::shared_ptr<Document> document);

    /**
     * @brief 检查是否正在拖拽选择
     * @return true 如果正在拖拽选择
     */
    bool IsDragSelecting() const { return is_drag_selecting_; }

    // ========== 键盘选择处理 ==========

    /**
     * @brief 处理 Shift+方向键选择
     * @param document 目标文档
     * @param direction 方向（"left", "right", "up", "down"）
     */
    void HandleShiftArrow(std::shared_ptr<Document> document, const std::string& direction);

    /**
     * @brief 处理方向键移动光标
     * @param document 目标文档
     * @param direction 方向（"left", "right", "up", "down"）
     */
    void HandleArrowKey(std::shared_ptr<Document> document, const std::string& direction);

    // ========== 光标位置计算 ==========

    /**
     * @brief 将屏幕坐标转换为光标位置
     * @param element 目标元素
     * @param x 屏幕 X 坐标
     * @param y 屏幕 Y 坐标
     * @return 光标位置信息
     */
    CaretPosition HitTestToCaretPosition(std::shared_ptr<Element> element, int x, int y);

    /**
     * @brief 获取当前光标位置
     * @param document 目标文档
     * @return 光标位置信息
     */
    CaretPosition GetCaretPosition(std::shared_ptr<Document> document);

    // ========== 光标渲染 ==========

    /**
     * @brief 渲染光标
     * @param canvas Skia 画布
     * @param document 目标文档
     */
    void RenderCaret(SkCanvas* canvas, std::shared_ptr<Document> document);

    /**
     * @brief 渲染选择高亮
     * @param canvas Skia 画布
     * @param document 目标文档
     */
    void RenderSelectionHighlight(SkCanvas* canvas, std::shared_ptr<Document> document);

    // ========== 光标闪烁控制 ==========

    /**
     * @brief 更新光标闪烁状态
     * @param delta_time 时间增量（毫秒）
     */
    void UpdateCaretBlink(float delta_time);

    /**
     * @brief 重置光标闪烁（显示光标）
     */
    void ResetCaretBlink();

    /**
     * @brief 检查光标是否可见
     * @return true 如果光标当前可见
     */
    bool IsCaretVisible() const { return caret_visible_; }

    // ========== 选择状态查询 ==========

    /**
     * @brief 检查是否正在进行选择操作
     * @return true 如果正在选择
     */
    bool IsSelecting() const { return is_selecting_; }

    /**
     * @brief 获取选择的文本
     * @param document 目标文档
     * @return 选中的文本
     */
    std::string GetSelectedText(std::shared_ptr<Document> document);

private:
    /**
     * @brief 查找包含坐标的文本节点
     * @param element 起始元素
     * @param x X 坐标
     * @param y Y 坐标
     * @return 光标位置
     */
    CaretPosition FindTextNodeAtPosition(std::shared_ptr<Element> element, int x, int y);

    /**
     * @brief 计算文本节点内的偏移量
     * @param text_node 文本节点
     * @param x X 坐标
     * @return 字符偏移量
     */
    int CalculateTextOffset(std::shared_ptr<Node> text_node, int x);

    /**
     * @brief 移动光标到下一个/上一个字符位置
     * @param document 文档
     * @param forward true 向前，false 向后
     * @return 新的光标位置
     */
    CaretPosition MoveCaretByCharacter(std::shared_ptr<Document> document, bool forward);

private:
    /// 每个文档的 Selection 对象
    std::map<Document*, std::shared_ptr<Selection>> selections_;

    /// 是否正在进行选择操作
    bool is_selecting_ = false;

    /// 是否正在拖拽选择
    bool is_drag_selecting_ = false;

    /// 拖拽选择起始节点
    std::shared_ptr<Node> drag_start_node_;

    /// 拖拽选择起始偏移量
    int drag_start_offset_ = 0;

    /// 选择开始位置
    CaretPosition selection_start_;

    /// 光标闪烁相关
    bool caret_visible_ = true;
    float caret_blink_timer_ = 0.0f;
    static constexpr float CARET_BLINK_INTERVAL = 500.0f; // 毫秒
};

} // namespace lightui
