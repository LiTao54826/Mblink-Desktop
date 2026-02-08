/**
 * 测试场景 3: 盒模型和溢出处理
 * 测试目标: 验证 box-sizing、overflow、min/max 尺寸
 * 潜在 BUG: 盒模型计算错误、溢出处理异常、尺寸约束失效
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\03_box_model.js
 */

import { h, render } from 'preact';

function BoxModelTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Box Model & Overflow Test'),
        
        // box-sizing 测试
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'box-sizing'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'content-box (default)'),
            h('div', { style: 'background: #e3f2fd; padding: 15px; border-radius: 4px; margin-bottom: 15px;' },
                h('div', { 
                    style: 'width: 200px; height: 100px; padding: 20px; border: 5px solid #2196f3; background: #90caf9; box-sizing: content-box;' 
                },
                    h('div', { style: 'background: white; padding: 5px; font-size: 12px;' }, 
                        'Width: 200px + 20px padding + 5px border = 250px total'
                    )
                )
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'border-box'),
            h('div', { style: 'background: #e3f2fd; padding: 15px; border-radius: 4px;' },
                h('div', { 
                    style: 'width: 200px; height: 100px; padding: 20px; border: 5px solid #2196f3; background: #90caf9; box-sizing: border-box;' 
                },
                    h('div', { style: 'background: white; padding: 5px; font-size: 12px;' }, 
                        'Width: 200px total (includes padding and border)'
                    )
                )
            )
        ),
        
        // overflow 测试
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'overflow'),
            
            h('div', { style: 'display: flex; gap: 15px; flex-wrap: wrap;' },
                // overflow: visible
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'visible'),
                    h('div', { 
                        style: 'width: 150px; height: 100px; background: #e8f5e9; border: 2px solid #4caf50; overflow: visible; position: relative;' 
                    },
                        h('div', { 
                            style: 'width: 200px; background: #4caf50; color: white; padding: 10px;' 
                        }, 'Content overflows the container')
                    )
                ),
                
                // overflow: hidden
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'hidden'),
                    h('div', { 
                        style: 'width: 150px; height: 100px; background: #fff3e0; border: 2px solid #ff9800; overflow: hidden;' 
                    },
                        h('div', { 
                            style: 'width: 200px; background: #ff9800; color: white; padding: 10px;' 
                        }, 'Content is clipped at container boundary')
                    )
                ),
                
                // overflow: scroll
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'scroll'),
                    h('div', { 
                        style: 'width: 150px; height: 100px; background: #f3e5f5; border: 2px solid #9c27b0; overflow: scroll;' 
                    },
                        h('div', { 
                            style: 'width: 250px; background: #9c27b0; color: white; padding: 10px;' 
                        }, 'Scrollbars always visible. Content can be scrolled horizontally and vertically.')
                    )
                ),
                
                // overflow: auto
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'auto'),
                    h('div', { 
                        style: 'width: 150px; height: 100px; background: #e1f5fe; border: 2px solid #00bcd4; overflow: auto;' 
                    },
                        h('div', { 
                            style: 'width: 250px; background: #00bcd4; color: white; padding: 10px;' 
                        }, 'Scrollbars appear only when needed. This content is wider than container.')
                    )
                )
            )
        ),
        
        // overflow-x 和 overflow-y
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'overflow-x & overflow-y'),
            
            h('div', { style: 'display: flex; gap: 15px; flex-wrap: wrap;' },
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'overflow-x: scroll'),
                    h('div', { 
                        style: 'width: 200px; height: 100px; background: #fff3e0; border: 2px solid #ff9800; overflow-x: scroll; overflow-y: hidden;' 
                    },
                        h('div', { 
                            style: 'width: 400px; background: #ff9800; color: white; padding: 10px;' 
                        }, 'Horizontal scroll only')
                    )
                ),
                
                h('div', { style: 'flex: 1; min-width: 200px;' },
                    h('p', { style: 'margin: 0 0 5px 0; font-weight: bold; color: #666;' }, 'overflow-y: scroll'),
                    h('div', { 
                        style: 'width: 200px; height: 100px; background: #f3e5f5; border: 2px solid #9c27b0; overflow-x: hidden; overflow-y: scroll;' 
                    },
                        h('div', { 
                            style: 'background: #9c27b0; color: white; padding: 10px;' 
                        }, 'Vertical scroll only. ', h('br'), 'Line 2', h('br'), 'Line 3', h('br'), 'Line 4', h('br'), 'Line 5', h('br'), 'Line 6')
                    )
                )
            )
        ),
        
        // min/max width 和 height
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'min/max width & height'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'min-width: 200px, max-width: 400px'),
            h('div', { style: 'background: #f3e5f5; padding: 15px; border-radius: 4px; margin-bottom: 15px;' },
                h('div', { 
                    style: 'min-width: 200px; max-width: 400px; background: #9c27b0; color: white; padding: 15px; border-radius: 4px;' 
                },
                    'This box has min-width of 200px and max-width of 400px. Try resizing the window to see the effect.'
                )
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'min-height: 100px, max-height: 150px with overflow'),
            h('div', { style: 'background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                h('div', { 
                    style: 'min-height: 100px; max-height: 150px; background: #9c27b0; color: white; padding: 15px; border-radius: 4px; overflow-y: auto;' 
                },
                    'This box has min-height of 100px and max-height of 150px. ',
                    h('br'), 'Line 2', h('br'), 'Line 3', h('br'), 'Line 4', h('br'), 'Line 5', 
                    h('br'), 'Line 6', h('br'), 'Line 7', h('br'), 'Line 8', h('br'), 'Line 9'
                )
            )
        ),
        
        // 文本溢出
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'Text Overflow'),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'text-overflow: ellipsis'),
            h('div', { 
                style: 'width: 300px; background: #ffebee; padding: 15px; border-radius: 4px; margin-bottom: 15px;' 
            },
                h('div', { 
                    style: 'white-space: nowrap; overflow: hidden; text-overflow: ellipsis; background: #f44336; color: white; padding: 10px; border-radius: 4px;' 
                }, 'This is a very long text that will be truncated with an ellipsis when it overflows the container')
            ),
            
            h('p', { style: 'margin: 10px 0 5px 0; font-weight: bold; color: #666;' }, 'word-wrap: break-word'),
            h('div', { 
                style: 'width: 300px; background: #ffebee; padding: 15px; border-radius: 4px;' 
            },
                h('div', { 
                    style: 'word-wrap: break-word; background: #f44336; color: white; padding: 10px; border-radius: 4px;' 
                }, 'ThisIsAVeryLongWordWithoutSpacesThatShouldBreakIntoMultipleLinesWhenItOverflowsTheContainer')
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Box Model & Overflow Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(BoxModelTest), root);

console.log('[Test 03] Box model test rendered');

