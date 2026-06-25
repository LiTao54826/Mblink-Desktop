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
#include <vector>

class SkCanvas;
class SkFont;

namespace mblink {

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

struct SelectionRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
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
    CaretPosition HitTestToCaretPosition(std::shared_ptr<Element> element,
                                         int x,
                                         int y,
                                         const SkFont* font = nullptr);

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
     * @brief 获取当前选择高亮矩形
     * @param document 目标文档
     * @return 选择区域矩形列表
     */
    std::vector<SelectionRect> GetSelectionRects(std::shared_ptr<Document> document);

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
    CaretPosition FindTextNodeAtPosition(std::shared_ptr<Element> element,
                                         int x,
                                         int y,
                                         const SkFont* font = nullptr);

    /**
     * @brief 计算文本节点内的偏移量
     * @param text_node 文本节点
     * @param x X 坐标
     * @return 字符偏移量
     */
    int CalculateTextOffset(std::shared_ptr<Node> text_node,
                            int x,
                            const SkFont* font = nullptr);

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

    /// 光标闪烁相关
    bool caret_visible_ = true;
    float caret_blink_timer_ = 0.0f;
    static constexpr float CARET_BLINK_INTERVAL = 500.0f; // 毫秒
};

} // namespace mblink
