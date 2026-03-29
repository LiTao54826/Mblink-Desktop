/**
 * @file dom_token_list.h
 * @brief DOMTokenList类 - classList实现
 * 
 * 功能：
 * - 实现W3C DOMTokenList接口
 * - 提供classList API（add, remove, toggle, contains）
 * - 支持多个token操作
 * - 符合HTML5规范
 * 
 * 参考：
 * - W3C DOM Standard - DOMTokenList
 * - MDN Web Docs - Element.classList
 * - RmlUi/Source/Core/Element.cpp (SetClass, IsClassSet)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mbink {

// 前向声明
class Element;

/**
 * @brief DOMTokenList类
 * 
 * 表示一组空格分隔的token（如class列表）
 * 
 * 符合W3C DOMTokenList接口：
 * - add(token1, token2, ...) - 添加一个或多个token
 * - remove(token1, token2, ...) - 移除一个或多个token
 * - toggle(token, force?) - 切换token
 * - contains(token) - 检查是否包含token
 * - item(index) - 获取指定索引的token
 * - length - token数量
 * - value - 完整的token字符串
 * 
 * 参考：
 * - https://dom.spec.whatwg.org/#interface-domtokenlist
 * - https://developer.mozilla.org/en-US/docs/Web/API/DOMTokenList
 */
class DOMTokenList {
public:
    /**
     * @brief 构造函数
     * @param element 关联的元素
     * @param attr_name 属性名（通常是"class"）
     */
    DOMTokenList(std::weak_ptr<Element> element, const std::string& attr_name);
    
    /**
     * @brief 添加一个或多个token
     * @param tokens token列表
     * 
     * 如果token已存在，则不会重复添加
     * 如果token包含空格，会抛出异常（符合W3C规范）
     */
    void Add(const std::vector<std::string>& tokens);
    
    /**
     * @brief 添加单个token
     * @param token token字符串
     */
    void Add(const std::string& token);
    
    /**
     * @brief 移除一个或多个token
     * @param tokens token列表
     * 
     * 如果token不存在，不会报错
     */
    void Remove(const std::vector<std::string>& tokens);
    
    /**
     * @brief 移除单个token
     * @param token token字符串
     */
    void Remove(const std::string& token);
    
    /**
     * @brief 切换token
     * @param token token字符串
     * @param force 可选，true强制添加，false强制移除
     * @return true表示添加，false表示移除
     * 
     * 如果token存在且force未指定或为false，则移除token
     * 如果token不存在且force未指定或为true，则添加token
     */
    bool Toggle(const std::string& token);
    bool Toggle(const std::string& token, bool force);
    
    /**
     * @brief 检查是否包含token
     * @param token token字符串
     * @return true表示包含
     */
    bool Contains(const std::string& token) const;
    
    /**
     * @brief 获取指定索引的token
     * @param index 索引（从0开始）
     * @return token字符串，如果索引越界返回空字符串
     */
    std::string Item(size_t index) const;
    
    /**
     * @brief 获取token数量
     * @return token数量
     */
    size_t Length() const;
    
    /**
     * @brief 获取完整的token字符串
     * @return token字符串（空格分隔）
     */
    std::string Value() const;
    
    /**
     * @brief 设置完整的token字符串
     * @param value token字符串（空格分隔）
     */
    void SetValue(const std::string& value);
    
    /**
     * @brief 替换token
     * @param old_token 旧token
     * @param new_token 新token
     * @return true表示替换成功，false表示旧token不存在
     */
    bool Replace(const std::string& old_token, const std::string& new_token);
    
    /**
     * @brief 检查token是否支持
     * @param token token字符串
     * @return true表示支持
     * 
     * 注意：对于classList，所有非空token都支持
     * 这个方法主要用于其他DOMTokenList（如relList）
     */
    bool Supports(const std::string& token) const;

private:
    /**
     * @brief 解析token字符串为token列表
     * @param value token字符串
     * @return token列表
     */
    std::vector<std::string> ParseTokens(const std::string& value) const;
    
    /**
     * @brief 将token列表序列化为字符串
     * @param tokens token列表
     * @return token字符串
     */
    std::string SerializeTokens(const std::vector<std::string>& tokens) const;
    
    /**
     * @brief 验证token是否有效
     * @param token token字符串
     * @return true表示有效
     * 
     * 有效的token：
     * - 不能为空
     * - 不能包含空格
     */
    bool ValidateToken(const std::string& token) const;
    
    /**
     * @brief 更新元素的属性值
     */
    void UpdateAttribute();

private:
    std::weak_ptr<Element> element_;  // 关联的元素（弱引用避免循环引用）
    std::string attr_name_;            // 属性名（通常是"class"）
};

} // namespace mbink

