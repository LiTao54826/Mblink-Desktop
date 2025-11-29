/**
 * @file app.js
 * @brief Preact Form Demo - 表单元素和验证示例
 *
 * Features:
 * - 各种表单输入类型 (text, email, password, number, checkbox, radio)
 * - 表单验证
 * - 实时状态显示
 * - 提交处理
 */

// ========== 子组件 ==========

// Header组件
function Header(props) {
    return Preact.h('div', {
        style: 'background-color: #673AB7; color: white; padding: 20px; margin-bottom: 20px;'
    },
        Preact.h('h1', {
            style: 'margin: 0; font-size: 28px;'
        }, props.title),
        Preact.h('p', {
            style: 'margin: 5px 0 0 0; font-size: 14px; opacity: 0.9;'
        }, props.subtitle)
    );
}

// FormField组件 - 单个表单字段
function FormField(props) {
    var labelStyle = 'display: block; margin-bottom: 5px; font-weight: bold; color: #333;';
    var inputStyle = 'width: 100%; padding: 10px; font-size: 14px; border: 2px solid ' +
                     (props.error ? '#f44336' : '#ddd') + '; border-radius: 4px; box-sizing: border-box;';
    var errorStyle = 'color: #f44336; font-size: 12px; margin-top: 5px;';
    var containerStyle = 'margin-bottom: 15px;';

    return Preact.h('div', { style: containerStyle },
        Preact.h('label', { style: labelStyle }, props.label),
        Preact.h('input', {
            type: props.type || 'text',
            style: inputStyle,
            placeholder: props.placeholder,
            value: props.value,
            onChange: props.onChange
        }),
        props.error ? Preact.h('div', { style: errorStyle }, props.error) : null
    );
}

// CheckboxField组件
function CheckboxField(props) {
    var containerStyle = 'margin-bottom: 15px; display: flex; align-items: center;';
    var checkboxStyle = 'width: 18px; height: 18px; margin-right: 10px;';
    var labelStyle = 'color: #333; cursor: pointer;';

    return Preact.h('div', { style: containerStyle },
        Preact.h('input', {
            type: 'checkbox',
            style: checkboxStyle,
            checked: props.checked,
            onChange: props.onChange
        }),
        Preact.h('label', { style: labelStyle }, props.label)
    );
}

// RadioGroup组件
function RadioGroup(props) {
    var containerStyle = 'margin-bottom: 15px;';
    var labelStyle = 'display: block; margin-bottom: 10px; font-weight: bold; color: #333;';
    var optionStyle = 'display: flex; align-items: center; margin-bottom: 8px;';
    var radioStyle = 'width: 18px; height: 18px; margin-right: 10px;';
    var textStyle = 'color: #333;';

    var options = [];
    for (var i = 0; i < props.options.length; i++) {
        var option = props.options[i];
        options.push(
            Preact.h('div', { style: optionStyle, key: option.value },
                Preact.h('input', {
                    type: 'radio',
                    style: radioStyle,
                    name: props.name,
                    value: option.value,
                    checked: props.value === option.value,
                    onChange: function(val) {
                        return function(e) { props.onChange(val); };
                    }(option.value)
                }),
                Preact.h('span', { style: textStyle }, option.label)
            )
        );
    }

    return Preact.h('div', { style: containerStyle },
        Preact.h('label', { style: labelStyle }, props.label),
        options
    );
}

// FormPreview组件 - 实时显示表单状态
function FormPreview(props) {
    var containerStyle = 'background-color: #f5f5f5; padding: 15px; border-radius: 8px; margin-bottom: 20px;';
    var titleStyle = 'margin: 0 0 10px 0; font-size: 16px; color: #333; font-weight: bold;';
    var itemStyle = 'margin: 5px 0; font-size: 14px; color: #666;';
    var valueStyle = 'color: #333; font-weight: bold;';

    var data = props.data;

    return Preact.h('div', { style: containerStyle },
        Preact.h('h3', { style: titleStyle }, '📋 Form Preview'),
        Preact.h('div', { style: itemStyle },
            'Name: ', Preact.h('span', { style: valueStyle }, data.name || '(empty)')
        ),
        Preact.h('div', { style: itemStyle },
            'Email: ', Preact.h('span', { style: valueStyle }, data.email || '(empty)')
        ),
        Preact.h('div', { style: itemStyle },
            'Age: ', Preact.h('span', { style: valueStyle }, data.age || '(empty)')
        ),
        Preact.h('div', { style: itemStyle },
            'Gender: ', Preact.h('span', { style: valueStyle }, data.gender || '(not selected)')
        ),
        Preact.h('div', { style: itemStyle },
            'Newsletter: ', Preact.h('span', { style: valueStyle }, data.newsletter ? 'Yes' : 'No')
        ),
        Preact.h('div', { style: itemStyle },
            'Terms: ', Preact.h('span', { style: valueStyle }, data.terms ? 'Accepted' : 'Not accepted')
        )
    );
}

