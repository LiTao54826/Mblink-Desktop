/**
 * @file html_terminal_element.h
 * @brief HTML 终端元素
 *
 * 提供 <terminal> 自定义元素，支持 ANSI 终端模拟。
 */

#pragma once

#include "ansi_parser.h"
#include "terminal_buffer.h"
#include "terminal_renderer.h"
#include "core/dom/element.h"
#include "core/dom/elements/virtual_text/selection_manager.h"

#include <memory>
#include <string>

namespace mbink {

// 前向声明
class PtyBackend;
class CommandExecutor;

/**
 * @brief HTML 终端元素
 *
 * 实现 <terminal> 自定义元素，提供：
 * - ANSI 转义序列渲染
 * - 命令执行
 * - 交互式 Shell（PTY）
 * - 文本选择和复制
 *
 * 使用示例：
 * ```html
 * <terminal rows="24" cols="80" scrollback="10000"></terminal>
 * ```
 */
class HTMLTerminalElement : public Element {
public:
    HTMLTerminalElement();
    ~HTMLTerminalElement() override;

    // === 属性 ===

    /**
     * @brief 获取可见行数
     */
    int rows() const { return rows_; }

    /**
     * @brief 设置可见行数
     */
    void set_rows(int value);

    /**
     * @brief 获取列数
     */
    int cols() const { return cols_; }

    /**
     * @brief 设置列数
     */
    void set_cols(int value);

    /**
     * @brief 获取回滚缓冲区大小
     */
    int scrollback() const { return scrollback_; }

    /**
     * @brief 设置回滚缓冲区大小
     */
    void set_scrollback(int value);

    // === 方法 ===

    /**
     * @brief 写入数据
     * @param data 要写入的数据（可包含 ANSI 序列）
     */
    void Write(const std::string& data);

    /**
     * @brief 清空终端
     */
    void Clear();

    /**
     * @brief 滚动到指定行
     * @param line 行号
     */
    void ScrollTo(int line);

    /**
     * @brief 序列化为纯文本
     * @return 纯文本内容
     */
    std::string Serialize() const;

    /**
     * @brief 获取焦点
     */
    void Focus();

    // === 命令执行 ===

    /**
     * @brief 执行单次命令
     * @param command 命令字符串
     */
    void Execute(const std::string& command);

    /**
     * @brief 启动交互式 Shell
     * @param shell Shell 路径（空则使用默认）
     */
    void StartShell(const std::string& shell = "");

    /**
     * @brief 发送输入到 PTY
     * @param input 输入数据
     */
    void SendInput(const std::string& input);

    /**
     * @brief 调整终端大小
     * @param rows 行数
     * @param cols 列数
     */
    void Resize(int rows, int cols);

    // === 选择 ===

    /**
     * @brief 获取选中的文本
     * @return 选中的文本
     */
    std::string GetSelectedText() const;

    /**
     * @brief 复制选中文本到剪贴板
     */
    void CopySelection();

    /**
     * @brief 从剪贴板粘贴文本
     */
    void Paste();

    // === 渲染 ===

    /**
     * @brief 渲染终端
     * @param canvas Skia 画布
     * @param x 左上角 X
     * @param y 左上角 Y
     * @param width 宽度
     * @param height 高度
     */
    void Render(SkCanvas* canvas, float x, float y, float width, float height);

    // === 事件处理 ===

    /**
     * @brief 处理键盘输入
     * @param key 按键
     * @param modifiers 修饰键
     */
    void HandleKeyInput(const std::string& key, int modifiers);

    /**
     * @brief 处理鼠标按下
     * @param x X 坐标
     * @param y Y 坐标
     * @param button 按钮
     * @param clicks 点击次数
     */
    void HandleMouseDown(float x, float y, int button, int clicks);

    /**
     * @brief 处理鼠标移动
     * @param x X 坐标
     * @param y Y 坐标
     */
    void HandleMouseMove(float x, float y);

    /**
     * @brief 处理鼠标释放
     * @param x X 坐标
     * @param y Y 坐标
     * @param button 按钮
     */
    void HandleMouseUp(float x, float y, int button);

    /**
     * @brief 处理滚轮
     * @param delta 滚动量
     */
    void HandleWheel(float delta, bool horizontal = false);

private:
    int rows_ = 24;
    int cols_ = 80;
    int scrollback_ = 10000;

    std::unique_ptr<AnsiParser> parser_;
    std::unique_ptr<TerminalBuffer> buffer_;
    std::unique_ptr<TerminalRenderer> renderer_;
    std::unique_ptr<PtyBackend> pty_;
    std::unique_ptr<CommandExecutor> executor_;
    virtual_text::SelectionManager selection_;

    bool is_focused_ = false;
    
    // 滚动条拖动状态
    bool is_dragging_scrollbar_ = false;
    bool is_dragging_horizontal_scrollbar_ = false;
    float drag_start_y_ = 0.0f;
    float drag_start_x_ = 0.0f;
    int drag_start_offset_ = 0;
    int last_drag_horizontal_offset_ = -1;
    SkRect last_bounds_;  // 缓存渲染区域用于命中测试

    /**
     * @brief 初始化组件
     */
    void Initialize();

    /**
     * @brief 设置解析器回调
     */
    void SetupParserCallbacks();

    /**
     * @brief 更新选择高亮
     */
    void UpdateSelectionHighlight();

    /**
     * @brief 滚动到底部
     */
    void ScrollToBottom();

    /**
     * @brief 检查是否在底部
     */
    bool IsAtBottom();

    /**
     * @brief 将屏幕坐标转换为行列
     */
    void ScreenToCell(float x, float y, int& row, int& col) const;
};

}  // namespace mbink
