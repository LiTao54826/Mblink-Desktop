/**
 * @file preact_counter_enhanced.js
 * @brief 增强版 Preact 计数器示例
 * 
 * 功能：
 * - 多个独立计数器
 * - 可配置步长
 * - 自动递增功能
 * - 操作历史记录
 */

console.log('🚀 启动增强版 Preact 计数器应用...');

// ========== 子组件 ==========

/**
 * 单个计数器组件
 */
function Counter(props) {
    const useState = preactHooks.useState;

    const [count, setCount] = useState(props.initialValue || 0);
    const [step, setStep] = useState(1);
    const [autoIncrement, setAutoIncrement] = useState(false);
    const [history, setHistory] = useState([]);

    // 自动递增效果
    preactHooks.useEffect(function () {
        if (autoIncrement) {
            const timer = setInterval(function () {
                setCount(function (c) { return c + step; });
                addHistory('自动增加 +' + step);
            }, 1000);

            return function () {
                clearInterval(timer);
            };
        }
    }, [autoIncrement, step]);

    // 添加历史记录
    function addHistory(action) {
        setHistory(function (h) {
            const newHistory = [action].concat(h);
            // 只保留最近5条
            return newHistory.slice(0, 5);
        });
    }

    // 增加
    function increment() {
        setCount(function (c) { return c + step; });
        addHistory('增加 +' + step);
    }

    // 减少
    function decrement() {
        setCount(function (c) { return c - step; });
        addHistory('减少 -' + step);
    }

    // 重置
    function reset() {
        setCount(0);
        addHistory('重置为 0');
    }

    // 更新步长
    function updateStep(e) {
        const value = parseInt(e.target.value) || 1;
        setStep(value);
    }

    // 切换自动递增
    function toggleAuto(e) {
        setAutoIncrement(e.target.checked);
        if (e.target.checked) {
            addHistory('开启自动递增');
        } else {
            addHistory('关闭自动递增');
        }
    }

    return preact.h('div', {
        style: 'background: white; border-radius: 12px; padding: 20px; margin-bottom: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.1);'
    },
        // 标题
        preact.h('h3', {
            style: 'margin: 0 0 15px 0; color: #2c3e50; font-size: 18px;'
        }, props.title || '计数器'),

        // 当前值显示
        preact.h('div', {
            style: 'background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 8px; text-align: center; margin-bottom: 20px;'
        },
            preact.h('div', {
                style: 'font-size: 48px; font-weight: bold; margin-bottom: 10px;'
            }, count),
            preact.h('div', {
                style: 'font-size: 14px; opacity: 0.9;'
            }, '当前计数')
        ),

        // 步长设置
        preact.h('div', {
            style: 'margin-bottom: 15px;'
        },
            preact.h('label', {
                style: 'display: block; margin-bottom: 5px; color: #555; font-size: 14px;'
            }, '步长：'),
            preact.h('input', {
                type: 'number',
                value: step,
                onchange: updateStep,
                style: 'width: 100%; padding: 8px; border: 2px solid #ddd; border-radius: 6px; font-size: 16px; box-sizing: border-box;'
            })
        ),

        // 自动递增选项
        preact.h('div', {
            style: 'margin-bottom: 20px; display: flex; align-items: center;'
        },
            preact.h('input', {
                type: 'checkbox',
                checked: autoIncrement,
                onchange: toggleAuto,
                style: 'width: 18px; height: 18px; margin-right: 8px;'
            }),
            preact.h('label', {
                style: 'color: #555; font-size: 14px;'
            }, '自动递增 (每秒)')
        ),

        // 按钮组
        preact.h('div', {
            style: 'display: flex; gap: 8px; margin-bottom: 20px;'
        },
            preact.h('button', {
                onclick: decrement,
                style: 'flex: 1; padding: 12px; font-size: 16px; background: #e74c3c; color: white; border: none; border-radius: 6px; cursor: pointer; font-weight: bold;'
            }, '➖ 减少'),
            preact.h('button', {
                onclick: reset,
                style: 'padding: 12px 20px; font-size: 16px; background: #95a5a6; color: white; border: none; border-radius: 6px; cursor: pointer; font-weight: bold;'
            }, '🔄'),
            preact.h('button', {
                onclick: increment,
                style: 'flex: 1; padding: 12px; font-size: 16px; background: #27ae60; color: white; border: none; border-radius: 6px; cursor: pointer; font-weight: bold;'
            }, '➕ 增加')
        ),

        // 操作历史
        history.length > 0 ? preact.h('div', {
            style: 'background: #f8f9fa; padding: 12px; border-radius: 6px;'
        },
            preact.h('div', {
                style: 'font-size: 12px; color: #666; margin-bottom: 8px; font-weight: bold;'
            }, '📝 操作历史'),
            history.map(function (item, index) {
                return preact.h('div', {
                    key: index,
                    style: 'font-size: 12px; color: #777; padding: 3px 0;'
                }, '• ' + item);
            })
        ) : null
    );
}

/**
 * 应用头部组件
 */
function AppHeader() {
    return preact.h('div', {
        style: 'background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 12px; margin-bottom: 30px; text-align: center;'
    },
        preact.h('h1', {
            style: 'margin: 0 0 10px 0; font-size: 32px;'
        }, '🎯 增强版 Preact 计数器'),
        preact.h('p', {
            style: 'margin: 0; font-size: 16px; opacity: 0.9;'
        }, '多计数器 • 自定义步长 • 自动递增 • 历史记录')
    );
}

/**
 * 功能说明组件
 */
function FeatureList() {
    const features = [
        '✓ 多个独立计数器实例',
        '✓ 可配置递增/递减步长',
        '✓ 自动递增功能（定时器）',
        '✓ 操作历史记录',
        '✓ useState Hook',
        '✓ useEffect Hook'
    ];

    return preact.h('div', {
        style: 'background: #fff3cd; border-left: 4px solid #ffc107; padding: 20px; border-radius: 8px; margin-bottom: 30px;'
    },
        preact.h('h3', {
            style: 'margin: 0 0 15px 0; color: #856404; font-size: 18px;'
        }, '✨ 功能特性'),
        preact.h('div', {
            style: 'display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px;'
        },
            features.map(function (feature, index) {
                return preact.h('div', {
                    key: index,
                    style: 'color: #856404; font-size: 14px;'
                }, feature);
            })
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    return preact.h('div', {
        style: 'min-height: 100vh; background: linear-gradient(to bottom, #e0e0e0, #f5f5f5); padding: 30px; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;'
    },
        preact.h('div', {
            style: 'max-width: 1200px; margin: 0 auto;'
        },
            // 头部
            preact.h(AppHeader),

            // 功能说明
            preact.h(FeatureList),

            // 多个计数器实例
            preact.h('div', {
                style: 'display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px;'
            },
                preact.h(Counter, { title: '计数器 A', initialValue: 0 }),
                preact.h(Counter, { title: '计数器 B', initialValue: 10 }),
                preact.h(Counter, { title: '计数器 C', initialValue: -5 })
            ),

            // 页脚
            preact.h('div', {
                style: 'margin-top: 40px; padding: 20px; background: white; border-radius: 12px; text-align: center; color: #666;'
            },
                preact.h('p', {
                    style: 'margin: 0; font-size: 14px;'
                }, '🚀 Powered by MBink + Preact + QuickJS')
            )
        )
    );
}

// ========== 渲染应用 ==========

console.log('开始渲染增强版计数器应用...');
preact.render(preact.h(App), document.body);
console.log('✅ 应用渲染完成！');
