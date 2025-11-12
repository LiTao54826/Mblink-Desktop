/**
 * @file html_form_element.cpp
 * @brief HTML Form元素实现
 */

#include "html_form_element.h"
#include "html_input_element.h"
#include "html_textarea_element.h"
#include "html_button_element.h"
#include "html_select_element.h"
#include "event.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>

namespace lightui {

// ========== 构造函数 ==========

HTMLFormElement::HTMLFormElement()
    : Element("form") {
}

// ========== IDL属性实现 ==========

std::string HTMLFormElement::GetMethod() const {
    std::string method = GetAttribute("method");
    if (method.empty()) {
        return "get";
    }
    
    // 转换为小写
    std::transform(method.begin(), method.end(), method.begin(), ::tolower);
    
    // 只接受 get 或 post
    if (method == "post") {
        return "post";
    }
    return "get";
}

void HTMLFormElement::SetMethod(const std::string& method) {
    std::string lower_method = method;
    std::transform(lower_method.begin(), lower_method.end(), lower_method.begin(), ::tolower);
    
    if (lower_method == "get" || lower_method == "post") {
        SetAttribute("method", lower_method);
    }
}

std::string HTMLFormElement::GetEnctype() const {
    std::string enctype = GetAttribute("enctype");
    if (enctype.empty()) {
        return "application/x-www-form-urlencoded";
    }
    
    // 转换为小写
    std::transform(enctype.begin(), enctype.end(), enctype.begin(), ::tolower);
    
    // 验证有效值
    if (enctype == "multipart/form-data" || enctype == "text/plain") {
        return enctype;
    }
    return "application/x-www-form-urlencoded";
}

void HTMLFormElement::SetEnctype(const std::string& enctype) {
    std::string lower_enctype = enctype;
    std::transform(lower_enctype.begin(), lower_enctype.end(), lower_enctype.begin(), ::tolower);
    
    if (lower_enctype == "application/x-www-form-urlencoded" ||
        lower_enctype == "multipart/form-data" ||
        lower_enctype == "text/plain") {
        SetAttribute("enctype", lower_enctype);
    }
}

// ========== 表单控件集合 ==========

std::vector<std::shared_ptr<Element>> HTMLFormElement::GetElements() const {
    std::vector<std::shared_ptr<Element>> elements;
    
    // 递归收集所有表单控件
    std::function<void(std::shared_ptr<Node>)> collect = [&](std::shared_ptr<Node> node) {
        if (!node) return;
        
        auto element = std::dynamic_pointer_cast<Element>(node);
        if (element) {
            std::string tag = element->GetTagName();
            
            // 检查是否是表单控件
            if (tag == "input" || tag == "textarea" || tag == "select" || 
                tag == "button" || tag == "output") {
                elements.push_back(element);
            }
        }
        
        // 递归处理子节点
        for (const auto& child : node->GetChildNodes()) {
            collect(child);
        }
    };
    
    // 从当前节点开始收集
    auto self = std::const_pointer_cast<HTMLFormElement>(
        std::static_pointer_cast<const HTMLFormElement>(shared_from_this())
    );
    collect(self);
    
    return elements;
}

// ========== 表单操作 ==========

void HTMLFormElement::Submit() {
    // 1. 验证表单（如果有novalidate属性则跳过）
    if (!HasAttribute("novalidate")) {
        if (!CheckValidity()) {
            // 验证失败，不提交
            return;
        }
    }

    // 2. 触发submit事件
    auto event = std::make_shared<Event>("submit");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());

    DispatchEvent(event);

    // 3. 收集表单数据
    auto form_data = CollectFormData();

    // 4. 根据method和action提交
    // 注意：在实际浏览器环境中，这里会发起HTTP请求
    // 在MBink中，我们只触发事件，让应用层处理提交逻辑

    // TODO: 如果需要，可以在这里添加HTTP请求逻辑
    // 或者通过自定义事件将数据传递给应用层
}

void HTMLFormElement::Reset() {
    // 触发reset事件
    auto event = std::make_shared<Event>("reset");
    event->SetTarget(shared_from_this());
    event->SetCurrentTarget(shared_from_this());

    DispatchEvent(event);

    // 重置所有表单控件到默认值
    auto elements = GetElements();

    for (auto& elem : elements) {
        std::string tag = elem->GetTagName();

        if (tag == "input") {
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(elem);
            if (input) {
                // 重置到默认值
                std::string default_value = input->GetAttribute("value");
                input->SetValue(default_value, false);

                // 重置checked状态
                bool default_checked = input->HasAttribute("checked");
                input->SetChecked(default_checked, false);
            }
        } else if (tag == "textarea") {
            auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(elem);
            if (textarea) {
                // 重置到默认值（初始文本内容）
                // 使用GetTextContent()作为默认值
                std::string default_value = "";
                textarea->SetValue(default_value, false);
            }
        } else if (tag == "select") {
            auto select = std::dynamic_pointer_cast<HTMLSelectElement>(elem);
            if (select) {
                // 重置到默认选中状态
                auto options = select->GetOptions();
                for (auto& option : options) {
                    bool default_selected = option->GetDefaultSelected();
                    option->SetSelected(default_selected);
                }
            }
        }
    }
}

