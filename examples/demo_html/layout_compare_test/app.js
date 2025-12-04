/**
 * 布局渲染对比测试 - 同时在浏览器和 MBink 中运行
 * 用于验证原生布局引擎与浏览器的渲染结果一致性
 * 
 * 测试覆盖:
 * - Block 布局 (width, height, margin, padding, border)
 * - Flexbox 布局 (flex-direction, justify-content, align-items, gap)
 * - 尺寸约束 (min-width, max-width, min-height, max-height)
 * - 百分比布局
 */

var h = Preact.h;
var render = Preact.render;

// ========== 测试容器样式 ==========
var testContainerStyle = 'margin: 10px 0; padding: 10px; border: 1px solid #ddd; background: #f9f9f9;';
var testBoxStyle = 'background: #4CAF50; color: white; padding: 10px; text-align: center;';
var testBoxAltStyle = 'background: #2196F3; color: white; padding: 10px; text-align: center;';
var testBoxHighlightStyle = 'background: #FF9800; color: white; padding: 10px; text-align: center;';

// ========== 测试1: Block 基础布局 ==========
function BlockBasicTest() {
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '1. Block 基础布局'),
        
        // 1.1 固定宽高
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.1 固定宽高 (200x100)'),
            h('div', { style: testBoxStyle + 'width: 200px; height: 100px;' }, '200x100')
        ),
        
        // 1.2 百分比宽度
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.2 百分比宽度 (50%, 75%, 100%)'),
            h('div', { style: testBoxStyle + 'width: 50%; margin-bottom: 5px;' }, '50%'),
            h('div', { style: testBoxAltStyle + 'width: 75%; margin-bottom: 5px;' }, '75%'),
            h('div', { style: testBoxHighlightStyle + 'width: 100%;' }, '100%')
        ),
        
        // 1.3 Margin 测试
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.3 Margin 测试'),
            h('div', { style: testBoxStyle + 'margin: 10px;' }, 'margin: 10px'),
            h('div', { style: testBoxAltStyle + 'margin: 20px 10px;' }, 'margin: 20px 10px'),
            h('div', { style: testBoxHighlightStyle + 'margin-left: 50px;' }, 'margin-left: 50px')
        ),
        
        // 1.4 Padding 测试
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.4 Padding 测试'),
            h('div', { style: testBoxStyle + 'padding: 20px;' }, 'padding: 20px'),
            h('div', { style: testBoxAltStyle + 'padding: 10px 30px;' }, 'padding: 10px 30px'),
            h('div', { style: testBoxHighlightStyle + 'padding: 5px 10px 15px 20px;' }, 'padding: 5 10 15 20')
        ),
        
        // 1.5 Border 测试
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.5 Border 测试'),
            h('div', { style: 'background: #4CAF50; color: white; padding: 10px; border: 1px solid #333; margin-bottom: 5px;' }, 'border: 1px'),
            h('div', { style: 'background: #2196F3; color: white; padding: 10px; border: 5px solid #333; margin-bottom: 5px;' }, 'border: 5px'),
            h('div', { style: 'background: #FF9800; color: white; padding: 10px; border: 10px solid #333;' }, 'border: 10px')
        )
    );
}

