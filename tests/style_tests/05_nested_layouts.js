/**
 * 测试场景 5: 复杂嵌套布局
 * 测试目标: 验证多层嵌套、混合布局模式、复杂组合
 * 潜在 BUG: 嵌套计算错误、布局冲突、性能问题
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\05_nested_layouts.js
 */

import { h, render } from 'preact';

function NestedLayoutsTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Nested Layouts Test'),
        
        // 嵌套 Flexbox
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'Nested Flexbox'),
            h('div', { 
                style: 'display: flex; flex-direction: column; gap: 10px; background: #e3f2fd; padding: 15px; border-radius: 4px;' 
            },
                // 第一行 - 水平布局
                h('div', { style: 'display: flex; gap: 10px;' },
                    h('div', { style: 'flex: 1; height: 80px; background: #2196f3; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'A'),
                    h('div', { style: 'flex: 2; height: 80px; background: #1976d2; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'B (flex: 2)'),
                    h('div', { style: 'flex: 1; height: 80px; background: #1565c0; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'C')
                ),
                // 第二行 - 嵌套垂直布局
                h('div', { style: 'display: flex; gap: 10px;' },
                    h('div', { 
                        style: 'flex: 1; display: flex; flex-direction: column; gap: 10px;' 
                    },
                        h('div', { style: 'height: 60px; background: #64b5f6; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'D1'),
                        h('div', { style: 'height: 60px; background: #42a5f5; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'D2')
                    ),
                    h('div', { style: 'flex: 2; height: 130px; background: #0d47a1; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, 'E (Large)')
                )
            )
        ),
        
        // 卡片网格布局
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'Card Grid Layout'),
            h('div', { 
                style: 'display: flex; flex-wrap: wrap; gap: 15px;' 
            },
                ...Array.from({ length: 6 }, (_, i) => 
                    h('div', { 
                        key: i,
                        style: 'flex: 1 1 calc(33.333% - 10px); min-width: 200px; background: #e8f5e9; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1);' 
                    },
                        // 卡片头部
                        h('div', { 
                            style: 'height: 120px; background: linear-gradient(135deg, #4caf50, #81c784); display: flex; align-items: center; justify-content: center; color: white; font-size: 32px; font-weight: bold;' 
                        }, i + 1),
                        // 卡片内容
                        h('div', { style: 'padding: 15px;' },
                            h('h3', { style: 'margin: 0 0 10px 0; color: #2e7d32;' }, `Card ${i + 1}`),
                            h('p', { style: 'margin: 0; color: #666; font-size: 14px;' }, 'This is a card with nested flex layout and responsive design.')
                        )
                    )
                )
            )
        ),
        
        // 侧边栏布局
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'Sidebar Layout'),
            h('div', { 
                style: 'display: flex; gap: 15px; height: 400px; background: #fff3e0; padding: 15px; border-radius: 4px;' 
            },
                // 侧边栏
                h('div', { 
                    style: 'width: 200px; background: #ff9800; border-radius: 4px; padding: 15px; display: flex; flex-direction: column; gap: 10px;' 
                },
                    h('div', { style: 'color: white; font-weight: bold; font-size: 18px; margin-bottom: 10px;' }, 'Sidebar'),
                    ...['Home', 'Products', 'Services', 'About', 'Contact'].map(item =>
                        h('div', { 
                            key: item,
                            style: 'background: rgba(255,255,255,0.2); padding: 10px; border-radius: 4px; color: white; cursor: pointer;' 
                        }, item)
                    )
                ),
                // 主内容区
                h('div', { 
                    style: 'flex: 1; display: flex; flex-direction: column; gap: 15px;' 
                },
                    // 头部
                    h('div', { 
                        style: 'height: 80px; background: #f57c00; border-radius: 4px; padding: 15px; display: flex; align-items: center; color: white; font-size: 24px; font-weight: bold;' 
                    }, 'Header'),
                    // 内容区域
                    h('div', { 
                        style: 'flex: 1; background: #ffe0b2; border-radius: 4px; padding: 15px; overflow-y: auto;' 
                    },
                        h('h3', { style: 'margin: 0 0 10px 0; color: #e65100;' }, 'Main Content'),
                        ...Array.from({ length: 10 }, (_, i) => 
                            h('p', { key: i, style: 'margin: 0 0 10px 0; color: #666;' }, 
                                `Paragraph ${i + 1}: Lorem ipsum dolor sit amet, consectetur adipiscing elit.`
                            )
                        )
                    ),
                    // 底部
                    h('div', { 
                        style: 'height: 60px; background: #ef6c00; border-radius: 4px; padding: 15px; display: flex; align-items: center; justify-content: center; color: white;' 
                    }, 'Footer')
                )
            )
        ),
        
        // 圣杯布局
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'Holy Grail Layout'),
            h('div', { 
                style: 'display: flex; flex-direction: column; height: 500px; background: #f3e5f5; border-radius: 4px; overflow: hidden;' 
            },
                // 顶部导航
                h('div', { 
                    style: 'height: 60px; background: #9c27b0; color: white; display: flex; align-items: center; padding: 0 20px; font-size: 20px; font-weight: bold;' 
                }, 'Navigation Bar'),
                // 中间内容区
                h('div', { 
                    style: 'flex: 1; display: flex; overflow: hidden;' 
                },
                    // 左侧边栏
                    h('div', { 
                        style: 'width: 180px; background: #ce93d8; padding: 15px; overflow-y: auto;' 
                    },
                        h('h4', { style: 'margin: 0 0 10px 0; color: #4a148c;' }, 'Left Sidebar'),
                        ...Array.from({ length: 8 }, (_, i) => 
                            h('div', { 
                                key: i,
                                style: 'padding: 8px; margin-bottom: 5px; background: rgba(255,255,255,0.5); border-radius: 4px; font-size: 14px;' 
                            }, `Item ${i + 1}`)
                        )
                    ),
                    // 主内容
                    h('div', { 
                        style: 'flex: 1; background: white; padding: 20px; overflow-y: auto;' 
                    },
                        h('h3', { style: 'margin: 0 0 15px 0; color: #7b1fa2;' }, 'Main Content Area'),
                        ...Array.from({ length: 15 }, (_, i) => 
                            h('p', { key: i, style: 'margin: 0 0 10px 0; color: #666;' }, 
                                `Content paragraph ${i + 1}: Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.`
                            )
                        )
                    ),
                    // 右侧边栏
                    h('div', { 
                        style: 'width: 180px; background: #ba68c8; padding: 15px; overflow-y: auto;' 
                    },
                        h('h4', { style: 'margin: 0 0 10px 0; color: #4a148c;' }, 'Right Sidebar'),
                        h('div', { style: 'padding: 10px; background: rgba(255,255,255,0.5); border-radius: 4px; margin-bottom: 10px;' },
                            h('div', { style: 'font-weight: bold; margin-bottom: 5px;' }, 'Widget 1'),
                            h('div', { style: 'font-size: 12px;' }, 'Some content here')
                        ),
                        h('div', { style: 'padding: 10px; background: rgba(255,255,255,0.5); border-radius: 4px; margin-bottom: 10px;' },
                            h('div', { style: 'font-weight: bold; margin-bottom: 5px;' }, 'Widget 2'),
                            h('div', { style: 'font-size: 12px;' }, 'More content')
                        )
                    )
                ),
                // 底部
                h('div', { 
                    style: 'height: 50px; background: #7b1fa2; color: white; display: flex; align-items: center; justify-content: center;' 
                }, 'Footer © 2024')
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Nested Layouts Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(NestedLayoutsTest), root);

console.log('[Test 05] Nested layouts test rendered');

