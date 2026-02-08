/**
 * 测试场景 7: 文本和字体样式
 * 测试目标: 验证字体渲染、文本对齐、行高、字间距
 * 潜在 BUG: 字体加载失败、文本溢出、行高计算错误
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\07_text_fonts.js
 */

import { h, render } from 'preact';

function TextFontsTest() {
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Text & Fonts Test'),
        
        // 字体大小
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'Font Sizes'),
            h('div', { style: 'background: #e3f2fd; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'font-size: 10px; margin: 5px 0;' }, '10px - Very small text'),
                h('p', { style: 'font-size: 12px; margin: 5px 0;' }, '12px - Small text'),
                h('p', { style: 'font-size: 14px; margin: 5px 0;' }, '14px - Normal text'),
                h('p', { style: 'font-size: 16px; margin: 5px 0;' }, '16px - Medium text'),
                h('p', { style: 'font-size: 18px; margin: 5px 0;' }, '18px - Large text'),
                h('p', { style: 'font-size: 24px; margin: 5px 0;' }, '24px - Extra large text'),
                h('p', { style: 'font-size: 32px; margin: 5px 0;' }, '32px - Huge text'),
                h('p', { style: 'font-size: 48px; margin: 5px 0;' }, '48px - Giant')
            )
        ),
        
        // 字体粗细
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'Font Weights'),
            h('div', { style: 'background: #e8f5e9; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'font-weight: 100; margin: 5px 0; font-size: 18px;' }, 'Font weight 100 - Thin'),
                h('p', { style: 'font-weight: 200; margin: 5px 0; font-size: 18px;' }, 'Font weight 200 - Extra Light'),
                h('p', { style: 'font-weight: 300; margin: 5px 0; font-size: 18px;' }, 'Font weight 300 - Light'),
                h('p', { style: 'font-weight: 400; margin: 5px 0; font-size: 18px;' }, 'Font weight 400 - Normal'),
                h('p', { style: 'font-weight: 500; margin: 5px 0; font-size: 18px;' }, 'Font weight 500 - Medium'),
                h('p', { style: 'font-weight: 600; margin: 5px 0; font-size: 18px;' }, 'Font weight 600 - Semi Bold'),
                h('p', { style: 'font-weight: 700; margin: 5px 0; font-size: 18px;' }, 'Font weight 700 - Bold'),
                h('p', { style: 'font-weight: 800; margin: 5px 0; font-size: 18px;' }, 'Font weight 800 - Extra Bold'),
                h('p', { style: 'font-weight: 900; margin: 5px 0; font-size: 18px;' }, 'Font weight 900 - Black')
            )
        ),
        
        // 文本对齐
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'Text Alignment'),
            h('div', { style: 'background: #fff3e0; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'text-align: left; margin: 0; padding: 5px; background: #ffe0b2;' }, 'Left aligned text - This is the default alignment')
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'text-align: center; margin: 0; padding: 5px; background: #ffe0b2;' }, 'Center aligned text - Centered in the container')
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'text-align: right; margin: 0; padding: 5px; background: #ffe0b2;' }, 'Right aligned text - Aligned to the right edge')
                ),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px;' },
                    h('p', { style: 'text-align: justify; margin: 0; padding: 5px; background: #ffe0b2;' }, 
                        'Justified text - This text is justified which means it stretches to fill the entire width of the container. The spacing between words is adjusted to make the text align with both left and right margins.'
                    )
                )
            )
        ),
        
        // 行高
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'Line Height'),
            h('div', { style: 'display: flex; gap: 15px;' },
                h('div', { style: 'flex: 1; background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'line-height: 1'),
                    h('p', { style: 'line-height: 1; margin: 0; background: white; padding: 10px; border-radius: 4px;' }, 
                        'This text has a line height of 1. The lines are very close together. Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
                    )
                ),
                h('div', { style: 'flex: 1; background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'line-height: 1.5'),
                    h('p', { style: 'line-height: 1.5; margin: 0; background: white; padding: 10px; border-radius: 4px;' }, 
                        'This text has a line height of 1.5. This is a comfortable reading distance. Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
                    )
                ),
                h('div', { style: 'flex: 1; background: #f3e5f5; padding: 15px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 10px 0;' }, 'line-height: 2'),
                    h('p', { style: 'line-height: 2; margin: 0; background: white; padding: 10px; border-radius: 4px;' }, 
                        'This text has a line height of 2. The lines are spaced far apart. Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
                    )
                )
            )
        ),
        
        // 字间距和词间距
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'Letter & Word Spacing'),
            h('div', { style: 'background: #ffebee; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0; font-size: 14px;' }, 'letter-spacing: -1px'),
                    h('p', { style: 'letter-spacing: -1px; margin: 0; font-size: 16px;' }, 'Tight letter spacing makes text compact')
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0; font-size: 14px;' }, 'letter-spacing: 0 (normal)'),
                    h('p', { style: 'letter-spacing: 0; margin: 0; font-size: 16px;' }, 'Normal letter spacing is the default')
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0; font-size: 14px;' }, 'letter-spacing: 2px'),
                    h('p', { style: 'letter-spacing: 2px; margin: 0; font-size: 16px;' }, 'Wide letter spacing spreads out text')
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0; font-size: 14px;' }, 'word-spacing: 10px'),
                    h('p', { style: 'word-spacing: 10px; margin: 0; font-size: 16px;' }, 'Word spacing affects the space between words in a sentence')
                )
            )
        ),
        
        // 文本装饰
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00bcd4;' }, 'Text Decoration'),
            h('div', { style: 'background: #e0f7fa; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'text-decoration: none; margin: 5px 0; font-size: 18px;' }, 'No decoration - Plain text'),
                h('p', { style: 'text-decoration: underline; margin: 5px 0; font-size: 18px;' }, 'Underline - Text with underline'),
                h('p', { style: 'text-decoration: overline; margin: 5px 0; font-size: 18px;' }, 'Overline - Text with overline'),
                h('p', { style: 'text-decoration: line-through; margin: 5px 0; font-size: 18px;' }, 'Line-through - Strikethrough text'),
                h('p', { style: 'text-decoration: underline overline; margin: 5px 0; font-size: 18px;' }, 'Multiple - Underline and overline')
            )
        ),
        
        // 文本转换
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #673ab7;' }, 'Text Transform'),
            h('div', { style: 'background: #ede7f6; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'text-transform: none; margin: 5px 0; font-size: 18px;' }, 'None - Original Text Case'),
                h('p', { style: 'text-transform: uppercase; margin: 5px 0; font-size: 18px;' }, 'Uppercase - all letters uppercase'),
                h('p', { style: 'text-transform: lowercase; margin: 5px 0; font-size: 18px;' }, 'LOWERCASE - ALL LETTERS LOWERCASE'),
                h('p', { style: 'text-transform: capitalize; margin: 5px 0; font-size: 18px;' }, 'capitalize - first letter of each word')
            )
        ),
        
        // 文本阴影
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #795548;' }, 'Text Shadow'),
            h('div', { style: 'background: #efebe9; padding: 15px; border-radius: 4px;' },
                h('p', { style: 'text-shadow: 2px 2px 4px rgba(0,0,0,0.3); margin: 10px 0; font-size: 24px; font-weight: bold;' }, 
                    'Simple shadow'
                ),
                h('p', { style: 'text-shadow: 0 0 10px rgba(33,150,243,0.8); margin: 10px 0; font-size: 24px; font-weight: bold; color: #2196f3;' }, 
                    'Glow effect'
                ),
                h('p', { style: 'text-shadow: 3px 3px 0 #ff9800, 6px 6px 0 #f44336; margin: 10px 0; font-size: 24px; font-weight: bold; color: #ffc107;' }, 
                    'Multiple shadows'
                ),
                h('p', { style: 'text-shadow: 1px 1px 2px black, 0 0 25px blue, 0 0 5px darkblue; margin: 10px 0; font-size: 24px; font-weight: bold; color: white;' }, 
                    'Complex shadow'
                )
            )
        ),
        
        // 长文本处理
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #e91e63;' }, 'Long Text Handling'),
            h('div', { style: 'background: #fce4ec; padding: 15px; border-radius: 4px;' },
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0;' }, 'Normal wrap:'),
                    h('p', { style: 'margin: 0;' }, 
                        'This is a very long paragraph that will wrap normally when it reaches the edge of the container. The text will continue on the next line automatically. Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
                    )
                ),
                h('div', { style: 'background: white; padding: 10px; margin-bottom: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0;' }, 'white-space: nowrap + overflow: hidden:'),
                    h('p', { style: 'margin: 0; white-space: nowrap; overflow: hidden;' }, 
                        'This text will not wrap and will be clipped at the container edge. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt.'
                    )
                ),
                h('div', { style: 'background: white; padding: 10px; border-radius: 4px;' },
                    h('p', { style: 'font-weight: bold; margin: 0 0 5px 0;' }, 'Ellipsis:'),
                    h('p', { style: 'margin: 0; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;' }, 
                        'This text will be truncated with an ellipsis (...) when it overflows. Lorem ipsum dolor sit amet, consectetur adipiscing elit.'
                    )
                )
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Text & Fonts Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(TextFontsTest), root);

console.log('[Test 07] Text and fonts test rendered');

