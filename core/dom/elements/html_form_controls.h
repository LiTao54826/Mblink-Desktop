/**
 * @file html_form_controls.h
 * @brief HTML 表单增强控件元素
 * 
 * 包含：fieldset, legend, optgroup, datalist, output, progress, meter, dialog
 */

#pragma once

#include "../element.h"

namespace mblink {

/**
 * @brief HTML FieldSet元素类（fieldset）
 * 
 * 用于表单分组
 */
class HTMLFieldSetElement : public Element {
public:
    HTMLFieldSetElement();
    ~HTMLFieldSetElement() override = default;

    /**
     * @brief 获取是否禁用
     */
    bool GetDisabled() const;

    /**
     * @brief 设置是否禁用
     */
    void SetDisabled(bool disabled);

    /**
     * @brief 获取表单
     */
    std::shared_ptr<Element> GetForm() const;

    /**
     * @brief 获取字段集的名称
     */
    std::string GetName() const;

    /**
     * @brief 设置字段集的名称
     */
    void SetName(const std::string& name);
};

/**
 * @brief HTML Legend元素类（legend）
 * 
 * 用于 fieldset 的标题
 */
class HTMLLegendElement : public Element {
public:
    HTMLLegendElement();
    ~HTMLLegendElement() override = default;

    /**
     * @brief 获取关联的 fieldset
     */
    std::shared_ptr<HTMLFieldSetElement> GetForm() const;
};

/**
 * @brief HTML OptGroup元素类（optgroup）
 * 
 * 用于 select 中的选项分组
 */
class HTMLOptGroupElement : public Element {
public:
    HTMLOptGroupElement();
    ~HTMLOptGroupElement() override = default;

    /**
     * @brief 获取是否禁用
     */
    bool GetDisabled() const;

    /**
     * @brief 设置是否禁用
     */
    void SetDisabled(bool disabled);

    /**
     * @brief 获取分组标签
     */
    std::string GetLabel() const;

    /**
     * @brief 设置分组标签
     */
    void SetLabel(const std::string& label);
};

/**
 * @brief HTML DataList元素类（datalist）
 * 
 * 用于 input 的自动补全列表
 */
class HTMLDataListElement : public Element {
public:
    HTMLDataListElement();
    ~HTMLDataListElement() override = default;

    /**
     * @brief 获取所有 option 元素
     */
    std::vector<std::shared_ptr<Element>> GetOptions() const;
};

/**
 * @brief HTML Output元素类（output）
 * 
 * 用于显示计算结果
 */
class HTMLOutputElement : public Element {
public:
    HTMLOutputElement();
    ~HTMLOutputElement() override = default;

    /**
     * @brief 获取默认值
     */
    std::string GetDefaultValue() const;

    /**
     * @brief 设置默认值
     */
    void SetDefaultValue(const std::string& value);

    /**
     * @brief 获取当前值
     */
    std::string GetValue() const;

    /**
     * @brief 设置当前值
     */
    void SetValue(const std::string& value);

    /**
     * @brief 获取名称
     */
    std::string GetName() const;

    /**
     * @brief 设置名称
     */
    void SetName(const std::string& name);

    /**
     * @brief 获取 for 属性（关联的表单元素 ID 列表）
     */
    std::string GetHtmlFor() const;

    /**
     * @brief 设置 for 属性
     */
    void SetHtmlFor(const std::string& value);
};

/**
 * @brief HTML Progress元素类（progress）
 * 
 * 用于显示进度条
 */
class HTMLProgressElement : public Element {
public:
    HTMLProgressElement();
    ~HTMLProgressElement() override = default;

    /**
     * @brief 获取当前值
     */
    double GetValue() const;

    /**
     * @brief 设置当前值
     */
    void SetValue(double value);

    /**
     * @brief 获取最大值
     */
    double GetMax() const;

    /**
     * @brief 设置最大值
     */
    void SetMax(double max);

    /**
     * @brief 获取进度位置（0.0 - 1.0）
     * 如果没有设置 value，返回 -1（不确定状态）
     */
    double GetPosition() const;
};

/**
 * @brief HTML Meter元素类（meter）
 * 
 * 用于显示度量值（如磁盘使用量、温度等）
 */
class HTMLMeterElement : public Element {
public:
    HTMLMeterElement();
    ~HTMLMeterElement() override = default;

    /**
     * @brief 获取当前值
     */
    double GetValue() const;

    /**
     * @brief 设置当前值
     */
    void SetValue(double value);

    /**
     * @brief 获取最小值
     */
    double GetMin() const;

    /**
     * @brief 设置最小值
     */
    void SetMin(double min);

    /**
     * @brief 获取最大值
     */
    double GetMax() const;

    /**
     * @brief 设置最大值
     */
    void SetMax(double max);

    /**
     * @brief 获取低阈值
     */
    double GetLow() const;

    /**
     * @brief 设置低阈值
     */
    void SetLow(double low);

    /**
     * @brief 获取高阈值
     */
    double GetHigh() const;

    /**
     * @brief 设置高阈值
     */
    void SetHigh(double high);

    /**
     * @brief 获取最优值
     */
    double GetOptimum() const;

    /**
     * @brief 设置最优值
     */
    void SetOptimum(double optimum);
};

/**
 * @brief HTML Dialog元素类（dialog）
 * 
 * 用于模态对话框
 */
class HTMLDialogElement : public Element {
public:
    HTMLDialogElement();
    ~HTMLDialogElement() override = default;

    /**
     * @brief 获取是否打开
     */
    bool GetOpen() const;

    /**
     * @brief 设置是否打开
     */
    void SetOpen(bool open);

    /**
     * @brief 获取返回值
     */
    std::string GetReturnValue() const;

    /**
     * @brief 设置返回值
     */
    void SetReturnValue(const std::string& value);

    /**
     * @brief 显示对话框（非模态）
     */
    void Show();

    /**
     * @brief 显示模态对话框
     */
    void ShowModal();

    /**
     * @brief 关闭对话框
     */
    void Close(const std::string& returnValue = "");

private:
    std::string return_value_;
};

} // namespace mblink

