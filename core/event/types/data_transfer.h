/**
 * @file data_transfer.h
 * @brief DataTransfer类 - 拖拽数据传输对象
 * 
 * 功能：
 * - 实现W3C DataTransfer接口
 * - 支持拖拽数据存储和传输
 * - 支持effectAllowed和dropEffect
 * - 支持多种数据格式
 * 
 * 参考：
 * - W3C HTML5 - DataTransfer
 * - MDN Web Docs - DataTransfer
 * - RmlUi拖拽系统
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace lightui {

/**
 * @brief 拖拽效果类型
 * 
 * 参考：W3C DataTransfer.effectAllowed
 */
enum class DragEffect {
    None,           // 无效果
    Copy,           // 复制
    Move,           // 移动
    Link,           // 链接
    CopyMove,       // 复制或移动
    CopyLink,       // 复制或链接
    LinkMove,       // 链接或移动
    All,            // 所有效果
    Uninitialized   // 未初始化
};

/**
 * @brief DataTransfer类
 * 
 * 表示拖拽操作中的数据传输对象
 * 
 * 符合W3C DataTransfer接口：
 * - setData(format, data) - 设置数据
 * - getData(format) - 获取数据
 * - clearData(format?) - 清除数据
 * - effectAllowed - 允许的拖拽效果
 * - dropEffect - 当前拖拽效果
 * 
 * 参考：
 * - https://html.spec.whatwg.org/multipage/dnd.html#the-datatransfer-interface
 * - https://developer.mozilla.org/en-US/docs/Web/API/DataTransfer
 */
class DataTransfer {
public:
    /**
     * @brief 构造函数
     */
    DataTransfer();
    
    /**
     * @brief 设置数据
     * @param format 数据格式（如"text/plain", "text/html"）
     * @param data 数据内容
     * 
     * 示例：
     * dataTransfer->SetData("text/plain", "Hello World");
     * dataTransfer->SetData("text/html", "<b>Hello World</b>");
     */
    void SetData(const std::string& format, const std::string& data);
    
    /**
     * @brief 获取数据
     * @param format 数据格式
     * @return 数据内容，如果不存在返回空字符串
     * 
     * 示例：
     * std::string text = dataTransfer->GetData("text/plain");
     */
    std::string GetData(const std::string& format) const;
    
    /**
     * @brief 清除数据
     * @param format 数据格式，如果为空则清除所有数据
     * 
     * 示例：
     * dataTransfer->ClearData("text/plain");  // 清除特定格式
     * dataTransfer->ClearData();              // 清除所有数据
     */
    void ClearData(const std::string& format = "");
    
    /**
     * @brief 检查是否有指定格式的数据
     * @param format 数据格式
     * @return 是否存在
     */
    bool HasData(const std::string& format) const;
    
    /**
     * @brief 获取所有数据格式
     * @return 数据格式列表
     */
    std::vector<std::string> GetTypes() const;
    
    /**
     * @brief 设置允许的拖拽效果
     * @param effect 拖拽效果
     * 
     * 示例：
     * dataTransfer->SetEffectAllowed(DragEffect::Copy);
     */
    void SetEffectAllowed(DragEffect effect);
    
    /**
     * @brief 获取允许的拖拽效果
     * @return 拖拽效果
     */
    DragEffect GetEffectAllowed() const;
    
    /**
     * @brief 设置当前拖拽效果
     * @param effect 拖拽效果
     * 
     * 示例：
     * dataTransfer->SetDropEffect(DragEffect::Move);
     */
    void SetDropEffect(DragEffect effect);
    
    /**
     * @brief 获取当前拖拽效果
     * @return 拖拽效果
     */
    DragEffect GetDropEffect() const;
    
    /**
     * @brief 将DragEffect转换为字符串
     * @param effect 拖拽效果
     * @return 字符串表示
     */
    static std::string EffectToString(DragEffect effect);
    
    /**
     * @brief 将字符串转换为DragEffect
     * @param str 字符串
     * @return 拖拽效果
     */
    static DragEffect StringToEffect(const std::string& str);

private:
    // 数据存储（格式 -> 数据）
    std::unordered_map<std::string, std::string> data_;
    
    // 格式插入顺序（用于 GetTypes() 返回有序列表）
    std::vector<std::string> format_order_;
    
    // 允许的拖拽效果
    DragEffect effect_allowed_;
    
    // 当前拖拽效果
    DragEffect drop_effect_;
};

} // namespace lightui

