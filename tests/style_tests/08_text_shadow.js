/**
 * 测试场景 8: Text Shadow 阴影效果
 * 测试目标: 验证 text-shadow 的各种效果（偏移、模糊、颜色、多重阴影）
 * 潜在 BUG: 阴影不显示、模糊效果不正确、多重阴影渲染错误
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\08_text_shadow.js
 */

import { h, render } from 'preact';

function TextShadowTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh;' 
    },
        h('h1', { 
            style: 'color: white; margin: 0 0 20px 0; text-align: center; font-size: 36px; text-shadow: 3px 3px 6px rgba(0,0,0,0.5);' 
        }, 'Text Shadow Test 🌟'),
        
        // 基础阴影测试
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #667eea; border-bottom: 2px solid #667eea; padding-bottom: 10px;' }, 
                '1. Basic Shadows (基础阴影)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 4px rgba(0,0,0,0.5)'),
                    h('p', { style: 'text-shadow: 2px 2px 4px rgba(0,0,0,0.5); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Simple Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 4px 4px 8px rgba(0,0,0,0.3)'),
                    h('p', { style: 'text-shadow: 4px 4px 8px rgba(0,0,0,0.3); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Larger Offset'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 1px 1px 2px rgba(0,0,0,0.8)'),
                    h('p', { style: 'text-shadow: 1px 1px 2px rgba(0,0,0,0.8); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Tight Shadow'
                    )
                )
            )
        ),
        
        // 不同模糊半径
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #764ba2; border-bottom: 2px solid #764ba2; padding-bottom: 10px;' }, 
                '2. Blur Radius (模糊半径)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 0 black (No Blur)'),
                    h('p', { style: 'text-shadow: 2px 2px 0 black; margin: 0; font-size: 32px; font-weight: bold; color: #ffc107;' }, 
                        'Hard Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 2px rgba(0,0,0,0.5)'),
                    h('p', { style: 'text-shadow: 2px 2px 2px rgba(0,0,0,0.5); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Small Blur (2px)'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 8px rgba(0,0,0,0.5)'),
                    h('p', { style: 'text-shadow: 2px 2px 8px rgba(0,0,0,0.5); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Medium Blur (8px)'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 16px rgba(0,0,0,0.5)'),
                    h('p', { style: 'text-shadow: 2px 2px 16px rgba(0,0,0,0.5); margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Large Blur (16px)'
                    )
                )
            )
        ),
        
        // 发光效果
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: #1a1a2e; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.3);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00d4ff; border-bottom: 2px solid #00d4ff; padding-bottom: 10px;' }, 
                '3. Glow Effects (发光效果)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #16213e; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #aaa;' }, 'text-shadow: 0 0 10px rgba(33,150,243,0.8)'),
                    h('p', { style: 'text-shadow: 0 0 10px rgba(33,150,243,0.8); margin: 0; font-size: 32px; font-weight: bold; color: #2196f3;' }, 
                        'Blue Glow'
                    )
                ),
                h('div', { style: 'background: #16213e; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #aaa;' }, 'text-shadow: 0 0 15px rgba(76,175,80,0.9)'),
                    h('p', { style: 'text-shadow: 0 0 15px rgba(76,175,80,0.9); margin: 0; font-size: 32px; font-weight: bold; color: #4caf50;' }, 
                        'Green Glow'
                    )
                ),
                h('div', { style: 'background: #16213e; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #aaa;' }, 'text-shadow: 0 0 20px rgba(255,193,7,1)'),
                    h('p', { style: 'text-shadow: 0 0 20px rgba(255,193,7,1); margin: 0; font-size: 32px; font-weight: bold; color: #ffc107;' }, 
                        'Gold Glow'
                    )
                )
            )
        ),
        
        // 彩色阴影
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #e91e63; border-bottom: 2px solid #e91e63; padding-bottom: 10px;' }, 
                '4. Colored Shadows (彩色阴影)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 3px 3px 6px red'),
                    h('p', { style: 'text-shadow: 3px 3px 6px red; margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Red Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 3px 3px 6px blue'),
                    h('p', { style: 'text-shadow: 3px 3px 6px blue; margin: 0; font-size: 32px; font-weight: bold; color: #333;' }, 
                        'Blue Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 3px 3px 6px #ff9800'),
                    h('p', { style: 'text-shadow: 3px 3px 6px #ff9800; margin: 0; font-size: 32px; font-weight: bold; color: white;' }, 
                        'Orange Shadow'
                    )
                )
            )
        ),

        // 多重阴影
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff5722; border-bottom: 2px solid #ff5722; padding-bottom: 10px;' },
                '5. Multiple Shadows (多重阴影)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 1px 1px 2px red, -1px -1px 2px blue'),
                    h('p', { style: 'text-shadow: 1px 1px 2px red, -1px -1px 2px blue; margin: 0; font-size: 32px; font-weight: bold; color: #333;' },
                        'Dual Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 3px 3px 0 #ff9800, 6px 6px 0 #f44336'),
                    h('p', { style: 'text-shadow: 3px 3px 0 #ff9800, 6px 6px 0 #f44336; margin: 0; font-size: 32px; font-weight: bold; color: #ffc107;' },
                        'Layered Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 1px 1px 2px black, 0 0 25px blue, 0 0 5px darkblue'),
                    h('p', { style: 'text-shadow: 1px 1px 2px black, 0 0 25px blue, 0 0 5px darkblue; margin: 0; font-size: 32px; font-weight: bold; color: white;' },
                        'Complex Glow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 2px 2px 4px red, 4px 4px 8px orange, 6px 6px 12px yellow'),
                    h('p', { style: 'text-shadow: 2px 2px 4px red, 4px 4px 8px orange, 6px 6px 12px yellow; margin: 0; font-size: 32px; font-weight: bold; color: #333;' },
                        'Rainbow Shadow'
                    )
                )
            )
        ),

        // 负偏移和方向
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #009688; border-bottom: 2px solid #009688; padding-bottom: 10px;' },
                '6. Directional Shadows (方向阴影)'
            ),
            h('div', { style: 'display: grid; grid-template-columns: 1fr 1fr; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 3px 0 5px rgba(0,0,0,0.5) (Right)'),
                    h('p', { style: 'text-shadow: 3px 0 5px rgba(0,0,0,0.5); margin: 0; font-size: 24px; font-weight: bold; color: #333;' },
                        'Right →'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: -3px 0 5px rgba(0,0,0,0.5) (Left)'),
                    h('p', { style: 'text-shadow: -3px 0 5px rgba(0,0,0,0.5); margin: 0; font-size: 24px; font-weight: bold; color: #333;' },
                        '← Left'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 0 3px 5px rgba(0,0,0,0.5) (Down)'),
                    h('p', { style: 'text-shadow: 0 3px 5px rgba(0,0,0,0.5); margin: 0; font-size: 24px; font-weight: bold; color: #333;' },
                        'Down ↓'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 0 -3px 5px rgba(0,0,0,0.5) (Up)'),
                    h('p', { style: 'text-shadow: 0 -3px 5px rgba(0,0,0,0.5); margin: 0; font-size: 24px; font-weight: bold; color: #333;' },
                        '↑ Up'
                    )
                )
            )
        ),

        // 极端情况测试
        h('div', { style: 'margin-bottom: 20px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0; border-bottom: 2px solid #9c27b0; padding-bottom: 10px;' },
                '7. Edge Cases (极端情况)'
            ),
            h('div', { style: 'display: flex; flex-direction: column; gap: 15px;' },
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 0 0 0 black (Zero blur)'),
                    h('p', { style: 'text-shadow: 0 0 0 black; margin: 0; font-size: 32px; font-weight: bold; color: #333;' },
                        'No Effect'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 10px 10px 20px rgba(0,0,0,0.7) (Large offset)'),
                    h('p', { style: 'text-shadow: 10px 10px 20px rgba(0,0,0,0.7); margin: 0; font-size: 32px; font-weight: bold; color: #333;' },
                        'Far Shadow'
                    )
                ),
                h('div', { style: 'background: #f8f9fa; padding: 15px; border-radius: 8px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-size: 12px; color: #666;' }, 'text-shadow: 1px 1px 1px rgba(0,0,0,0.1) (Very subtle)'),
                    h('p', { style: 'text-shadow: 1px 1px 1px rgba(0,0,0,0.1); margin: 0; font-size: 32px; font-weight: bold; color: #333;' },
                        'Subtle Shadow'
                    )
                )
            )
        ),

        // 状态指示
        h('div', {
            style: 'padding: 15px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border-radius: 12px; text-align: center; font-weight: bold; box-shadow: 0 4px 6px rgba(0,0,0,0.2);'
        }, '✓ Text Shadow Test Loaded Successfully')
    );
}

const root = document.getElementById('root') || document.body;
render(h(TextShadowTest), root);

console.log('[Test 08] Text shadow test rendered');

