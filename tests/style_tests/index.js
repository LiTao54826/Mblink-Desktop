/**
 * 测试索引页面 - 样式和布局系统测试套件
 * 提供一个可视化界面来选择和运行不同的测试场景
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\index.js
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

function TestIndex() {
    const [selectedTest, setSelectedTest] = useState(null);
    
    const tests = [
        {
            id: '01',
            name: 'Basic Styles',
            file: '01_basic_styles.js',
            description: '基础样式渲染测试',
            details: '颜色、尺寸、边框、圆角、内外边距、文本样式、阴影',
            color: '#2196f3',
            icon: '🎨'
        },
        {
            id: '02',
            name: 'Flexbox Layout',
            file: '02_flexbox_layout.js',
            description: 'Flexbox 布局系统测试',
            details: 'flex-direction, justify-content, align-items, flex-grow, flex-wrap',
            color: '#4caf50',
            icon: '📐'
        },
        {
            id: '03',
            name: 'Box Model',
            file: '03_box_model.js',
            description: '盒模型和溢出处理测试',
            details: 'box-sizing, overflow, min/max 尺寸, text-overflow',
            color: '#ff9800',
            icon: '📦'
        },
        {
            id: '04',
            name: 'Positioning',
            file: '04_positioning.js',
            description: '定位系统测试',
            details: 'static, relative, absolute, fixed, z-index',
            color: '#9c27b0',
            icon: '📍'
        },
        {
            id: '05',
            name: 'Nested Layouts',
            file: '05_nested_layouts.js',
            description: '复杂嵌套布局测试',
            details: '嵌套 Flexbox, 卡片网格, 侧边栏, 圣杯布局',
            color: '#f44336',
            icon: '🏗️'
        },
        {
            id: '06',
            name: 'Dynamic Styles',
            file: '06_dynamic_styles.js',
            description: '动态样式更新测试',
            details: '动态尺寸、颜色、旋转、透明度、Flexbox 变化、动画',
            color: '#00bcd4',
            icon: '⚡'
        },
        {
            id: '07',
            name: 'Text & Fonts',
            file: '07_text_fonts.js',
            description: '文本和字体样式测试',
            details: '字体大小、粗细、对齐、行高、字间距、文本装饰、阴影',
            color: '#673ab7',
            icon: '📝'
        },
        {
            id: '08',
            name: 'Stress Test',
            file: '08_stress_test.js',
            description: '边界情况和压力测试',
            details: '大量元素、深层嵌套、极端尺寸、极端文本、性能测试',
            color: '#e91e63',
            icon: '🔥'
        }
    ];
    
    return h('div', { 
        style: 'min-height: 100vh; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 40px 20px; font-family: Arial, sans-serif;' 
    },
        // 头部
        h('div', { style: 'max-width: 1200px; margin: 0 auto 40px;' },
            h('h1', { 
                style: 'color: white; font-size: 48px; margin: 0 0 10px 0; text-align: center; text-shadow: 2px 2px 4px rgba(0,0,0,0.3);' 
            }, '🎨 MBlink Style Tests'),
            h('p', { 
                style: 'color: rgba(255,255,255,0.9); font-size: 20px; margin: 0; text-align: center;' 
            }, '样式和布局系统测试套件'),
            h('p', { 
                style: 'color: rgba(255,255,255,0.7); font-size: 14px; margin: 10px 0 0 0; text-align: center;' 
            }, '选择一个测试场景查看详情，或直接运行对应的 .js 文件')
        ),
        
        // 测试卡片网格
        h('div', { 
            style: 'max-width: 1200px; margin: 0 auto; display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px;' 
        },
            tests.map(test => 
                h('div', {
                    key: test.id,
                    onclick: () => setSelectedTest(selectedTest === test.id ? null : test.id),
                    style: `
                        background: white;
                        border-radius: 12px;
                        padding: 24px;
                        cursor: pointer;
                        transition: all 0.3s;
                        box-shadow: ${selectedTest === test.id ? '0 8px 24px rgba(0,0,0,0.3)' : '0 4px 12px rgba(0,0,0,0.2)'};
                        transform: ${selectedTest === test.id ? 'translateY(-8px) scale(1.02)' : 'translateY(0) scale(1)'};
                        border: ${selectedTest === test.id ? `3px solid ${test.color}` : '3px solid transparent'};
                    `
                },
                    // 图标和编号
                    h('div', { style: 'display: flex; align-items: center; justify-content: space-between; margin-bottom: 16px;' },
                        h('div', { 
                            style: `font-size: 48px; line-height: 1;` 
                        }, test.icon),
                        h('div', { 
                            style: `
                                width: 40px;
                                height: 40px;
                                background: ${test.color};
                                color: white;
                                border-radius: 50%;
                                display: flex;
                                align-items: center;
                                justify-content: center;
                                font-weight: bold;
                                font-size: 18px;
                            ` 
                        }, test.id)
                    ),
                    
                    // 标题
                    h('h3', { 
                        style: `color: ${test.color}; font-size: 24px; margin: 0 0 8px 0;` 
                    }, test.name),
                    
                    // 描述
                    h('p', { 
                        style: 'color: #666; font-size: 16px; margin: 0 0 12px 0; font-weight: 500;' 
                    }, test.description),
                    
                    // 详情
                    h('p', { 
                        style: 'color: #999; font-size: 14px; margin: 0 0 16px 0; line-height: 1.5;' 
                    }, test.details),
                    
                    // 文件名
                    h('div', { 
                        style: 'background: #f5f5f5; padding: 8px 12px; border-radius: 6px; font-family: monospace; font-size: 12px; color: #666;' 
                    }, test.file),
                    
                    // 展开的详细信息
                    selectedTest === test.id && h('div', { 
                        style: 'margin-top: 16px; padding-top: 16px; border-top: 2px solid #f0f0f0;' 
                    },
                        h('p', { style: 'color: #333; font-weight: bold; margin: 0 0 8px 0;' }, '运行命令:'),
                        h('div', { 
                            style: 'background: #2d2d2d; color: #f8f8f2; padding: 12px; border-radius: 6px; font-family: monospace; font-size: 12px; overflow-x: auto;' 
                        }, `build\\bin\\Release\\esm_loader.exe tests\\style_tests\\${test.file}`),
                        h('p', { style: 'color: #666; font-size: 12px; margin: 8px 0 0 0;' }, '点击卡片收起详情')
                    )
                )
            )
        ),
        
        // 底部信息
        h('div', { 
            style: 'max-width: 1200px; margin: 40px auto 0; padding: 24px; background: rgba(255,255,255,0.1); border-radius: 12px; backdrop-filter: blur(10px);' 
        },
            h('h3', { style: 'color: white; margin: 0 0 16px 0; font-size: 20px;' }, '📖 使用说明'),
            h('div', { style: 'color: rgba(255,255,255,0.9); line-height: 1.8;' },
                h('p', { style: 'margin: 0 0 8px 0;' }, '• 点击测试卡片查看详细信息和运行命令'),
                h('p', { style: 'margin: 0 0 8px 0;' }, '• 使用 esm_loader.exe 运行对应的 .js 文件'),
                h('p', { style: 'margin: 0 0 8px 0;' }, '• 添加 --devtools 参数可以打开开发者工具'),
                h('p', { style: 'margin: 0 0 8px 0;' }, '• 运行 run_all_tests.bat 可以批量执行所有测试'),
                h('p', { style: 'margin: 0;' }, '• 查看 README.md 了解更多详细信息')
            )
        ),
        
        // 快速启动按钮
        h('div', { 
            style: 'max-width: 1200px; margin: 20px auto 0; text-align: center;' 
        },
            h('div', {
                style: 'display: block; max-width: 600px; margin: 0 auto; background: rgba(255,255,255,0.2); padding: 16px 24px; border-radius: 8px; backdrop-filter: blur(10px);'
            },
                h('p', { style: 'color: white; margin: 0 0 12px 0; font-size: 16px; font-weight: bold;' }, '💡 提示'),
                h('p', { style: 'color: rgba(255,255,255,0.9); margin: 0; font-size: 14px;' }, 
                    '这是测试索引页面，实际测试需要运行对应的 .js 文件'
                )
            )
        )
    );
}

const root = document.getElementById('root') || document.body;
render(h(TestIndex), root);

console.log('[Test Index] Test index page loaded');
console.log('Available tests:', 8);

