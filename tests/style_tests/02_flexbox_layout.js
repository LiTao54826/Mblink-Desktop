/**
 * 测试场景 2: Flexbox 布局系统
 * 测试目标: 验证 flex 容器、flex 项目、对齐方式、方向、换行
 * 潜在 BUG: flex 属性不生效、对齐错误、尺寸计算问题、换行异常
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\02_flexbox_layout.js
 */

import { h, render } from 'preact';

function FlexboxTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Flexbox Layout Test'),
        
        // flex-direction: row
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'flex-direction: row'),
            h('div', { style: 'display: flex; flex-direction: row; gap: 10px; background: #e3f2fd; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'width: 80px; height: 80px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '1'),
                h('div', { style: 'width: 80px; height: 80px; background: #1976d2; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '2'),
                h('div', { style: 'width: 80px; height: 80px; background: #1565c0; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '3')
            )
        ),
        
        // flex-direction: column
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'flex-direction: column'),
            h('div', { style: 'display: flex; flex-direction: column; gap: 10px; background: #e8f5e9; padding: 15px; border-radius: 4px; width: 200px;' },
                h('div', { style: 'height: 50px; background: #4caf50; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '1'),
                h('div', { style: 'height: 50px; background: #388e3c; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '2'),
                h('div', { style: 'height: 50px; background: #2e7d32; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '3')
            )
        ),
        
        // justify-content 测试
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'justify-content'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-start'),
            h('div', { style: 'display: flex; justify-content: flex-start; gap: 10px; background: #fff3e0; padding: 15px; border-radius: 4px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 60px; background: #ff9800; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #f57c00; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #ef6c00; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'center'),
            h('div', { style: 'display: flex; justify-content: center; gap: 10px; background: #fff3e0; padding: 15px; border-radius: 4px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 60px; background: #ff9800; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #f57c00; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #ef6c00; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-end'),
            h('div', { style: 'display: flex; justify-content: flex-end; gap: 10px; background: #fff3e0; padding: 15px; border-radius: 4px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 60px; background: #ff9800; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #f57c00; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #ef6c00; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'space-between'),
            h('div', { style: 'display: flex; justify-content: space-between; background: #fff3e0; padding: 15px; border-radius: 4px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 60px; background: #ff9800; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #f57c00; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #ef6c00; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'space-around'),
            h('div', { style: 'display: flex; justify-content: space-around; background: #fff3e0; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'width: 60px; height: 60px; background: #ff9800; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #f57c00; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #ef6c00; border-radius: 4px;' })
            )
        ),
        
        // align-items 测试
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'align-items'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-start'),
            h('div', { style: 'display: flex; align-items: flex-start; gap: 10px; background: #f3e5f5; padding: 15px; border-radius: 4px; height: 120px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 40px; background: #9c27b0; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #7b1fa2; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 80px; background: #6a1b9a; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'center'),
            h('div', { style: 'display: flex; align-items: center; gap: 10px; background: #f3e5f5; padding: 15px; border-radius: 4px; height: 120px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 40px; background: #9c27b0; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #7b1fa2; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 80px; background: #6a1b9a; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-end'),
            h('div', { style: 'display: flex; align-items: flex-end; gap: 10px; background: #f3e5f5; padding: 15px; border-radius: 4px; height: 120px; margin-bottom: 10px;' },
                h('div', { style: 'width: 60px; height: 40px; background: #9c27b0; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 60px; background: #7b1fa2; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; height: 80px; background: #6a1b9a; border-radius: 4px;' })
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'stretch'),
            h('div', { style: 'display: flex; align-items: stretch; gap: 10px; background: #f3e5f5; padding: 15px; border-radius: 4px; height: 120px;' },
                h('div', { style: 'width: 60px; background: #9c27b0; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; background: #7b1fa2; border-radius: 4px;' }),
                h('div', { style: 'width: 60px; background: #6a1b9a; border-radius: 4px;' })
            )
        ),
        
        // flex-grow 和 flex-shrink
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'flex-grow & flex-shrink'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-grow: 1 on middle item'),
            h('div', { style: 'display: flex; gap: 10px; background: #ffebee; padding: 15px; border-radius: 4px; margin-bottom: 10px;' },
                h('div', { style: 'width: 100px; height: 60px; background: #f44336; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Fixed'),
                h('div', { style: 'flex-grow: 1; height: 60px; background: #e53935; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Grow'),
                h('div', { style: 'width: 100px; height: 60px; background: #d32f2f; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Fixed')
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'flex-grow with different ratios'),
            h('div', { style: 'display: flex; gap: 10px; background: #ffebee; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'flex-grow: 1; height: 60px; background: #f44336; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Grow 1'),
                h('div', { style: 'flex-grow: 2; height: 60px; background: #e53935; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Grow 2'),
                h('div', { style: 'flex-grow: 1; height: 60px; background: #d32f2f; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'Grow 1')
            )
        ),
        
        // flex-wrap
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00bcd4;' }, 'flex-wrap'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'nowrap (default)'),
            h('div', { style: 'display: flex; flex-wrap: nowrap; gap: 10px; background: #e0f7fa; padding: 15px; border-radius: 4px; margin-bottom: 10px; overflow-x: auto;' },
                ...Array.from({ length: 10 }, (_, i) => 
                    h('div', { 
                        key: i,
                        style: 'min-width: 80px; height: 60px; background: #00bcd4; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' 
                    }, i + 1)
                )
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'wrap'),
            h('div', { style: 'display: flex; flex-wrap: wrap; gap: 10px; background: #e0f7fa; padding: 15px; border-radius: 4px;' },
                ...Array.from({ length: 10 }, (_, i) => 
                    h('div', { 
                        key: i,
                        style: 'width: 80px; height: 60px; background: #00bcd4; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' 
                    }, i + 1)
                )
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Flexbox Layout Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(FlexboxTest), root);

console.log('[Test 02] Flexbox layout test rendered');

