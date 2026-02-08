/**
 * 测试场景 4: 定位系统 (Position)
 * 测试目标: 验证 static、relative、absolute、fixed 定位
 * 潜在 BUG: 定位计算错误、z-index 层叠问题、偏移量不生效
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\04_positioning.js
 */

import { h, render } from 'preact';

function PositioningTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Positioning System Test'),
        
        // position: static (default)
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'position: static (default)'),
            h('div', { style: 'background: #e3f2fd; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'width: 150px; height: 80px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; margin-bottom: 10px;' }, 'Box 1'),
                h('div', { style: 'width: 150px; height: 80px; background: #1976d2; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; margin-bottom: 10px;' }, 'Box 2'),
                h('div', { style: 'width: 150px; height: 80px; background: #1565c0; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Box 3'),
                h('p', { style: 'margin: 10px 0 0 0; color: #666; font-size: 14px;' }, 'Normal document flow')
            )
        ),
        
        // position: relative
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'position: relative'),
            h('div', { style: 'background: #e8f5e9; padding: 15px; border-radius: 4px; position: relative;' },
                h('div', { style: 'width: 150px; height: 80px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; margin-bottom: 10px;' }, 'Box 1'),
                h('div', { 
                    style: 'width: 150px; height: 80px; background: #388e3c; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; margin-bottom: 10px; position: relative; top: 20px; left: 30px; border: 2px solid #fff;' 
                }, 'Box 2 (offset)'),
                h('div', { style: 'width: 150px; height: 80px; background: #2e7d32; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Box 3'),
                h('p', { style: 'margin: 10px 0 0 0; color: #666; font-size: 14px;' }, 'Box 2 offset by top: 20px, left: 30px')
            )
        ),
        
        // position: absolute
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'position: absolute'),
            h('div', { style: 'background: #fff3e0; padding: 15px; border-radius: 4px; position: relative; height: 250px;' },
                h('div', { style: 'width: 150px; height: 80px; background: #ff9800; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Normal Box'),
                
                // 四个角的绝对定位
                h('div', { 
                    style: 'position: absolute; top: 10px; left: 10px; width: 80px; height: 60px; background: #f57c00; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' 
                }, 'Top Left'),
                h('div', { 
                    style: 'position: absolute; top: 10px; right: 10px; width: 80px; height: 60px; background: #ef6c00; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' 
                }, 'Top Right'),
                h('div', { 
                    style: 'position: absolute; bottom: 10px; left: 10px; width: 80px; height: 60px; background: #e65100; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' 
                }, 'Bottom Left'),
                h('div', { 
                    style: 'position: absolute; bottom: 10px; right: 10px; width: 80px; height: 60px; background: #bf360c; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' 
                }, 'Bottom Right'),
                
                // 居中定位
                h('div', { 
                    style: 'position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 100px; height: 70px; background: #ff5722; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-weight: bold;' 
                }, 'Centered')
            )
        ),
        
        // position: fixed
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'position: fixed'),
            h('div', { style: 'background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'margin: 0 0 10px 0; color: #666;' }, 'Fixed elements are positioned relative to the viewport'),
                h('div', { style: 'height: 150px; background: #e1bee7; border-radius: 4px; overflow-y: scroll; padding: 10px;' },
                    h('p', null, 'Scroll this area to see the fixed element behavior.'),
                    h('p', null, 'Lorem ipsum dolor sit amet, consectetur adipiscing elit.'),
                    h('p', null, 'Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.'),
                    h('p', null, 'Ut enim ad minim veniam, quis nostrud exercitation ullamco.'),
                    h('p', null, 'Duis aute irure dolor in reprehenderit in voluptate velit.'),
                    h('p', null, 'Excepteur sint occaecat cupidatat non proident.')
                )
            )
        ),
        
        // z-index 层叠测试
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'z-index (Stacking Order)'),
            h('div', { style: 'background: #ffebee; padding: 15px; border-radius: 4px; position: relative; height: 200px;' },
                h('div', { 
                    style: 'position: absolute; top: 20px; left: 20px; width: 120px; height: 120px; background: #f44336; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; z-index: 1;' 
                }, 'z-index: 1'),
                h('div', { 
                    style: 'position: absolute; top: 50px; left: 50px; width: 120px; height: 120px; background: #e53935; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; z-index: 3;' 
                }, 'z-index: 3'),
                h('div', { 
                    style: 'position: absolute; top: 80px; left: 80px; width: 120px; height: 120px; background: #d32f2f; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; z-index: 2;' 
                }, 'z-index: 2'),
                h('p', { style: 'position: absolute; bottom: 10px; left: 10px; margin: 0; color: #666; font-size: 14px;' }, 'Higher z-index appears on top')
            )
        ),
        
        // 复杂定位组合
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00bcd4;' }, 'Complex Positioning'),
            h('div', { style: 'background: #e0f7fa; padding: 15px; border-radius: 4px; position: relative; height: 300px;' },
                // 相对定位的容器
                h('div', { 
                    style: 'position: relative; width: 200px; height: 150px; background: #00bcd4; border-radius: 4px; padding: 10px; color: white; top: 20px; left: 20px;' 
                },
                    h('div', { style: 'font-weight: bold; margin-bottom: 10px;' }, 'Relative Container'),
                    // 绝对定位的子元素
                    h('div', { 
                        style: 'position: absolute; top: -10px; right: -10px; width: 40px; height: 40px; background: #ff5722; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 12px; font-weight: bold;' 
                    }, 'Badge'),
                    h('div', { 
                        style: 'position: absolute; bottom: 10px; left: 10px; background: rgba(0,0,0,0.3); padding: 5px 10px; border-radius: 4px; font-size: 12px;' 
                    }, 'Absolute child')
                ),
                
                // 另一个绝对定位元素
                h('div', { 
                    style: 'position: absolute; bottom: 20px; right: 20px; width: 150px; padding: 15px; background: #0097a7; color: white; border-radius: 4px;' 
                },
                    h('div', { style: 'font-weight: bold; margin-bottom: 5px;' }, 'Absolute Box'),
                    h('div', { style: 'font-size: 12px;' }, 'Positioned at bottom-right')
                )
            )
        ),
        
        // 固定定位的浮动按钮（右下角）
        h('div', { 
            style: 'position: fixed; bottom: 20px; right: 20px; width: 60px; height: 60px; background: #4caf50; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); cursor: pointer; z-index: 1000;' 
        }, '+'),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Positioning System Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(PositioningTest), root);

console.log('[Test 04] Positioning test rendered');