// ========== 测试2: Flexbox Row 布局 ==========
function FlexboxRowTest() {
    // 添加固定宽度以便测试 justify-content
    var flexContainer = 'display: flex; gap: 10px; padding: 10px; background: #eee; width: 400px;';
    
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '2. Flexbox Row 布局'),
        
        // 2.1 基础 flex-direction: row
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.1 flex-direction: row'),
            h('div', { style: flexContainer + 'flex-direction: row;' },
                h('div', { style: testBoxStyle + 'width: 80px;' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 80px;' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 80px;' }, 'C')
            )
        ),
        
        // 2.2 justify-content
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.2 justify-content: flex-start'),
            h('div', { style: flexContainer + 'justify-content: flex-start;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.3 justify-content: center'),
            h('div', { style: flexContainer + 'justify-content: center;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.4 justify-content: flex-end'),
            h('div', { style: flexContainer + 'justify-content: flex-end;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.5 justify-content: space-between'),
            h('div', { style: flexContainer + 'justify-content: space-between;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.6 justify-content: space-around'),
            h('div', { style: flexContainer + 'justify-content: space-around;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.7 justify-content: space-evenly'),
            h('div', { style: flexContainer + 'justify-content: space-evenly;' },
                h('div', { style: testBoxStyle + 'width: 60px;' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;' }, '3')
            )
        )
    );
}

// ========== 测试3: Flexbox align-items ==========
function FlexboxAlignTest() {
    var flexContainer = 'display: flex; gap: 10px; padding: 10px; background: #eee; height: 120px;';
    
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '3. Flexbox align-items'),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.1 align-items: flex-start'),
            h('div', { style: flexContainer + 'align-items: flex-start;' },
                h('div', { style: testBoxStyle }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;' }, 'C')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.2 align-items: center'),
            h('div', { style: flexContainer + 'align-items: center;' },
                h('div', { style: testBoxStyle }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;' }, 'C')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.3 align-items: flex-end'),
            h('div', { style: flexContainer + 'align-items: flex-end;' },
                h('div', { style: testBoxStyle }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;' }, 'C')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.4 align-items: stretch'),
            h('div', { style: flexContainer + 'align-items: stretch;' },
                h('div', { style: testBoxStyle }, 'A'),
                h('div', { style: testBoxAltStyle }, 'B'),
                h('div', { style: testBoxHighlightStyle }, 'C')
            )
        )
    );
}

// ========== 测试4: Flex grow/shrink ==========
function FlexGrowShrinkTest() {
    // 添加固定宽度以便测试 flex-grow
    var flexContainer = 'display: flex; gap: 10px; padding: 10px; background: #eee; width: 500px;';
    
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '4. Flex grow/shrink'),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.1 flex-grow: 1 (equal)'),
            h('div', { style: flexContainer },
                h('div', { style: testBoxStyle + 'flex-grow: 1;' }, 'grow:1'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 1;' }, 'grow:1'),
                h('div', { style: testBoxHighlightStyle + 'flex-grow: 1;' }, 'grow:1')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.2 flex-grow: 1, 2, 1'),
            h('div', { style: flexContainer },
                h('div', { style: testBoxStyle + 'flex-grow: 1;' }, 'grow:1'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 2;' }, 'grow:2'),
                h('div', { style: testBoxHighlightStyle + 'flex-grow: 1;' }, 'grow:1')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.3 flex-basis: 100px'),
            h('div', { style: flexContainer },
                h('div', { style: testBoxStyle + 'flex-basis: 100px;' }, 'basis:100'),
                h('div', { style: testBoxAltStyle + 'flex-basis: 150px;' }, 'basis:150'),
                h('div', { style: testBoxHighlightStyle + 'flex-basis: 200px;' }, 'basis:200')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.4 flex: 1 (shorthand)'),
            h('div', { style: flexContainer },
                h('div', { style: testBoxStyle + 'flex: 1;' }, 'flex:1'),
                h('div', { style: testBoxAltStyle + 'flex: 2;' }, 'flex:2'),
                h('div', { style: testBoxHighlightStyle + 'flex: 3;' }, 'flex:3')
            )
        )
    );
}

// ========== 测试5: Flexbox Column 布局 ==========
function FlexboxColumnTest() {
    var flexContainer = 'display: flex; flex-direction: column; gap: 10px; padding: 10px; background: #eee;';
    
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '5. Flexbox Column 布局'),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.1 flex-direction: column'),
            h('div', { style: flexContainer + 'height: 300px;' },
                h('div', { style: testBoxStyle }, 'A'),
                h('div', { style: testBoxAltStyle }, 'B'),
                h('div', { style: testBoxHighlightStyle }, 'C')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.2 column + justify-content: space-between'),
            h('div', { style: flexContainer + 'height: 300px; justify-content: space-between;' },
                h('div', { style: testBoxStyle }, 'Top'),
                h('div', { style: testBoxAltStyle }, 'Middle'),
                h('div', { style: testBoxHighlightStyle }, 'Bottom')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.3 column + flex-grow'),
            h('div', { style: flexContainer + 'height: 250px;' },
                h('div', { style: testBoxStyle }, 'Fixed'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 1;' }, 'Grow'),
                h('div', { style: testBoxHighlightStyle }, 'Fixed')
            )
        )
    );
}

// ========== 测试6: 尺寸约束 ==========
function SizeConstraintTest() {
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '6. 尺寸约束'),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.1 min-width: 200px (容器300px)'),
            h('div', { style: 'width: 300px; background: #ddd; padding: 5px;' },
                h('div', { style: testBoxStyle + 'min-width: 200px;' }, 'min-w:200')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.2 max-width: 150px'),
            h('div', { style: 'width: 300px; background: #ddd; padding: 5px;' },
                h('div', { style: testBoxStyle + 'max-width: 150px;' }, 'max-w:150')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.3 min-height: 80px'),
            h('div', { style: 'background: #ddd; padding: 5px;' },
                h('div', { style: testBoxStyle + 'min-height: 80px;' }, 'min-h:80')
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.4 max-height: 50px + overflow'),
            h('div', { style: 'background: #ddd; padding: 5px;' },
                h('div', { style: testBoxStyle + 'max-height: 50px; overflow: hidden;' }, 
                    'max-h:50 This text might be cut off if too long for the container')
            )
        )
    );
}

// ========== 测试7: 嵌套布局 ==========
function NestedLayoutTest() {
    return h('section', { style: 'margin: 20px 0;' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '7. 嵌套布局'),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.1 Block 内嵌 Flex'),
            h('div', { style: 'padding: 10px; background: #ddd;' },
                h('div', { style: 'display: flex; gap: 10px; padding: 10px; background: #bbb;' },
                    h('div', { style: testBoxStyle + 'flex: 1;' }, 'Flex 1'),
                    h('div', { style: testBoxAltStyle + 'flex: 1;' }, 'Flex 2')
                )
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.2 Flex 内嵌 Block'),
            h('div', { style: 'display: flex; gap: 10px; padding: 10px; background: #ddd;' },
                h('div', { style: 'flex: 1; background: #bbb; padding: 10px;' },
                    h('div', { style: testBoxStyle + 'margin-bottom: 5px;' }, 'Block A'),
                    h('div', { style: testBoxAltStyle }, 'Block B')
                ),
                h('div', { style: 'flex: 1; background: #bbb; padding: 10px;' },
                    h('div', { style: testBoxHighlightStyle }, 'Block C')
                )
            )
        ),
        
        h('div', { style: testContainerStyle },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.3 复杂嵌套'),
            h('div', { style: 'display: flex; gap: 10px; padding: 10px; background: #ddd;' },
                h('div', { style: 'flex: 2; display: flex; flex-direction: column; gap: 5px;' },
                    h('div', { style: testBoxStyle }, 'Header'),
                    h('div', { style: 'display: flex; gap: 5px; flex: 1;' },
                        h('div', { style: testBoxAltStyle + 'flex: 1;' }, 'Side'),
                        h('div', { style: testBoxHighlightStyle + 'flex: 2;' }, 'Main')
                    ),
                    h('div', { style: testBoxStyle }, 'Footer')
                ),
                h('div', { style: 'flex: 1;' },
                    h('div', { style: testBoxAltStyle + 'height: 100%;' }, 'Right Panel')
                )
            )
        )
    );
}

// ========== 主应用 ==========
function App() {
    return h('div', { style: 'padding: 20px; font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto;' },
        h('h1', { style: 'margin: 0 0 20px 0; color: #333;' }, '🔬 Layout Compare Test'),
        h('p', { style: 'margin: 0 0 20px 0; color: #666;' }, 
            '浏览器与 MBink 布局渲染对比测试 - 使用同一 app.js 在两端运行'),
        
        h(BlockBasicTest),
        h(FlexboxRowTest),
        h(FlexboxAlignTest),
        h(FlexGrowShrinkTest),
        h(FlexboxColumnTest),
        h(SizeConstraintTest),
        h(NestedLayoutTest),
        
        h('footer', { style: 'margin-top: 30px; padding: 20px; text-align: center; background: #f5f5f5; border-radius: 4px;' },
            h('p', { style: 'margin: 0; color: #666;' }, '✅ Layout Compare Test - Powered by MBink')
        )
    );
}

// 渲染
render(h(App), document.body);
console.log('Layout Compare Test app rendered!');

