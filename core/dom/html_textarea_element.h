/**
 * @file html_textarea_element.h
 * @brief HTML TextArea元素类
 * 
 * 参考：
 * - W3C HTML5 - HTMLTextAreaElement
 * - MDN Web Docs - HTMLTextAreaElement
 * - RmlUi/Source/Core/Elements/ElementFormControl.h
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>

// Forward declaration for Skia SkFont
class SkFont;

namespace lightui {

/**
 * @brief HTML TextArea元素类
 * 
 * 实现W3C HTMLTextAreaElement接口的子集
 * 参考：https://html.spec.whatwg.org/multipage/form-elements.html#the-textarea-element
 */
class HTMLTextAreaElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLTextAreaElement();
    
    /**
     * @brief 析构函数
     */
    ~HTMLTextAreaElement() override = default;
    
    // ========== TextArea特有属性 ==========
    
    /**
     * @brief 获取value值
     * @return 当前值
     */
    std::string GetValue() const { return value_; }
    
    /**
     * @brief 设置value值
     * @param value 新值
     * @param trigger_events 是否触发change/input事件
     */
    void SetValue(const std::string& value, bool trigger_events = false);
    
    /**
     * @brief 获取placeholder文本
     * @return placeholder文本
     */
    std::string GetPlaceholder() const { return GetAttribute("placeholder"); }
    
    /**
     * @brief 设置placeholder文本
     * @param placeholder placeholder文本
     */
    void SetPlaceholder(const std::string& placeholder) { SetAttribute("placeholder", placeholder); }
    
    /**
     * @brief 获取maxlength限制
     * @return 最大长度，-1表示无限制
     */
    int GetMaxLength() const;
    
    /**
     * @brief 设置maxlength限制
     * @param max_length 最大长度
     */
    void SetMaxLength(int max_length);
    
    /**
     * @brief 获取rows（行数）
     * @return 行数
     */
    int GetRows() const;
    
    /**
     * @brief 设置rows（行数）
     * @param rows 行数
     */
    void SetRows(int rows);
    
    /**
     * @brief 获取cols（列数）
     * @return 列数
     */
    int GetCols() const;
    
    /**
     * @brief 设置cols（列数）
     * @param cols 列数
     */
    void SetCols(int cols);
    
    /**
     * @brief 检查是否disabled
     * @return true表示禁用
     */
    bool IsDisabled() const { return HasAttribute("disabled"); }
    
    /**
     * @brief 设置disabled状态
     * @param disabled 是否禁用
     */
    void SetDisabled(bool disabled);
    
    /**
     * @brief 检查是否readonly
     * @return true表示只读
     */
    bool IsReadOnly() const { return HasAttribute("readonly"); }
    
    /**
     * @brief 设置readonly状态
     * @param readonly 是否只读
     */
    void SetReadOnly(bool readonly);
    
    /**
     * @brief 检查是否required
     * @return true表示必填
     */
    bool IsRequired() const { return HasAttribute("required"); }
    
    /**
     * @brief 设置required状态
     * @param required 是否必填
     */
    void SetRequired(bool required);
    
    // ========== 表单验证 ==========
    
    /**
     * @brief 检查输入是否有效
     * @return true表示有效
     */
    bool CheckValidity() const;
    
    /**
     * @brief 获取验证错误消息
     * @return 错误消息，如果有效则返回空字符串
     */
    std::string GetValidationMessage() const;
    
    // ========== 焦点和选择 ==========
    
    /**
     * @brief 选中所有文本
     */
    void Select();
    
    /**
     * @brief 设置选择范围
     * @param start 起始位置
     * @param end 结束位置
     */
    void SetSelectionRange(int start, int end);

    /**
     * @brief 获取选择起始位置
     * @return 选择起始位置
     */
    int GetSelectionStart() const { return selection_start_; }

    /**
     * @brief 获取选择结束位置
     * @return 选择结束位置
     */
    int GetSelectionEnd() const { return selection_end_; }

    // ========== 内部方法 ==========
    
    /**
     * @brief 处理文本输入（由EventLoop调用）
     * @param text 输入的文本
     */
    void HandleTextInput(const std::string& text);
    
    /**
     * @brief 处理键盘事件（由EventLoop调用）
     * @param key 按键名称
     * @param ctrl_key Ctrl键是否按下
     * @param shift_key Shift键是否按下
     */
    void HandleKeyPress(const std::string& key, bool ctrl_key, bool shift_key = false);

    // ========== 鼠标交互 ==========

    /**
     * @brief 处理鼠标按下事件
     * @param local_x 相对于元素的X坐标
     * @param local_y 相对于元素的Y坐标
     */
    void HandleMouseDown(float local_x, float local_y);

    /**
     * @brief 处理鼠标移动事件
     * @param local_x 相对于元素的X坐标
     * @param local_y 相对于元素的Y坐标
     */
    void HandleMouseMove(float local_x, float local_y);

    /**
     * @brief 处理鼠标释放事件
     */
    void HandleMouseUp();

    /**
     * @brief 设置光标位置
     * @param char_pos 字符位置
     */
    void SetCursorPosition(int char_pos);

    /**
     * @brief 设置选择范围
     * @param start 起始位置
     * @param end 结束位置
     */
    void SetSelection(int start, int end);

    /**
     * @brief 检查是否正在拖动选择
     * @return true表示正在拖动
     */
    bool IsDraggingSelection() const { return is_dragging_selection_; }

    /**
     * @brief 设置拖动起始位置
     * @param pos 字符位置
     */
    void SetDragStartPos(int pos) { drag_start_pos_ = pos; }

    /**
     * @brief 获取拖动起始位置
     * @return 字符位置
     */
    int GetDragStartPos() const { return drag_start_pos_; }

    // ========== 滚动相关方法 ==========

    /**
     * @brief 获取垂直滚动偏移量
     * @return 滚动偏移量（像素）
     */
    float GetScrollTop() const { return scroll_top_; }

    /**
     * @brief 设置垂直滚动偏移量
     * @param scroll_top 滚动偏移量（像素）
     */
    void SetScrollTop(float scroll_top);

    /**
     * @brief 获取横向滚动偏移量
     * @return 滚动偏移量（像素）
     */
    float GetScrollLeft() const { return scroll_left_; }

    /**
     * @brief 设置横向滚动偏移量
     * @param scroll_left 滚动偏移量（像素）
     */
    void SetScrollLeft(float scroll_left);

    /**
     * @brief 处理鼠标滚轮事件
     * @param delta_y 滚轮增量（正值向下，负值向上）
     * @param line_height 行高（像素）
     * @param visible_height 可见区域高度（像素）
     */
    void HandleMouseWheel(float delta_y, float line_height, float visible_height);

    /**
     * @brief 处理水平鼠标滚轮事件（Shift+滚轮）
     * @param delta_x 滚轮增量（正值向右，负值向左）
     * @param visible_width 可见区域宽度（像素）
     * @param font 用于测量文本宽度的字体
     */
    void HandleMouseWheelHorizontal(float delta_x, float visible_width, const SkFont& font);

    /**
     * @brief 确保光标可见（自动滚动）
     * @param line_height 行高（像素）
     * @param visible_height 可见区域高度（像素）
     * @param visible_width 可见区域宽度（像素）
     * @param font 用于测量文本宽度的字体
     */
    void EnsureCursorVisible(float line_height, float visible_height, float visible_width, const SkFont& font);

    /**
     * @brief 检查是否需要滚动到光标位置
     * @return true表示需要滚动
     */
    bool NeedsScrollToCursor() const { return needs_scroll_to_cursor_; }

    /**
     * @brief 重置滚动到光标的标记
     */
    void ResetScrollToCursor() { needs_scroll_to_cursor_ = false; }

    /**
     * @brief 获取总行数
     * @return 行数
     */
    int GetLineCount() const;

    /**
     * @brief 获取内容总高度
     * @param line_height 行高（像素）
     * @return 内容总高度（像素）
     */
    float GetContentHeight(float line_height) const;

    /**
     * @brief 获取最大行宽度
     * @param font 用于测量文本宽度的字体
     * @return 最大行宽度（像素）
     */
    float GetMaxLineWidth(const SkFont& font) const;

    // ========== 滚动条拖动相关 ==========

    /**
     * @brief 滚动条类型枚举
     */
    enum class ScrollbarType { NONE, VERTICAL, HORIZONTAL };

    /**
     * @brief 开始拖动滚动条
     * @param type 滚动条类型
     * @param mouse_pos 鼠标位置（相对于滚动条轨道）
     */
    void StartScrollbarDrag(ScrollbarType type, float mouse_pos);

    /**
     * @brief 更新滚动条拖动
     * @param mouse_pos 当前鼠标位置
     * @param track_size 轨道尺寸
     * @param content_size 内容尺寸
     * @param visible_size 可见区域尺寸
     */
    void UpdateScrollbarDrag(float mouse_pos, float track_size, float content_size, float visible_size);

    /**
     * @brief 结束滚动条拖动
     */
    void EndScrollbarDrag();

    /**
     * @brief 是否正在拖动滚动条
     */
    bool IsDraggingScrollbar() const { return scrollbar_drag_type_ != ScrollbarType::NONE; }

    /**
     * @brief 获取当前拖动的滚动条类型
     */
    ScrollbarType GetDraggingScrollbarType() const { return scrollbar_drag_type_; }

    /**
     * @brief 滚动条宽度常量
     */
    static constexpr float SCROLLBAR_WIDTH = 8.0f;

