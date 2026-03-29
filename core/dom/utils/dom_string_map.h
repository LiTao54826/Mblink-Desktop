/**
 * @file dom_string_map.h
 * @brief DOMStringMap类 - dataset属性实现
 * 
 * 功能：
 * - 实现W3C DOMStringMap接口
 * - 提供dataset API（data-*属性访问）
 * - 自动转换属性名（data-foo-bar <-> fooBar）
 * - 符合HTML5规范
 * 
 * 参考：
 * - W3C HTML5 - DOMStringMap
 * - MDN Web Docs - HTMLElement.dataset
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace mbink {

// 前向声明
class Element;

/**
 * @brief DOMStringMap类
 * 
 * 表示元素的data-*属性集合
 * 
 * 符合W3C DOMStringMap接口：
 * - 通过属性名访问data-*属性
 * - 自动转换命名（data-foo-bar <-> fooBar）
 * - 设置/删除data-*属性
 * 
 * 命名转换规则：
 * - HTML属性：data-foo-bar
 * - JavaScript属性：fooBar
 * - 转换：移除"data-"前缀，将连字符后的字母大写
 * 
 * 参考：
 * - https://html.spec.whatwg.org/multipage/dom.html#domstringmap
 * - https://developer.mozilla.org/en-US/docs/Web/API/HTMLElement/dataset
 */
class DOMStringMap {
public:
    /**
     * @brief 构造函数
     * @param element 关联的元素
     */
    DOMStringMap(std::weak_ptr<Element> element);
    
    /**
     * @brief 获取data-*属性值
     * @param name 属性名（JavaScript格式，如"fooBar"）
     * @return 属性值，如果不存在返回空字符串
     * 
     * 示例：
     * std::string value = dataset->Get("userId");  // 对应data-user-id
     */
    std::string Get(const std::string& name) const;
    
    /**
     * @brief 设置data-*属性值
     * @param name 属性名（JavaScript格式，如"fooBar"）
     * @param value 属性值
     * 
     * 示例：
     * dataset->Set("userId", "123");  // 设置data-user-id="123"
     */
    void Set(const std::string& name, const std::string& value);
    
    /**
     * @brief 删除data-*属性
     * @param name 属性名（JavaScript格式，如"fooBar"）
     * 
     * 示例：
     * dataset->Remove("userId");  // 删除data-user-id
     */
    void Remove(const std::string& name);
    
    /**
     * @brief 检查是否存在data-*属性
     * @param name 属性名（JavaScript格式，如"fooBar"）
     * @return 是否存在
     */
    bool Has(const std::string& name) const;
    
    /**
     * @brief 获取所有data-*属性
     * @return 属性名到属性值的映射（JavaScript格式）
     * 
     * 示例：
     * auto all = dataset->GetAll();
     * for (const auto& [name, value] : all) {
     *     // name: "userId", value: "123"
     * }
     */
    std::unordered_map<std::string, std::string> GetAll() const;

private:
    /**
     * @brief 将JavaScript属性名转换为HTML属性名
     * @param js_name JavaScript属性名（如"fooBar"）
     * @return HTML属性名（如"data-foo-bar"）
     * 
     * 转换规则：
     * - 添加"data-"前缀
     * - 将大写字母转换为"-小写字母"
     * 
     * 示例：
     * "userId" -> "data-user-id"
     * "fooBarBaz" -> "data-foo-bar-baz"
     */
    std::string JsNameToHtmlAttr(const std::string& js_name) const;
    
    /**
     * @brief 将HTML属性名转换为JavaScript属性名
     * @param html_attr HTML属性名（如"data-foo-bar"）
     * @return JavaScript属性名（如"fooBar"）
     * 
     * 转换规则：
     * - 移除"data-"前缀
     * - 将"-字母"转换为大写字母
     * 
     * 示例：
     * "data-user-id" -> "userId"
     * "data-foo-bar-baz" -> "fooBarBaz"
     */
    std::string HtmlAttrToJsName(const std::string& html_attr) const;
    
    /**
     * @brief 检查属性名是否是data-*属性
     * @param attr_name 属性名
     * @return 是否是data-*属性
     */
    bool IsDataAttribute(const std::string& attr_name) const;

private:
    std::weak_ptr<Element> element_;  // 关联的元素（弱引用避免循环引用）
};

} // namespace mbink