// SubmitResult组件
function SubmitResult(props) {
    if (!props.show) return null;

    var containerStyle = 'background-color: #4CAF50; color: white; padding: 15px; border-radius: 8px; ' +
                         'margin-bottom: 20px; text-align: center;';

    return Preact.h('div', { style: containerStyle },
        Preact.h('h3', { style: 'margin: 0 0 10px 0;' }, '✓ Form Submitted Successfully!'),
        Preact.h('p', { style: 'margin: 0;' }, 'Thank you, ' + props.name + '!')
    );
}

// Footer组件
function Footer() {
    var footerStyle = 'margin-top: 30px; padding: 15px; background-color: #f5f5f5; ' +
                      'border-top: 2px solid #ddd; text-align: center;';
    var textStyle = 'margin: 0; font-size: 14px; color: #666;';

    return Preact.h('div', { style: footerStyle },
        Preact.h('p', { style: textStyle },
            '🚀 Powered by MBink + Preact + QuickJS'
        ),
        Preact.h('p', { style: textStyle },
            '📝 Form Demo - Input Validation Example'
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    var useState = PreactHooks.useState;

    // 表单数据
    var formState = useState({
        name: '',
        email: '',
        password: '',
        age: '',
        gender: '',
        newsletter: false,
        terms: false
    });
    var formData = formState[0];
    var setFormData = formState[1];

    // 错误状态
    var errorsState = useState({});
    var errors = errorsState[0];
    var setErrors = errorsState[1];

    // 提交状态
    var submittedState = useState(false);
    var submitted = submittedState[0];
    var setSubmitted = submittedState[1];

    // 更新字段
    var updateField = function(field) {
        return function(e) {
            var value = e.target.value;
            setFormData(function(prev) {
                var newData = {};
                for (var key in prev) {
                    newData[key] = prev[key];
                }
                newData[field] = value;
                return newData;
            });
            // 清除该字段的错误
            setErrors(function(prev) {
                var newErrors = {};
                for (var key in prev) {
                    if (key !== field) {
                        newErrors[key] = prev[key];
                    }
                }
                return newErrors;
            });
        };
    };

    // 更新checkbox
    var updateCheckbox = function(field) {
        return function(e) {
            var checked = e.target.checked;
            setFormData(function(prev) {
                var newData = {};
                for (var key in prev) {
                    newData[key] = prev[key];
                }
                newData[field] = checked;
                return newData;
            });
        };
    };

    // 更新radio
    var updateRadio = function(value) {
        setFormData(function(prev) {
            var newData = {};
            for (var key in prev) {
                newData[key] = prev[key];
            }
            newData.gender = value;
            return newData;
        });
    };

    // 验证表单
    var validate = function() {
        var newErrors = {};
        
        if (!formData.name.trim()) {
            newErrors.name = 'Name is required';
        }
        
        if (!formData.email.trim()) {
            newErrors.email = 'Email is required';
        } else if (formData.email.indexOf('@') === -1) {
            newErrors.email = 'Please enter a valid email';
        }
        
        if (!formData.password) {
            newErrors.password = 'Password is required';
        } else if (formData.password.length < 6) {
            newErrors.password = 'Password must be at least 6 characters';
        }
        
        if (formData.age && (isNaN(formData.age) || parseInt(formData.age) < 1)) {
            newErrors.age = 'Please enter a valid age';
        }
        
        if (!formData.terms) {
            newErrors.terms = 'You must accept the terms';
        }
        
        setErrors(newErrors);
        
        // 检查是否有错误
        for (var key in newErrors) {
            return false;
        }
        return true;
    };

    // 提交表单
    var handleSubmit = function(e) {
        e.preventDefault();
        console.log('[Form] Submit clicked');
        
        if (validate()) {
            console.log('[Form] Validation passed, data:', JSON.stringify(formData));
            setSubmitted(true);
        } else {
            console.log('[Form] Validation failed');
        }
    };

    // 重置表单
    var handleReset = function() {
        setFormData({
            name: '',
            email: '',
            password: '',
            age: '',
            gender: '',
            newsletter: false,
            terms: false
        });
        setErrors({});
        setSubmitted(false);
    };

    var appStyle = 'padding: 0; margin: 0; background-color: #e0e0e0; min-height: 100vh;';
    var containerStyle = 'max-width: 600px; margin: 0 auto; padding: 20px;';
    var formStyle = 'background-color: white; padding: 20px; border-radius: 8px;';
    var buttonContainerStyle = 'display: flex; gap: 10px; margin-top: 20px;';
    var submitButtonStyle = 'flex: 1; padding: 12px; font-size: 16px; background-color: #673AB7; ' +
                            'color: white; border: none; border-radius: 4px; cursor: pointer;';
    var resetButtonStyle = 'padding: 12px 20px; font-size: 16px; background-color: #9E9E9E; ' +
                           'color: white; border: none; border-radius: 4px; cursor: pointer;';

    var genderOptions = [
        { value: 'male', label: 'Male' },
        { value: 'female', label: 'Female' },
        { value: 'other', label: 'Other' }
    ];

    return Preact.h('div', { style: appStyle },
        Preact.h('div', { style: containerStyle },
            Preact.h(Header, {
                title: 'MBink Form Demo',
                subtitle: 'Interactive form with validation - Fill out the form below'
            }),
            Preact.h(SubmitResult, { show: submitted, name: formData.name }),
            Preact.h(FormPreview, { data: formData }),
            Preact.h('form', { style: formStyle, onSubmit: handleSubmit },
                Preact.h(FormField, {
                    label: 'Full Name *',
                    type: 'text',
                    placeholder: 'Enter your name',
                    value: formData.name,
                    error: errors.name,
                    onChange: updateField('name')
                }),
                Preact.h(FormField, {
                    label: 'Email Address *',
                    type: 'email',
                    placeholder: 'Enter your email',
                    value: formData.email,
                    error: errors.email,
                    onChange: updateField('email')
                }),
                Preact.h(FormField, {
                    label: 'Password *',
                    type: 'password',
                    placeholder: 'Enter password (min 6 chars)',
                    value: formData.password,
                    error: errors.password,
                    onChange: updateField('password')
                }),
                Preact.h(FormField, {
                    label: 'Age',
                    type: 'number',
                    placeholder: 'Enter your age',
                    value: formData.age,
                    error: errors.age,
                    onChange: updateField('age')
                }),
                Preact.h(RadioGroup, {
                    label: 'Gender',
                    name: 'gender',
                    options: genderOptions,
                    value: formData.gender,
                    onChange: updateRadio
                }),
                Preact.h(CheckboxField, {
                    label: 'Subscribe to newsletter',
                    checked: formData.newsletter,
                    onChange: updateCheckbox('newsletter')
                }),
                Preact.h(CheckboxField, {
                    label: 'I accept the terms and conditions *',
                    checked: formData.terms,
                    onChange: updateCheckbox('terms')
                }),
                errors.terms ? Preact.h('div', { 
                    style: 'color: #f44336; font-size: 12px; margin-top: -10px; margin-bottom: 10px;' 
                }, errors.terms) : null,
                Preact.h('div', { style: buttonContainerStyle },
                    Preact.h('button', { type: 'submit', style: submitButtonStyle }, 'Submit'),
                    Preact.h('button', { type: 'button', style: resetButtonStyle, onClick: handleReset }, 'Reset')
                )
            ),
            Preact.h(Footer)
        )
    );
}

// ========== 渲染应用 ==========

console.log('Starting Form Demo...');
var appVNode = Preact.h(App);
Preact.render(appVNode, document.body);
console.log('Form Demo rendered!');