protected:
    /**
     * @brief 触发change事件
     */
    void TriggerChangeEvent();
    
    /**
     * @brief 触发input事件
     */
    void TriggerInputEvent();

private:
    /**
     * @brief 根据字符位置计算行号和列号
     * @param char_pos 字符位置
     * @param out_line 输出行号（0-based）
     * @param out_col 输出列号（0-based）
     */
    void GetLineAndColumn(int char_pos, int& out_line, int& out_col) const;

    /**
     * @brief 根据行号和列号计算字符位置
     * @param line 行号（0-based）
     * @param col 列号（0-based）
     * @return 字符位置
     */
    int GetCharPosFromLineColumn(int line, int col) const;

    /**
     * @brief 获取指定行的起始和结束字符位置
     * @param line 行号（0-based）
     * @param out_start 输出行起始字符位置
     * @param out_end 输出行结束字符位置（不含换行符）
     */
    void GetLineRange(int line, int& out_start, int& out_end) const;

    std::string value_;             // 当前值
    int selection_start_;           // 选择起始位置
    int selection_end_;             // 选择结束位置
    bool is_dragging_selection_;    // 是否正在拖动选择
    int drag_start_pos_;            // 拖动起始位置
    float scroll_top_;              // 垂直滚动偏移量
    float scroll_left_;             // 横向滚动偏移量
    bool needs_scroll_to_cursor_;   // 是否需要滚动到光标位置

    // 滚动条拖动相关
    ScrollbarType scrollbar_drag_type_ = ScrollbarType::NONE;  // 当前拖动的滚动条类型
    float scrollbar_drag_start_pos_;   // 拖动开始时的鼠标位置
    float scrollbar_drag_start_scroll_; // 拖动开始时的滚动位置
};

} // namespace lightui

