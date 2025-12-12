/**
 * @file preact_ui_component_test.js
 * @brief 演示如何在 MBink 中引入外部 UI 组件
 * 
 * 方法 1: 使用 CDN 加载 Preact 兼容组件库
 * 方法 2: 手动实现简单组件
 * 方法 3: 使用预编译的 UMD 包
 */

console.log('🔧 UI 组件引入测试...');

// ============================================================
// 方法 1: 自定义轻量组件（推荐）
// ============================================================

/**
 * 简单的 Button 组件
 * 遵循 Material Design 风格
 */
function Button(props) {
    const { children, onClick, variant = 'contained', color = 'primary', disabled = false } = props;

    // 定义颜色主题
    const colors = {
        primary: { bg: '#1976d2', hover: '#1565c0', text: '#fff' },
        secondary: { bg: '#dc004e', hover: '#c51162', text: '#fff' },
        default: { bg: '#e0e0e0', hover: '#d5d5d5', text: '#000' }
    };

    const theme = colors[color] || colors.default;

    // 基础样式
    let baseStyle = `
        padding: 8px 16px;
        font-size: 14px;
        font-weight: 500;
        border-radius: 4px;
        cursor: pointer;
        border: none;
        outline: none;
        transition: all 0.2s;
        font-family: Roboto, Arial, sans-serif;
        text-transform: uppercase;
        letter-spacing: 0.5px;
    `;

    // 根据 variant 调整样式
    if (variant === 'contained') {
        baseStyle += `
            background: ${theme.bg};
            color: ${theme.text};
        `;
    } else if (variant === 'outlined') {
        baseStyle += `
            background: transparent;
            color: ${theme.bg};
            border: 1px solid ${theme.bg};
        `;
    } else {
        baseStyle += `
            background: transparent;
            color: ${theme.bg};
        `;
    }

    if (disabled) {
        baseStyle += `
            opacity: 0.5;
            cursor: not-allowed;
        `;
    }

    return preact.h('button', {
        style: baseStyle,
        onclick: disabled ? null : onClick,
        disabled: disabled,
        onmouseenter: function (e) {
            if (!disabled && variant === 'contained') {
                e.target.style.background = theme.hover;
            }
        },
        onmouseleave: function (e) {
            if (!disabled && variant === 'contained') {
                e.target.style.background = theme.bg;
            }
        }
    }, children);
}

/**
 * Card 组件
 */
function Card(props) {
    const { children, elevation = 2 } = props;

    const shadowMap = {
        1: '0 1px 3px rgba(0,0,0,0.12)',
        2: '0 2px 4px rgba(0,0,0,0.14)',
        3: '0 4px 8px rgba(0,0,0,0.16)',
        4: '0 8px 16px rgba(0,0,0,0.18)'
    };

    return preact.h('div', {
        style: `
            background: white;
            border-radius: 8px;
            padding: 16px;
            box-shadow: ${shadowMap[elevation] || shadowMap[2]};
        `
    }, children);
}

/**
 * Input 组件
 */
function Input(props) {
    const { placeholder, value, onChange, type = 'text', label } = props;

    return preact.h('div', {
        style: 'margin: 16px 0;'
    },
        label ? preact.h('label', {
            style: 'display: block; margin-bottom: 8px; font-size: 14px; color: #666;'
        }, label) : null,

        preact.h('input', {
            type: type,
            placeholder: placeholder,
            value: value,
            oninput: (e) => onChange && onChange(e.target.value),
            style: `
                width: 100%;
                padding: 12px;
                border: 1px solid #ddd;
                border-radius: 4px;
                font-size: 14px;
                outline: none;
                transition: border-color 0.2s;
                box-sizing: border-box;
            `,
            onfocus: function (e) {
                e.target.style.borderColor = '#1976d2';
            },
            onblur: function (e) {
                e.target.style.borderColor = '#ddd';
            }
        })
    );
}

// ============================================================
// 方法 2: 从 CDN 加载外部组件（需要适配）
// ============================================================

/**
 * 动态加载外部脚本
 * 注意：这需要在 C++ 层支持 <script> 标签动态加载
 */
function loadExternalScript(url) {
    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = url;
        script.onload = resolve;
        script.onerror = reject;
        document.head.appendChild(script);
    });
}

// 示例：如何加载 CDN 组件（需要测试）
// loadExternalScript('https://unpkg.com/preact-material-components@1.6.3/dist/preact-material-components.umd.js')
//     .then(() => {
//         console.log('外部组件库加载成功');
//     });

// ============================================================
// 测试应用
// ============================================================

function TestApp() {
    const [count, setCount] = preactHooks.useState(0);
    const [name, setName] = preactHooks.useState('');

    return preact.h('div', {
        style: 'padding: 40px; background: #f5f5f5; min-height: 100vh;'
    },
        preact.h('h1', {
            style: 'color: #333; margin-bottom: 32px;'
        }, '🎨 MBink UI 组件测试'),

        // Card 示例
        preact.h(Card, { elevation: 3 },
            preact.h('h2', {
                style: 'margin-top: 0; color: #1976d2;'
            }, '计数器组件'),

            preact.h('div', {
                style: 'font-size: 48px; text-align: center; margin: 20px 0; color: #333;'
            }, count),

            preact.h('div', {
                style: 'display: flex; gap: 12px; justify-content: center; flex-wrap: wrap;'
            },
                preact.h(Button, {
                    variant: 'contained',
                    color: 'primary',
                    onClick: () => setCount(count + 1)
                }, '➕ 增加'),

                preact.h(Button, {
                    variant: 'outlined',
                    color: 'secondary',
                    onClick: () => setCount(0)
                }, '🔄 重置'),

                preact.h(Button, {
                    variant: 'text',
                    color: 'default',
                    onClick: () => setCount(count - 1)
                }, '➖ 减少'),

                preact.h(Button, {
                    variant: 'contained',
                    color: 'default',
                    disabled: true
                }, '禁用按钮')
            )
        ),

        // Input 示例
        preact.h(Card, { elevation: 2 },
            preact.h('h2', {
                style: 'margin-top: 0; color: #1976d2;'
            }, '表单组件'),

            preact.h(Input, {
                label: '姓名',
                placeholder: '请输入您的姓名',
                value: name,
                onChange: setName
            }),

            preact.h('div', {
                style: 'margin-top: 16px; color: #666;'
            }, name ? `您好，${name}！` : '请输入姓名')
        ),

        // 说明文档
        preact.h(Card, { elevation: 1 },
            preact.h('h3', {
                style: 'margin-top: 0; color: #666;'
            }, '📝 使用方法'),

            preact.h('pre', {
                style: `
                    background: #f5f5f5;
                    padding: 16px;
                    border-radius: 4px;
                    overflow-x: auto;
                    font-size: 13px;
                    line-height: 1.6;
                `
            }, `// 引入组件
preact.h(Button, {
    variant: 'contained',  // 'contained' | 'outlined' | 'text'
    color: 'primary',      // 'primary' | 'secondary' | 'default'
    onClick: handleClick,
    disabled: false
}, '按钮文字');

preact.h(Input, {
    label: '标签',
    placeholder: '提示文字',
    value: value,
    onChange: setValue
});

preact.h(Card, {
    elevation: 2  // 1-4
}, children);`)
        )
    );
}

// 渲染应用
preact.render(preact.h(TestApp), document.body);

console.log('✅ UI 组件测试应用启动完成！');
