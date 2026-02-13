/**
 * @file exe_writer.h
 * @brief Exe 写入器 - 将 payload 追加到模板 exe 生成最终可执行文件
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace mbink {

/**
 * @brief Exe 写入器
 */
class ExeWriter {
public:
    ExeWriter() = default;
    ~ExeWriter() = default;

    /**
     * @brief 加载模板 exe
     * @param template_path 模板 exe 路径
     * @return 是否成功
     */
    bool LoadTemplate(const std::string& template_path);

    /**
     * @brief 写入输出文件
     * @param output_path 输出文件路径
     * @param payload 要追加的 payload 数据
     * @param show_console 是否显示控制台窗口
     * @param icon_path 可选的 ICO 图标文件路径
     * @return 是否成功
     */
    bool WriteOutput(const std::string& output_path,
                     const std::vector<uint8_t>& payload,
                     bool show_console = false,
                     const std::string& icon_path = "");

    /**
     * @brief 获取模板 exe 大小
     */
    size_t GetTemplateSize() const { return template_data_.size(); }

    /**
     * @brief 获取错误信息
     */
    const std::string& GetError() const { return error_; }

    /**
     * @brief 查找模板 exe
     * @param bundler_path bundler 可执行文件路径
     * @return 找到的模板路径，未找到返回空
     */
    static std::string FindTemplate(const std::string& bundler_path);

private:
    /**
     * @brief 修改 PE 文件的 subsystem 字段
     */
    bool SetSubsystem(std::vector<uint8_t>& data, bool use_console);

    /**
     * @brief 使用 UpdateResource API 将 ICO 图标注入到 PE 文件
     * @param exe_path 目标 exe 文件路径
     * @param ico_path ICO 图标文件路径
     * @return 是否成功
     */
    bool SetIcon(const std::string& exe_path, const std::string& ico_path);

    std::vector<uint8_t> template_data_;
    std::string error_;
};

}  // namespace mbink
