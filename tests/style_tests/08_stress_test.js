/**
 * 测试场景 8: 边界情况和压力测试
 * 测试目标: 验证大量元素渲染、深层嵌套、极端值处理
 * 潜在 BUG: 性能问题、栈溢出、内存泄漏、渲染卡顿
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\08_stress_test.js
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

function StressTest() {
    const [itemCount, setItemCount] = useState(100);
    const [nestingLevel, setNestingLevel] = useState(5);
    const [showLargeList, setShowLargeList] = useState(false);
    
    // 生成深层嵌套结构
    const createNestedDivs = (level, maxLevel) => {
        if (level >= maxLevel) {
            return h('div', { 
                style: 'padding: 10px; background: #4caf50; color: white; border-radius: 4px; text-align: center;' 
            }, `Level ${level} - End`);
        }
        
        const hue = (level / maxLevel) * 360;
        return h('div', { 
            style: `padding: 10px; background: hsl(${hue}, 70%, 90%); border: 2px solid hsl(${hue}, 70%, 50%); border-radius: 4px; margin: 5px;` 
        },
            h('div', { style: 'font-weight: bold; margin-bottom: 5px; color: #333;' }, `Level ${level}`),
            createNestedDivs(level + 1, maxLevel)
        );
    };
    
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Stress Test & Edge Cases'),
        
        // 大量元素渲染
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'Large List Rendering'),
            h('div', { style: 'margin-bottom: 15px;' },
                h('div', { style: 'display: flex; align-items: center; gap: 20px; margin-bottom: 10px;' },
                    h('label', { style: 'font-weight: bold;' }, `Item Count: ${itemCount}`),
                    h('input', {
                        type: 'range',
                        min: '10',
                        max: '1000',
                        step: '10',
                        value: itemCount,
                        oninput: (e) => setItemCount(parseInt(e.target.value)),
                        style: 'flex: 1;'
                    })
                ),
                h('button', {
                    onclick: () => setShowLargeList(!showLargeList),
                    style: `padding: 10px 20px; background: ${showLargeList ? '#f44336' : '#4caf50'}; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;`
                }, showLargeList ? 'Hide List' : 'Show List')
            ),
            showLargeList && h('div', { 
                style: 'max-height: 400px; overflow-y: auto; background: #e3f2fd; padding: 10px; border-radius: 4px;' 
            },
                h('div', { style: 'display: flex; flex-wrap: wrap; gap: 5px;' },
                    Array.from({ length: itemCount }, (_, i) => 
                        h('div', { 
                            key: i,
                            style: `width: 60px; height: 60px; background: hsl(${(i * 360) / itemCount}, 70%, 60%); color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-weight: bold; font-size: 12px;` 
                        }, i + 1)
                    )
                )
            ),
            showLargeList && h('p', { style: 'margin-top: 10px; color: #666; font-size: 14px;' }, 
                `Rendered ${itemCount} items`
            )
        ),
        
        // 深层嵌套
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'Deep Nesting'),
            h('div', { style: 'margin-bottom: 15px;' },
                h('div', { style: 'display: flex; align-items: center; gap: 20px;' },
                    h('label', { style: 'font-weight: bold;' }, `Nesting Level: ${nestingLevel}`),
                    h('input', {
                        type: 'range',
                        min: '1',
                        max: '20',
                        value: nestingLevel,
                        oninput: (e) => setNestingLevel(parseInt(e.target.value)),
                        style: 'flex: 1;'
                    })
                )
            ),
            h('div', { style: 'max-height: 400px; overflow-y: auto; background: #e8f5e9; padding: 10px; border-radius: 4px;' },
                createNestedDivs(1, nestingLevel)
            )
        ),
        
        // 极端尺寸
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'Extreme Sizes'),
            h('div', { style: 'background: #fff3e0; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'Very small (1x1px):'),
                h('div', { style: 'width: 1px; height: 1px; background: #ff9800; margin-bottom: 20px;' }),
                
                h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'Very large (2000x100px):'),
                h('div', { style: 'width: 2000px; height: 100px; background: linear-gradient(90deg, #ff9800, #f57c00, #ef6c00); border-radius: 4px; overflow-x: auto;' }),
                
                h('p', { style: 'font-weight: bold; margin: 20px 0 10px 0;' }, 'Very tall (100x2000px):'),
                h('div', { style: 'max-height: 300px; overflow-y: auto; background: #ffe0b2; padding: 10px; border-radius: 4px;' },
                    h('div', { style: 'width: 100px; height: 2000px; background: linear-gradient(180deg, #ff9800, #f57c00, #ef6c00); border-radius: 4px;' })
                )
            )
        ),
        
        // 极端文本
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'Extreme Text'),
            h('div', { style: 'background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'Very long word (no spaces):'),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px; margin-bottom: 15px; overflow-wrap: break-word;' },
                    'ThisIsAnExtremelyLongWordWithoutAnySpacesOrBreaksThatShouldTestTheWordWrappingAndOverflowHandlingCapabilitiesOfTheLayoutEngine'
                ),
                
                h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'Very long text (10000 characters):'),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px; max-height: 200px; overflow-y: auto;' },
                    'Lorem ipsum dolor sit amet. '.repeat(500)
                ),
                
                h('p', { style: 'font-weight: bold; margin: 20px 0 10px 0;' }, 'Empty text:'),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px; border: 2px dashed #9c27b0; min-height: 40px;' }, ''),
                
                h('p', { style: 'font-weight: bold; margin: 20px 0 10px 0;' }, 'Special characters:'),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px;' },
                    '!@#$%^&*()_+-=[]{}|;:\'",.<>?/~`\n\t\r\n测试中文字符 テスト 🎉🚀💻🎨'
                )
            )
        ),
        
        // 极端颜色值
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'Extreme Colors'),
            h('div', { style: 'display: flex; gap: 10px; flex-wrap: wrap;' },
                h('div', { style: 'width: 100px; height: 100px; background: #000000; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' }, 'Black'),
                h('div', { style: 'width: 100px; height: 100px; background: #ffffff; color: black; border: 2px solid #ccc; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' }, 'White'),
                h('div', { style: 'width: 100px; height: 100px; background: rgba(255,0,0,0); border: 2px solid #ccc; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' }, 'Transparent'),
                h('div', { style: 'width: 100px; height: 100px; background: rgba(255,0,0,0.01); border: 2px solid #ccc; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' }, 'Almost transparent'),
                h('div', { style: 'width: 100px; height: 100px; background: rgba(255,0,0,0.99); color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px; font-size: 12px;' }, 'Almost opaque')
            )
        ),
        
        // 复杂变换
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00bcd4;' }, 'Complex Transforms'),
            h('div', { style: 'background: #e0f7fa; padding: 40px; border-radius: 4px; display: flex; gap: 30px; flex-wrap: wrap; justify-content: center;' },
                h('div', { style: 'width: 80px; height: 80px; background: #00bcd4; transform: rotate(45deg); border-radius: 4px;' }),
                h('div', { style: 'width: 80px; height: 80px; background: #00acc1; transform: scale(1.5); border-radius: 4px;' }),
                h('div', { style: 'width: 80px; height: 80px; background: #0097a7; transform: skew(20deg, 10deg); border-radius: 4px;' }),
                h('div', { style: 'width: 80px; height: 80px; background: #00838f; transform: rotate(45deg) scale(1.2); border-radius: 4px;' }),
                h('div', { style: 'width: 80px; height: 80px; background: #006064; transform: rotate(180deg) scale(0.8); border-radius: 4px;' })
            )
        ),
        
        // 性能指标
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #673ab7;' }, 'Performance Metrics'),
            h('div', { style: 'background: #ede7f6; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'margin: 5px 0;' }, `Total DOM nodes: ~${itemCount + nestingLevel * 2 + 100}`),
                h('p', { style: 'margin: 5px 0;' }, `Nesting depth: ${nestingLevel}`),
                h('p', { style: 'margin: 5px 0;' }, `Rendered items: ${showLargeList ? itemCount : 0}`),
                h('p', { style: 'margin: 5px 0; color: #666; font-size: 14px;' }, 
                    'Monitor console for rendering performance and memory usage'
                )
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Stress Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(StressTest), root);

console.log('[Test 08] Stress test rendered');