bool HTMLFormElement::CheckValidity() const {
    auto elements = GetElements();

    for (const auto& elem : elements) {
        std::string tag = elem->GetTagName();

        if (tag == "input") {
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(elem);
            if (input && !input->CheckValidity()) {
                return false;
            }
        } else if (tag == "textarea") {
            auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(elem);
            if (textarea && !textarea->CheckValidity()) {
                return false;
            }
        } else if (tag == "select") {
            auto select = std::dynamic_pointer_cast<HTMLSelectElement>(elem);
            if (select && !select->CheckValidity()) {
                return false;
            }
        } else if (tag == "button") {
            auto button = std::dynamic_pointer_cast<HTMLButtonElement>(elem);
            if (button && !button->CheckValidity()) {
                return false;
            }
        }
    }

    return true;
}

bool HTMLFormElement::ReportValidity() const {
    auto elements = GetElements();

    // 检查所有控件，找到第一个无效的
    for (const auto& elem : elements) {
        std::string tag = elem->GetTagName();
        bool valid = true;

        if (tag == "input") {
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(elem);
            if (input) {
                valid = input->CheckValidity();
            }
        } else if (tag == "textarea") {
            auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(elem);
            if (textarea) {
                valid = textarea->CheckValidity();
            }
        } else if (tag == "select") {
            auto select = std::dynamic_pointer_cast<HTMLSelectElement>(elem);
            if (select) {
                valid = select->CheckValidity();
            }
        } else if (tag == "button") {
            auto button = std::dynamic_pointer_cast<HTMLButtonElement>(elem);
            if (button) {
                valid = button->CheckValidity();
            }
        }

        if (!valid) {
            // 找到第一个无效控件，返回false
            // TODO: 在实际浏览器中，这里会显示验证消息并聚焦到控件
            return false;
        }
    }

    return true;
}

// ========== 私有方法 ==========

std::vector<std::pair<std::string, std::string>> HTMLFormElement::CollectFormData() const {
    std::vector<std::pair<std::string, std::string>> data;
    
    auto elements = GetElements();
    
    for (const auto& elem : elements) {
        std::string tag = elem->GetTagName();
        std::string name = elem->GetAttribute("name");
        
        // 跳过没有name的控件
        if (name.empty()) {
            continue;
        }
        
        // 跳过禁用的控件
        if (elem->HasAttribute("disabled")) {
            continue;
        }
        
        if (tag == "input") {
            auto input = std::dynamic_pointer_cast<HTMLInputElement>(elem);
            if (input) {
                // 根据类型收集数据
                auto type = input->GetInputType();
                
                if (type == InputType::Checkbox || type == InputType::Radio) {
                    // 只收集选中的
                    if (input->GetChecked()) {
                        std::string value = input->GetValue();
                        if (value.empty()) {
                            value = "on";
                        }
                        data.push_back({name, value});
                    }
                } else if (type != InputType::Submit && type != InputType::Reset && 
                           type != InputType::Button) {
                    // 收集其他类型的值
                    data.push_back({name, input->GetValue()});
                }
            }
        } else if (tag == "textarea") {
            auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(elem);
            if (textarea) {
                data.push_back({name, textarea->GetValue()});
            }
        } else if (tag == "select") {
            auto select = std::dynamic_pointer_cast<HTMLSelectElement>(elem);
            if (select) {
                if (select->GetMultiple()) {
                    // 多选：收集所有选中的option
                    auto selected_options = select->GetSelectedOptions();
                    for (const auto& option : selected_options) {
                        data.push_back({name, option->GetValue()});
                    }
                } else {
                    // 单选：收集选中的option
                    data.push_back({name, select->GetValue()});
                }
            }
        }
        // button元素不提交数据（除非是提交按钮被点击，这需要额外处理）
    }

    return data;
}

std::string HTMLFormElement::URLEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : str) {
        // 保留字母、数字、-、_、.、~
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}

std::string HTMLFormElement::GetFormDataURLEncoded() const {
    auto data = CollectFormData();
    std::ostringstream result;

    for (size_t i = 0; i < data.size(); ++i) {
        if (i > 0) {
            result << '&';
        }
        result << URLEncode(data[i].first) << '=' << URLEncode(data[i].second);
    }

    return result.str();
}

std::string HTMLFormElement::GetFormDataJSON() const {
    auto data = CollectFormData();
    std::ostringstream result;

    result << "{";

    // 按name分组
    std::map<std::string, std::vector<std::string>> grouped;
    for (const auto& pair : data) {
        grouped[pair.first].push_back(pair.second);
    }

    bool first = true;
    for (const auto& group : grouped) {
        if (!first) {
            result << ",";
        }
        first = false;

        result << "\"" << group.first << "\":";

        if (group.second.size() == 1) {
            // 单个值
            result << "\"" << group.second[0] << "\"";
        } else {
            // 多个值（数组）
            result << "[";
            for (size_t i = 0; i < group.second.size(); ++i) {
                if (i > 0) result << ",";
                result << "\"" << group.second[i] << "\"";
            }
            result << "]";
        }
    }

    result << "}";

    return result.str();
}

} // namespace lightui

