/**
 * 测试场景 1: 基础样式渲染
 * 测试目标: 验证内联样式、CSS 属性解析、颜色处理
 * 潜在 BUG: 样式属性不生效、颜色解析错误、单位处理问题
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\01_basic_styles.js
 */

import { h, render } from 'preact';

function BasicStylesTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5; min-height: 100vh;' 
    },
        // 标题
        h('h1', { 
            style: 'color: #333; font-size: 32px; margin: 0 0 20px 0; font-weight: bold;' 
        }, 'Basic Styles Test'),
        
        // 颜色测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #2196f3;' }, 'Color Tests'),
            h('div', { style: 'display: flex; gap: 10px; flex-wrap: wrap;' },
                // Hex 颜色
                h('div', { style: 'width: 100px; height: 100px; background: #ff0000; border-radius: 4px;' }),
                h('div', { style: 'width: 100px; height: 100px; background: #00ff00; border-radius: 4px;' }),
                h('div', { style: 'width: 100px; height: 100px; background: #0000ff; border-radius: 4px;' }),
                // RGB 颜色
                h('div', { style: 'width: 100px; height: 100px; background: rgb(255, 165, 0); border-radius: 4px;' }),
                h('div', { style: 'width: 100px; height: 100px; background: rgb(128, 0, 128); border-radius: 4px;' }),
                // RGBA 颜色
                h('div', { style: 'width: 100px; height: 100px; background: rgba(0, 0, 0, 0.5); border-radius: 4px;' }),
                h('div', { style: 'width: 100px; height: 100px; background: rgba(255, 255, 255, 0.8); border: 2px solid #333; border-radius: 4px;' })
            )
        ),
        
        // 尺寸和单位测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #4caf50;' }, 'Size & Units Tests'),
            h('div', { style: 'display: flex; gap: 15px; align-items: flex-end;' },
                h('div', { style: 'width: 50px; height: 50px; background: #e91e63; border-radius: 4px;' }),
                h('div', { style: 'width: 100px; height: 75px; background: #9c27b0; border-radius: 4px;' }),
                h('div', { style: 'width: 150px; height: 100px; background: #3f51b5; border-radius: 4px;' }),
                h('div', { style: 'width: 200px; height: 125px; background: #00bcd4; border-radius: 4px;' })
            ),
            h('p', { style: 'margin-top: 15px; color: #666;' }, 'Boxes with different sizes (px units)')
        ),
        
        // 边框和圆角测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #ff9800;' }, 'Border & Border-Radius Tests'),
            h('div', { style: 'display: flex; gap: 15px; flex-wrap: wrap;' },
                h('div', { 
                    style: 'width: 120px; height: 80px; background: #fff3e0; border: 1px solid #ff9800; border-radius: 0;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: #e3f2fd; border: 2px solid #2196f3; border-radius: 4px;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: #f3e5f5; border: 3px solid #9c27b0; border-radius: 8px;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: #e8f5e9; border: 4px solid #4caf50; border-radius: 16px;' 
                }),
                h('div', { 
                    style: 'width: 80px; height: 80px; background: #fce4ec; border: 2px solid #e91e63; border-radius: 50%;' 
                })
            )
        ),
        
        // 内外边距测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #f44336;' }, 'Padding & Margin Tests'),
            h('div', { style: 'background: #ffebee; padding: 20px;' },
                h('div', { style: 'background: #ffcdd2; padding: 15px;' },
                    h('div', { style: 'background: #ef9a9a; padding: 10px;' },
                        h('div', { style: 'background: #e57373; padding: 5px; color: white; text-align: center;' },
                            'Nested Padding'
                        )
                    )
                )
            ),
            h('div', { style: 'margin-top: 20px; display: flex; gap: 0;' },
                h('div', { style: 'background: #e3f2fd; padding: 10px; margin: 5px;' }, 'Margin 5px'),
                h('div', { style: 'background: #bbdefb; padding: 10px; margin: 10px;' }, 'Margin 10px'),
                h('div', { style: 'background: #90caf9; padding: 10px; margin: 15px;' }, 'Margin 15px')
            )
        ),
        
        // 文本样式测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #673ab7;' }, 'Text Style Tests'),
            h('p', { style: 'font-size: 12px; color: #666;' }, 'Font size: 12px'),
            h('p', { style: 'font-size: 16px; color: #666;' }, 'Font size: 16px'),
            h('p', { style: 'font-size: 20px; color: #666;' }, 'Font size: 20px'),
            h('p', { style: 'font-size: 24px; color: #666;' }, 'Font size: 24px'),
            h('p', { style: 'font-weight: normal; color: #333;' }, 'Font weight: normal'),
            h('p', { style: 'font-weight: bold; color: #333;' }, 'Font weight: bold'),
            h('p', { style: 'font-style: italic; color: #333;' }, 'Font style: italic'),
            h('p', { style: 'text-decoration: underline; color: #2196f3;' }, 'Text decoration: underline'),
            h('p', { style: 'text-align: left; background: #f5f5f5; padding: 5px;' }, 'Text align: left'),
            h('p', { style: 'text-align: center; background: #f5f5f5; padding: 5px;' }, 'Text align: center'),
            h('p', { style: 'text-align: right; background: #f5f5f5; padding: 5px;' }, 'Text align: right')
        ),
        
        // 阴影测试
        h('div', { 
            style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
        },
            h('h2', { style: 'margin: 0 0 15px 0; font-size: 24px; color: #009688;' }, 'Shadow Tests'),
            h('div', { style: 'display: flex; gap: 20px; flex-wrap: wrap;' },
                h('div', { 
                    style: 'width: 120px; height: 80px; background: white; box-shadow: 0 1px 3px rgba(0,0,0,0.12); border-radius: 4px;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: white; box-shadow: 0 2px 6px rgba(0,0,0,0.16); border-radius: 4px;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: white; box-shadow: 0 4px 12px rgba(0,0,0,0.2); border-radius: 4px;' 
                }),
                h('div', { 
                    style: 'width: 120px; height: 80px; background: white; box-shadow: 0 8px 24px rgba(0,0,0,0.24); border-radius: 4px;' 
                })
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Basic Styles Test Loaded Successfully')
    );
}

// 渲染到 DOM
const root = document.getElementById('root') || document.body;
render(h(BasicStylesTest), root);

console.log('[Test 01] Basic styles test rendered');

