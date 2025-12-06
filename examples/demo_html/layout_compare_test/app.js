/**
 * 布局渲染对比测试 - 同时在浏览器和 MBink 中运行
 * 用于验证原生布局引擎与浏览器的渲染结果一致性
 *
 * 完整测试覆盖:
 * - Box Model (box-sizing, margin, padding, border)
 * - Block 布局 (inline-block, overflow, min/max)
 * - Flexbox 布局 (direction, justify, align, wrap, gap, grow/shrink)
 * - Grid 布局 (template-columns, fr, gap, repeat, span)
 * - Positioning (relative, absolute, fixed, z-index)
 * - Text/IFC (text-align, line-height, white-space)
 */

var h = Preact.h;
var render = Preact.render;

// ========== 测试容器样式 ==========
// 注意: 所有样式都添加 box-sizing: border-box 以匹配浏览器的 * { box-sizing: border-box; }
var boxSizing = 'box-sizing: border-box; ';
var testContainerStyle = boxSizing + 'margin: 10px 0; padding: 10px; border: 1px solid #ddd; background: #f9f9f9;';
var testBoxStyle = boxSizing + 'background: #4CAF50; color: white; padding: 10px; text-align: center;';
var testBoxAltStyle = boxSizing + 'background: #2196F3; color: white; padding: 10px; text-align: center;';
var testBoxHighlightStyle = boxSizing + 'background: #FF9800; color: white; padding: 10px; text-align: center;';
var testBoxPurpleStyle = boxSizing + 'background: #9C27B0; color: white; padding: 10px; text-align: center;';

// ========== 测试1: Block 基础布局 ==========
function BlockBasicTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-block-basic' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '1. Block 基础布局'),

        // 1.1 固定宽高
        h('div', { style: testContainerStyle, id: 'BM-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.1 固定宽高 (200x100)'),
            h('div', { style: testBoxStyle + 'width: 200px; height: 100px;', id: 'BM-01-box' }, '200x100')
        ),

        // 1.2 百分比宽度
        h('div', { style: testContainerStyle, id: 'BM-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.2 百分比宽度 (50%, 75%, 100%)'),
            h('div', { style: testBoxStyle + 'width: 50%; margin-bottom: 5px;', id: 'BM-02-A' }, '50%'),
            h('div', { style: testBoxAltStyle + 'width: 75%; margin-bottom: 5px;', id: 'BM-02-B' }, '75%'),
            h('div', { style: testBoxHighlightStyle + 'width: 100%;', id: 'BM-02-C' }, '100%')
        ),

        // 1.3 Margin 测试
        h('div', { style: testContainerStyle, id: 'BM-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.3 Margin 测试'),
            h('div', { style: testBoxStyle + 'margin: 10px;', id: 'BM-03-A' }, 'margin: 10px'),
            h('div', { style: testBoxAltStyle + 'margin: 20px 10px;', id: 'BM-03-B' }, 'margin: 20px 10px'),
            h('div', { style: testBoxHighlightStyle + 'margin-left: 50px;', id: 'BM-03-C' }, 'margin-left: 50px')
        ),

        // 1.4 Padding 测试
        h('div', { style: testContainerStyle, id: 'BM-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.4 Padding 测试'),
            h('div', { style: testBoxStyle + 'padding: 20px;', id: 'BM-04-A' }, 'padding: 20px'),
            h('div', { style: testBoxAltStyle + 'padding: 10px 30px;', id: 'BM-04-B' }, 'padding: 10px 30px'),
            h('div', { style: testBoxHighlightStyle + 'padding: 5px 10px 15px 20px;', id: 'BM-04-C' }, 'padding: 5 10 15 20')
        ),

        // 1.5 Border 测试
        h('div', { style: testContainerStyle, id: 'BM-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '1.5 Border 测试'),
            h('div', { style: boxSizing + 'background: #4CAF50; color: white; padding: 10px; border: 1px solid #333; margin-bottom: 5px;', id: 'BM-05-A' }, 'border: 1px'),
            h('div', { style: boxSizing + 'background: #2196F3; color: white; padding: 10px; border: 5px solid #333; margin-bottom: 5px;', id: 'BM-05-B' }, 'border: 5px'),
            h('div', { style: boxSizing + 'background: #FF9800; color: white; padding: 10px; border: 10px solid #333;', id: 'BM-05-C' }, 'border: 10px')
        ),

        // BM-07 box-sizing: border-box
        h('div', { style: testContainerStyle, id: 'BM-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-07 box-sizing: border-box'),
            h('div', { style: boxSizing + 'background: #ddd; padding: 5px;', id: 'BM-07-wrap' },
                h('div', { style: testBoxStyle + 'box-sizing: border-box; width: 200px; padding: 20px; border: 5px solid #333;', id: 'BM-07-box' }, '200px total')
            )
        ),

        // BM-08 box-sizing: content-box
        h('div', { style: testContainerStyle, id: 'BM-08' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-08 box-sizing: content-box'),
            h('div', { style: boxSizing + 'background: #ddd; padding: 5px;', id: 'BM-08-wrap' },
                h('div', { style: testBoxStyle + 'box-sizing: content-box; width: 200px; padding: 20px; border: 5px solid #333;', id: 'BM-08-box' }, '200px content')
            )
        ),

        // BM-09 margin: auto 水平居中
        h('div', { style: testContainerStyle, id: 'BM-09' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-09 margin: auto 水平居中'),
            h('div', { style: boxSizing + 'background: #ddd; padding: 5px;', id: 'BM-09-wrap' },
                h('div', { style: testBoxStyle + 'width: 200px; margin: 0 auto;', id: 'BM-09-box' }, 'margin: 0 auto')
            )
        ),

        // BM-10 margin 负值
        h('div', { style: testContainerStyle, id: 'BM-10' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-10 margin 负值'),
            h('div', { style: boxSizing + 'background: #ddd; padding: 20px;', id: 'BM-10-wrap' },
                h('div', { style: testBoxStyle + 'margin-left: -10px;', id: 'BM-10-box' }, 'margin-left: -10px')
            )
        ),

        // BM-11 margin 合并 (垂直)
        h('div', { style: testContainerStyle, id: 'BM-11' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-11 margin 合并 (垂直)'),
            h('div', { style: boxSizing + 'background: #ddd; padding: 5px;', id: 'BM-11-wrap' },
                h('div', { style: testBoxStyle + 'margin: 20px 0;', id: 'BM-11-A' }, 'margin: 20px 0'),
                h('div', { style: testBoxAltStyle + 'margin: 20px 0;', id: 'BM-11-B' }, 'margin: 20px 0')
            )
        ),

        // BM-12 padding 百分比
        h('div', { style: testContainerStyle, id: 'BM-12' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-12 padding 百分比 (10%)'),
            h('div', { style: boxSizing + 'width: 300px; background: #ddd;', id: 'BM-12-wrap' },
                h('div', { style: testBoxStyle + 'padding: 10%;', id: 'BM-12-box' }, 'padding: 10%')
            )
        ),

        // BM-13 border 各边不同
        h('div', { style: testContainerStyle, id: 'BM-13' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-13 border 各边不同'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'BM-13-wrap' },
                h('div', { style: testBoxStyle + 'border-left: 5px solid #333; border-right: 15px solid #666; border-top: 2px solid #999; border-bottom: 10px solid #000;', id: 'BM-13-box' }, 'different borders')
            )
        ),

        // BM-14 百分比高度
        h('div', { style: testContainerStyle, id: 'BM-14' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-14 百分比高度 (50%)'),
            h('div', { style: 'height: 150px; background: #ddd; padding: 5px;', id: 'BM-14-wrap' },
                h('div', { style: testBoxStyle + 'height: 50%;', id: 'BM-14-box' }, 'height: 50%')
            )
        ),

        // BM-15 calc() 计算
        h('div', { style: testContainerStyle, id: 'BM-15' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BM-15 calc() 计算'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'BM-15-wrap' },
                h('div', { style: testBoxStyle + 'width: calc(100% - 40px);', id: 'BM-15-box' }, 'calc(100% - 40px)')
            )
        )
    );
}

// ========== Block Layout 测试 (BL-04 到 BL-10) ==========
function BlockLayoutTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-block-layout' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '1.5 Block Layout 扩展'),

        // BL-04 display: inline-block
        h('div', { style: testContainerStyle, id: 'BL-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-04 display: inline-block'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'BL-04-wrap' },
                h('div', { style: testBoxStyle + 'display: inline-block; width: 100px;', id: 'BL-04-A' }, 'inline-block'),
                h('div', { style: testBoxAltStyle + 'display: inline-block; width: 100px;', id: 'BL-04-B' }, 'inline-block'),
                h('div', { style: testBoxHighlightStyle + 'display: inline-block; width: 100px;', id: 'BL-04-C' }, 'inline-block')
            )
        ),

        // BL-05 overflow: hidden
        h('div', { style: testContainerStyle, id: 'BL-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-05 overflow: hidden'),
            h('div', { style: testBoxStyle + 'width: 200px; height: 50px; overflow: hidden;', id: 'BL-05-box' },
                'This is a long text that should be clipped because overflow is hidden and the container is too small to contain all the text.'
            )
        ),

        // BL-06 overflow: scroll
        h('div', { style: testContainerStyle, id: 'BL-06' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-06 overflow: scroll'),
            h('div', { style: testBoxStyle + 'width: 200px; height: 80px; overflow: scroll;', id: 'BL-06-box' },
                'This is a long text that needs scrolling. Line 1. Line 2. Line 3. Line 4. Line 5. More content here.'
            )
        ),

        // BL-07 overflow: auto
        h('div', { style: testContainerStyle, id: 'BL-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-07 overflow: auto'),
            h('div', { style: testBoxStyle + 'width: 200px; height: 80px; overflow: auto;', id: 'BL-07-box' },
                'This text may or may not need scrolling depending on its length. If it is long enough, scrollbars will appear automatically.'
            )
        ),

        // BL-08 min-height + 内容撑开
        h('div', { style: testContainerStyle, id: 'BL-08' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-08 min-height + 内容撑开'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'BL-08-wrap' },
                h('div', { style: testBoxStyle + 'min-height: 50px;', id: 'BL-08-A' }, 'Short'),
                h('div', { style: testBoxAltStyle + 'min-height: 50px;', id: 'BL-08-B' },
                    'This has min-height: 50px but the content is long enough to expand beyond that minimum height requirement. It should grow to fit the content.'
                )
            )
        ),

        // BL-09 max-height 截断
        h('div', { style: testContainerStyle, id: 'BL-09' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-09 max-height 截断'),
            h('div', { style: testBoxStyle + 'max-height: 60px; overflow: hidden;', id: 'BL-09-box' },
                'This content will be clipped at max-height: 60px. Line 1. Line 2. Line 3. Line 4. Line 5. This should be cut off.'
            )
        ),

        // BL-10 嵌套块级元素
        h('div', { style: testContainerStyle, id: 'BL-10' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'BL-10 嵌套块级元素'),
            h('div', { style: 'background: #ddd; padding: 15px; margin: 10px;', id: 'BL-10-L1' },
                h('div', { style: 'background: #bbb; padding: 10px; margin: 10px;', id: 'BL-10-L2' },
                    h('div', { style: 'background: #999; padding: 5px; margin: 5px;', id: 'BL-10-L3' },
                        h('div', { style: testBoxStyle, id: 'BL-10-box' }, 'Deeply nested')
                    )
                )
            )
        )
    );
}

// ========== 测试2: Flexbox Row 布局 ==========
function FlexboxRowTest() {
    // 添加固定宽度以便测试 justify-content
    var flexContainer = boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #eee; width: 400px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-flex-row' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '2. Flexbox Row 布局'),

        // 2.1 基础 flex-direction: row
        h('div', { style: testContainerStyle, id: 'FL-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.1 flex-direction: row'),
            h('div', { style: flexContainer + 'flex-direction: row;', id: 'FL-01-container' },
                h('div', { style: testBoxStyle + 'width: 80px;', id: 'FL-01-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 80px;', id: 'FL-01-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 80px;', id: 'FL-01-C' }, 'C')
            )
        ),

        // 2.2 justify-content
        h('div', { style: testContainerStyle, id: 'FL-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.2 justify-content: flex-start'),
            h('div', { style: flexContainer + 'justify-content: flex-start;', id: 'FL-02-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-02-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-02-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-02-C' }, '3')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.3 justify-content: center'),
            h('div', { style: flexContainer + 'justify-content: center;', id: 'FL-03-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-03-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-03-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-03-C' }, '3')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.4 justify-content: flex-end'),
            h('div', { style: flexContainer + 'justify-content: flex-end;', id: 'FL-04-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-04-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-04-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-04-C' }, '3')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.5 justify-content: space-between'),
            h('div', { style: flexContainer + 'justify-content: space-between;', id: 'FL-05-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-05-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-05-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-05-C' }, '3')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-06' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.6 justify-content: space-around'),
            h('div', { style: flexContainer + 'justify-content: space-around;', id: 'FL-06-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-06-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-06-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-06-C' }, '3')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '2.7 justify-content: space-evenly'),
            h('div', { style: flexContainer + 'justify-content: space-evenly;', id: 'FL-07-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'FL-07-A' }, '1'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'FL-07-B' }, '2'),
                h('div', { style: testBoxHighlightStyle + 'width: 60px;', id: 'FL-07-C' }, '3')
            )
        )
    );
}

// ========== 测试3: Flexbox align-items ==========
function FlexboxAlignTest() {
    var flexContainer = boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #eee; height: 120px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-flex-align' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '3. Flexbox align-items'),

        h('div', { style: testContainerStyle, id: 'FL-08' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.1 align-items: flex-start'),
            h('div', { style: flexContainer + 'align-items: flex-start;', id: 'FL-08-container' },
                h('div', { style: testBoxStyle, id: 'FL-08-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;', id: 'FL-08-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;', id: 'FL-08-C' }, 'C')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-09' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.2 align-items: center'),
            h('div', { style: flexContainer + 'align-items: center;', id: 'FL-09-container' },
                h('div', { style: testBoxStyle, id: 'FL-09-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;', id: 'FL-09-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;', id: 'FL-09-C' }, 'C')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-10' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.3 align-items: flex-end'),
            h('div', { style: flexContainer + 'align-items: flex-end;', id: 'FL-10-container' },
                h('div', { style: testBoxStyle, id: 'FL-10-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 50px;', id: 'FL-10-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'height: 80px;', id: 'FL-10-C' }, 'C')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-11' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '3.4 align-items: stretch'),
            h('div', { style: flexContainer + 'align-items: stretch;', id: 'FL-11-container' },
                h('div', { style: testBoxStyle, id: 'FL-11-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'FL-11-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-11-C' }, 'C')
            )
        )
    );
}

// ========== 测试4: Flex grow/shrink ==========
function FlexGrowShrinkTest() {
    // 添加固定宽度以便测试 flex-grow
    var flexContainer = boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #eee; width: 500px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-flex-grow' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '4. Flex grow/shrink'),

        h('div', { style: testContainerStyle, id: 'FL-12-A' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.1 flex-grow: 1 (equal)'),
            h('div', { style: flexContainer, id: 'FL-12-A-container' },
                h('div', { style: testBoxStyle + 'flex-grow: 1;', id: 'FL-12-A-1' }, 'grow:1'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 1;', id: 'FL-12-A-2' }, 'grow:1'),
                h('div', { style: testBoxHighlightStyle + 'flex-grow: 1;', id: 'FL-12-A-3' }, 'grow:1')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-12-B' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.2 flex-grow: 1, 2, 1'),
            h('div', { style: flexContainer, id: 'FL-12-B-container' },
                h('div', { style: testBoxStyle + 'flex-grow: 1;', id: 'FL-12-B-1' }, 'grow:1'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 2;', id: 'FL-12-B-2' }, 'grow:2'),
                h('div', { style: testBoxHighlightStyle + 'flex-grow: 1;', id: 'FL-12-B-3' }, 'grow:1')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-12-C' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.3 flex-basis: 100px'),
            h('div', { style: flexContainer, id: 'FL-12-C-container' },
                h('div', { style: testBoxStyle + 'flex-basis: 100px;', id: 'FL-12-C-1' }, 'basis:100'),
                h('div', { style: testBoxAltStyle + 'flex-basis: 150px;', id: 'FL-12-C-2' }, 'basis:150'),
                h('div', { style: testBoxHighlightStyle + 'flex-basis: 200px;', id: 'FL-12-C-3' }, 'basis:200')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-12-D' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '4.4 flex: 1 (shorthand)'),
            h('div', { style: flexContainer, id: 'FL-12-D-container' },
                h('div', { style: testBoxStyle + 'flex: 1;', id: 'FL-12-D-1' }, 'flex:1'),
                h('div', { style: testBoxAltStyle + 'flex: 2;', id: 'FL-12-D-2' }, 'flex:2'),
                h('div', { style: testBoxHighlightStyle + 'flex: 3;', id: 'FL-12-D-3' }, 'flex:3')
            )
        )
    );
}

// ========== 测试5: Flexbox Column 布局 ==========
function FlexboxColumnTest() {
    var flexContainer = boxSizing + 'display: flex; flex-direction: column; gap: 10px; padding: 10px; background: #eee;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-flex-column' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '5. Flexbox Column 布局'),

        h('div', { style: testContainerStyle, id: 'FL-COL-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.1 flex-direction: column'),
            h('div', { style: flexContainer + 'height: 300px;', id: 'FL-COL-01-container' },
                h('div', { style: testBoxStyle, id: 'FL-COL-01-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'FL-COL-01-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-COL-01-C' }, 'C')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-COL-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.2 column + justify-content: space-between'),
            h('div', { style: flexContainer + 'height: 300px; justify-content: space-between;', id: 'FL-COL-02-container' },
                h('div', { style: testBoxStyle, id: 'FL-COL-02-A' }, 'Top'),
                h('div', { style: testBoxAltStyle, id: 'FL-COL-02-B' }, 'Middle'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-COL-02-C' }, 'Bottom')
            )
        ),

        h('div', { style: testContainerStyle, id: 'FL-COL-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '5.3 column + flex-grow'),
            h('div', { style: flexContainer + 'height: 250px;', id: 'FL-COL-03-container' },
                h('div', { style: testBoxStyle, id: 'FL-COL-03-A' }, 'Fixed'),
                h('div', { style: testBoxAltStyle + 'flex-grow: 1;', id: 'FL-COL-03-B' }, 'Grow'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-COL-03-C' }, 'Fixed')
            )
        )
    );
}

// ========== 测试6: 尺寸约束 ==========
function SizeConstraintTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-size-constraint' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '6. 尺寸约束'),

        h('div', { style: testContainerStyle, id: 'SC-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.1 min-width: 200px (容器300px)'),
            h('div', { style: 'width: 300px; background: #ddd; padding: 5px;', id: 'SC-01-wrap' },
                h('div', { style: testBoxStyle + 'min-width: 200px;', id: 'SC-01-box' }, 'min-w:200')
            )
        ),

        h('div', { style: testContainerStyle, id: 'SC-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.2 max-width: 150px'),
            h('div', { style: 'width: 300px; background: #ddd; padding: 5px;', id: 'SC-02-wrap' },
                h('div', { style: testBoxStyle + 'max-width: 150px;', id: 'SC-02-box' }, 'max-w:150')
            )
        ),

        h('div', { style: testContainerStyle, id: 'SC-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.3 min-height: 80px'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'SC-03-wrap' },
                h('div', { style: testBoxStyle + 'min-height: 80px;', id: 'SC-03-box' }, 'min-h:80')
            )
        ),

        h('div', { style: testContainerStyle, id: 'SC-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '6.4 max-height: 50px + overflow'),
            h('div', { style: 'background: #ddd; padding: 5px;', id: 'SC-04-wrap' },
                h('div', { style: testBoxStyle + 'max-height: 50px; overflow: hidden;', id: 'SC-04-box' },
                    'max-h:50 This text might be cut off if too long for the container')
            )
        )
    );
}

// ========== 测试7: 嵌套布局 ==========
function NestedLayoutTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-nested' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '7. 嵌套布局'),

        h('div', { style: testContainerStyle, id: 'NL-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.1 Block 内嵌 Flex'),
            h('div', { style: boxSizing + 'padding: 10px; background: #ddd;', id: 'NL-01-outer' },
                h('div', { style: boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #bbb;', id: 'NL-01-flex' },
                    h('div', { style: testBoxStyle + 'flex: 1;', id: 'NL-01-A' }, 'Flex 1'),
                    h('div', { style: testBoxAltStyle + 'flex: 1;', id: 'NL-01-B' }, 'Flex 2')
                )
            )
        ),

        h('div', { style: testContainerStyle, id: 'NL-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.2 Flex 内嵌 Block'),
            h('div', { style: boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #ddd;', id: 'NL-02-flex' },
                h('div', { style: boxSizing + 'flex: 1; background: #bbb; padding: 10px;', id: 'NL-02-left' },
                    h('div', { style: testBoxStyle + 'margin-bottom: 5px;', id: 'NL-02-A' }, 'Block A'),
                    h('div', { style: testBoxAltStyle, id: 'NL-02-B' }, 'Block B')
                ),
                h('div', { style: boxSizing + 'flex: 1; background: #bbb; padding: 10px;', id: 'NL-02-right' },
                    h('div', { style: testBoxHighlightStyle, id: 'NL-02-C' }, 'Block C')
                )
            )
        ),

        h('div', { style: testContainerStyle, id: 'NL-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, '7.3 复杂嵌套'),
            h('div', { style: boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #ddd;', id: 'NL-03-outer' },
                h('div', { style: boxSizing + 'flex: 2; display: flex; flex-direction: column; gap: 5px;', id: 'NL-03-left' },
                    h('div', { style: testBoxStyle, id: 'NL-03-header' }, 'Header'),
                    h('div', { style: boxSizing + 'display: flex; gap: 5px; flex: 1;', id: 'NL-03-middle' },
                        h('div', { style: testBoxAltStyle + 'flex: 1;', id: 'NL-03-side' }, 'Side'),
                        h('div', { style: testBoxHighlightStyle + 'flex: 2;', id: 'NL-03-main' }, 'Main')
                    ),
                    h('div', { style: testBoxStyle, id: 'NL-03-footer' }, 'Footer')
                ),
                h('div', { style: boxSizing + 'flex: 1;', id: 'NL-03-right' },
                    h('div', { style: testBoxAltStyle + 'height: 100%;', id: 'NL-03-panel' }, 'Right Panel')
                )
            )
        )
    );
}

// ========== Flexbox 扩展测试 (FL-13 到 FL-25) ==========
function FlexboxExtendedTest() {
    var flexContainer = boxSizing + 'display: flex; padding: 10px; background: #eee;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-flex-extended' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '8. Flexbox 扩展测试'),

        // FL-13 flex-wrap: wrap
        h('div', { style: testContainerStyle, id: 'FL-13' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-13 flex-wrap: wrap'),
            h('div', { style: flexContainer + 'flex-wrap: wrap; width: 300px; gap: 10px;', id: 'FL-13-container' },
                h('div', { style: testBoxStyle + 'width: 100px;', id: 'FL-13-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 100px;', id: 'FL-13-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 100px;', id: 'FL-13-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle + 'width: 100px;', id: 'FL-13-D' }, 'D')
            )
        ),

        // FL-14 flex-wrap: wrap-reverse
        h('div', { style: testContainerStyle, id: 'FL-14' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-14 flex-wrap: wrap-reverse'),
            h('div', { style: flexContainer + 'flex-wrap: wrap-reverse; width: 300px; gap: 10px;', id: 'FL-14-container' },
                h('div', { style: testBoxStyle + 'width: 100px;', id: 'FL-14-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 100px;', id: 'FL-14-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 100px;', id: 'FL-14-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle + 'width: 100px;', id: 'FL-14-D' }, 'D')
            )
        ),

        // FL-15 gap (行列间距)
        h('div', { style: testContainerStyle, id: 'FL-15' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-15 gap: 20px'),
            h('div', { style: flexContainer + 'gap: 20px; width: 400px;', id: 'FL-15-container' },
                h('div', { style: testBoxStyle + 'width: 80px;', id: 'FL-15-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 80px;', id: 'FL-15-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 80px;', id: 'FL-15-C' }, 'C')
            )
        ),

        // FL-16 row-gap + column-gap
        h('div', { style: testContainerStyle, id: 'FL-16' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-16 row-gap: 10px; column-gap: 30px'),
            h('div', { style: flexContainer + 'flex-wrap: wrap; width: 280px; row-gap: 10px; column-gap: 30px;', id: 'FL-16-container' },
                h('div', { style: testBoxStyle + 'width: 100px;', id: 'FL-16-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 100px;', id: 'FL-16-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 100px;', id: 'FL-16-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle + 'width: 100px;', id: 'FL-16-D' }, 'D')
            )
        ),

        // FL-18 flex-shrink 收缩
        h('div', { style: testContainerStyle, id: 'FL-18' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-18 flex-shrink: 0 (禁止收缩)'),
            h('div', { style: flexContainer + 'width: 300px;', id: 'FL-18-container' },
                h('div', { style: testBoxStyle + 'width: 150px; flex-shrink: 0;', id: 'FL-18-A' }, 'shrink:0'),
                h('div', { style: testBoxAltStyle + 'width: 150px; flex-shrink: 1;', id: 'FL-18-B' }, 'shrink:1'),
                h('div', { style: testBoxHighlightStyle + 'width: 150px; flex-shrink: 1;', id: 'FL-18-C' }, 'shrink:1')
            )
        ),

        // FL-20 flex 简写
        h('div', { style: testContainerStyle, id: 'FL-20' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-20 flex: 1 1 auto'),
            h('div', { style: flexContainer + 'width: 400px;', id: 'FL-20-container' },
                h('div', { style: testBoxStyle + 'flex: 1 1 auto;', id: 'FL-20-A' }, 'flex: 1 1 auto'),
                h('div', { style: testBoxAltStyle + 'flex: 2 1 auto;', id: 'FL-20-B' }, 'flex: 2 1 auto')
            )
        ),

        // FL-21 align-self
        h('div', { style: testContainerStyle, id: 'FL-21' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-21 align-self'),
            h('div', { style: flexContainer + 'height: 120px; align-items: flex-start;', id: 'FL-21-container' },
                h('div', { style: testBoxStyle, id: 'FL-21-A' }, 'start'),
                h('div', { style: testBoxAltStyle + 'align-self: center;', id: 'FL-21-B' }, 'self:center'),
                h('div', { style: testBoxHighlightStyle + 'align-self: flex-end;', id: 'FL-21-C' }, 'self:end')
            )
        ),

        // FL-22 align-content
        h('div', { style: testContainerStyle, id: 'FL-22' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-22 align-content: center'),
            h('div', { style: flexContainer + 'flex-wrap: wrap; width: 250px; height: 150px; align-content: center; gap: 5px;', id: 'FL-22-container' },
                h('div', { style: testBoxStyle + 'width: 100px;', id: 'FL-22-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 100px;', id: 'FL-22-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 100px;', id: 'FL-22-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle + 'width: 100px;', id: 'FL-22-D' }, 'D')
            )
        ),

        // FL-23 order
        h('div', { style: testContainerStyle, id: 'FL-23' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-23 order'),
            h('div', { style: flexContainer + 'gap: 10px;', id: 'FL-23-container' },
                h('div', { style: testBoxStyle + 'order: 2;', id: 'FL-23-A' }, 'order:2'),
                h('div', { style: testBoxAltStyle + 'order: -1;', id: 'FL-23-B' }, 'order:-1'),
                h('div', { style: testBoxHighlightStyle + 'order: 1;', id: 'FL-23-C' }, 'order:1')
            )
        ),

        // FL-24 flex-direction: row-reverse
        h('div', { style: testContainerStyle, id: 'FL-24' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-24 flex-direction: row-reverse'),
            h('div', { style: flexContainer + 'flex-direction: row-reverse; gap: 10px;', id: 'FL-24-container' },
                h('div', { style: testBoxStyle, id: 'FL-24-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'FL-24-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-24-C' }, 'C')
            )
        ),

        // FL-25 flex-direction: column-reverse
        h('div', { style: testContainerStyle, id: 'FL-25' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'FL-25 flex-direction: column-reverse'),
            h('div', { style: flexContainer + 'flex-direction: column-reverse; height: 150px; gap: 10px;', id: 'FL-25-container' },
                h('div', { style: testBoxStyle, id: 'FL-25-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'FL-25-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'FL-25-C' }, 'C')
            )
        )
    );
}

// ========== Grid 布局测试 (GR-01 到 GR-15) ==========
function GridLayoutTest() {
    var gridBase = boxSizing + 'display: grid; background: #ddd; padding: 10px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-grid' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '9. Grid 布局测试'),

        // GR-01 基础 Grid
        h('div', { style: testContainerStyle, id: 'GR-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-01 基础 Grid (1fr 1fr)'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; gap: 10px;', id: 'GR-01-container' },
                h('div', { style: testBoxStyle, id: 'GR-01-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-01-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-01-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle, id: 'GR-01-D' }, 'D')
            )
        ),

        // GR-02 固定列宽
        h('div', { style: testContainerStyle, id: 'GR-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-02 固定列宽 (100px 200px 100px)'),
            h('div', { style: gridBase + 'grid-template-columns: 100px 200px 100px; gap: 10px;', id: 'GR-02-container' },
                h('div', { style: testBoxStyle, id: 'GR-02-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-02-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-02-C' }, 'C')
            )
        ),

        // GR-03 fr 单位混合
        h('div', { style: testContainerStyle, id: 'GR-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-03 fr 单位混合 (100px 1fr 2fr)'),
            h('div', { style: gridBase + 'grid-template-columns: 100px 1fr 2fr; gap: 10px;', id: 'GR-03-container' },
                h('div', { style: testBoxStyle, id: 'GR-03-A' }, '100px'),
                h('div', { style: testBoxAltStyle, id: 'GR-03-B' }, '1fr'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-03-C' }, '2fr')
            )
        ),

        // GR-04 gap
        h('div', { style: testContainerStyle, id: 'GR-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-04 gap: 20px'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr 1fr; gap: 20px;', id: 'GR-04-container' },
                h('div', { style: testBoxStyle, id: 'GR-04-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-04-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-04-C' }, 'C')
            )
        ),

        // GR-05 row-gap + column-gap
        h('div', { style: testContainerStyle, id: 'GR-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-05 row-gap: 10px; column-gap: 30px'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; row-gap: 10px; column-gap: 30px;', id: 'GR-05-container' },
                h('div', { style: testBoxStyle, id: 'GR-05-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-05-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-05-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle, id: 'GR-05-D' }, 'D')
            )
        ),

        // GR-06 repeat()
        h('div', { style: testContainerStyle, id: 'GR-06' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-06 repeat(3, 1fr)'),
            h('div', { style: gridBase + 'grid-template-columns: repeat(3, 1fr); gap: 10px;', id: 'GR-06-container' },
                h('div', { style: testBoxStyle, id: 'GR-06-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-06-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-06-C' }, 'C')
            )
        ),

        // GR-07 minmax()
        h('div', { style: testContainerStyle, id: 'GR-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-07 minmax(100px, 1fr)'),
            h('div', { style: gridBase + 'grid-template-columns: minmax(100px, 1fr) minmax(100px, 1fr); gap: 10px;', id: 'GR-07-container' },
                h('div', { style: testBoxStyle, id: 'GR-07-A' }, 'A'),
                h('div', { style: testBoxAltStyle, id: 'GR-07-B' }, 'B')
            )
        ),

        // GR-10 grid-column span
        h('div', { style: testContainerStyle, id: 'GR-10' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-10 grid-column: span 2'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr 1fr; gap: 10px;', id: 'GR-10-container' },
                h('div', { style: testBoxStyle + 'grid-column: span 2;', id: 'GR-10-A' }, 'span 2'),
                h('div', { style: testBoxAltStyle, id: 'GR-10-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-10-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle, id: 'GR-10-D' }, 'D')
            )
        ),

        // GR-11 grid-row span
        h('div', { style: testContainerStyle, id: 'GR-11' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-11 grid-row: span 2'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; gap: 10px;', id: 'GR-11-container' },
                h('div', { style: testBoxStyle + 'grid-row: span 2;', id: 'GR-11-A' }, 'span 2'),
                h('div', { style: testBoxAltStyle, id: 'GR-11-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'GR-11-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle, id: 'GR-11-D' }, 'D')
            )
        ),

        // GR-14 justify-items
        h('div', { style: testContainerStyle, id: 'GR-14' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-14 justify-items: center'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; gap: 10px; justify-items: center;', id: 'GR-14-container' },
                h('div', { style: testBoxStyle + 'width: 60px;', id: 'GR-14-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 60px;', id: 'GR-14-B' }, 'B')
            )
        ),

        // GR-15 align-items
        h('div', { style: testContainerStyle, id: 'GR-15' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'GR-15 align-items: center'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; gap: 10px; align-items: center; height: 100px;', id: 'GR-15-container' },
                h('div', { style: testBoxStyle, id: 'GR-15-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'height: 60px;', id: 'GR-15-B' }, 'B')
            )
        )
    );
}

// ========== Positioning 定位测试 (PS-01 到 PS-10) ==========
function PositioningTest() {
    var posBase = boxSizing + 'background: #ddd;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-positioning' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '10. Positioning 定位测试'),

        // PS-01 position: relative
        h('div', { style: testContainerStyle, id: 'PS-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-01 position: relative'),
            h('div', { style: posBase + 'padding: 20px;', id: 'PS-01-wrap' },
                h('div', { style: testBoxStyle + 'position: relative; top: 10px; left: 20px;', id: 'PS-01-box' }, 'top:10 left:20')
            )
        ),

        // PS-02 position: absolute (相对父)
        h('div', { style: testContainerStyle, id: 'PS-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-02 position: absolute (相对父)'),
            h('div', { style: posBase + 'position: relative; height: 100px; padding: 10px;', id: 'PS-02-wrap' },
                h('div', { style: testBoxStyle + 'position: absolute; top: 10px; left: 10px;', id: 'PS-02-box' }, 'top:10 left:10')
            )
        ),

        // PS-03 position: absolute (四角)
        h('div', { style: testContainerStyle, id: 'PS-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-03 position: absolute (四角)'),
            h('div', { style: posBase + 'position: relative; height: 120px;', id: 'PS-03-wrap' },
                h('div', { style: testBoxStyle + 'position: absolute; top: 5px; left: 5px; width: 50px;', id: 'PS-03-TL' }, 'TL'),
                h('div', { style: testBoxAltStyle + 'position: absolute; top: 5px; right: 5px; width: 50px;', id: 'PS-03-TR' }, 'TR'),
                h('div', { style: testBoxHighlightStyle + 'position: absolute; bottom: 5px; left: 5px; width: 50px;', id: 'PS-03-BL' }, 'BL'),
                h('div', { style: testBoxPurpleStyle + 'position: absolute; bottom: 5px; right: 5px; width: 50px;', id: 'PS-03-BR' }, 'BR')
            )
        ),

        // PS-07 z-index 层叠
        h('div', { style: testContainerStyle, id: 'PS-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-07 z-index 层叠'),
            h('div', { style: posBase + 'position: relative; height: 100px;', id: 'PS-07-wrap' },
                h('div', { style: testBoxStyle + 'position: absolute; top: 10px; left: 10px; width: 80px; height: 80px; z-index: 1;', id: 'PS-07-z1' }, 'z:1'),
                h('div', { style: testBoxAltStyle + 'position: absolute; top: 30px; left: 30px; width: 80px; height: 80px; z-index: 2;', id: 'PS-07-z2' }, 'z:2'),
                h('div', { style: testBoxHighlightStyle + 'position: absolute; top: 50px; left: 50px; width: 80px; height: 80px; z-index: 3;', id: 'PS-07-z3' }, 'z:3')
            )
        ),

        // PS-08 absolute + 百分比
        h('div', { style: testContainerStyle, id: 'PS-08' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-08 absolute + 百分比 (50% x 50%)'),
            h('div', { style: posBase + 'position: relative; height: 120px;', id: 'PS-08-wrap' },
                h('div', { style: testBoxStyle + 'position: absolute; width: 50%; height: 50%;', id: 'PS-08-box' }, '50%x50%')
            )
        ),

        // PS-05 position: fixed (在容器内模拟，使用 transform 创建新的包含块)
        h('div', { style: testContainerStyle, id: 'PS-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-05 position: fixed (模拟)'),
            h('div', { style: posBase + 'position: relative; height: 120px; transform: translateZ(0);', id: 'PS-05-wrap' },
                h('div', { style: testBoxStyle + 'position: fixed; top: 10px; right: 10px;', id: 'PS-05-box' }, 'fixed')
            )
        ),

        // PS-04 position: absolute + transform 居中
        h('div', { style: testContainerStyle, id: 'PS-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'PS-04 absolute + transform 居中'),
            h('div', { style: posBase + 'position: relative; height: 150px;', id: 'PS-04-wrap' },
                h('div', { style: testBoxStyle + 'position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 100px;', id: 'PS-04-box' }, 'Centered')
            )
        )
    );
}

// ========== Text/IFC 文本布局测试 (TX-03 到 TX-12) ==========
function TextLayoutTest() {
    var textBase = boxSizing + 'background: #ddd; padding: 10px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-text' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, '11. Text/IFC 文本布局测试'),

        // TX-03 text-align: left
        h('div', { style: testContainerStyle, id: 'TX-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-03 text-align: left'),
            h('div', { style: textBase + 'width: 300px; text-align: left;', id: 'TX-03-box' }, 'Left aligned text')
        ),

        // TX-04 text-align: right
        h('div', { style: testContainerStyle, id: 'TX-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-04 text-align: right'),
            h('div', { style: textBase + 'width: 300px; text-align: right;', id: 'TX-04-box' }, 'Right aligned text')
        ),

        // TX-01 text-align: center (已有)
        h('div', { style: testContainerStyle, id: 'TX-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-01 text-align: center'),
            h('div', { style: textBase + 'width: 300px; text-align: center;', id: 'TX-01-box' }, 'Center aligned text')
        ),

        // TX-05 text-align: justify (多行)
        h('div', { style: testContainerStyle, id: 'TX-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-05 text-align: justify'),
            h('div', { style: textBase + 'width: 300px; text-align: justify;', id: 'TX-05-box' },
                'This is a long paragraph of text that should be justified. The text will be spread out evenly across each line, with extra space distributed between words. The last line should remain left-aligned.')
        ),

        // TX-06 line-height 数值
        h('div', { style: testContainerStyle, id: 'TX-06' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-06 line-height: 2'),
            h('div', { style: textBase + 'line-height: 2;', id: 'TX-06-box' },
                'Line 1 with line-height: 2', h('br'), 'Line 2', h('br'), 'Line 3')
        ),

        // TX-07 line-height 像素
        h('div', { style: testContainerStyle, id: 'TX-07' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-07 line-height: 30px'),
            h('div', { style: textBase + 'line-height: 30px;', id: 'TX-07-box' },
                'Line 1 with line-height: 30px', h('br'), 'Line 2', h('br'), 'Line 3')
        ),

        // TX-08 多行文本换行
        h('div', { style: testContainerStyle, id: 'TX-08' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-08 多行文本换行'),
            h('div', { style: textBase + 'width: 200px;', id: 'TX-08-box' },
                'This is a long text that should automatically wrap to the next line when it reaches the container width boundary.')
        ),

        // TX-09 white-space: nowrap
        h('div', { style: testContainerStyle, id: 'TX-09' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-09 white-space: nowrap'),
            h('div', { style: textBase + 'width: 200px; white-space: nowrap; overflow: hidden;', id: 'TX-09-box' },
                'This text should not wrap and will be clipped')
        ),

        // TX-10 white-space: pre
        h('div', { style: testContainerStyle, id: 'TX-10' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-10 white-space: pre'),
            h('div', { style: textBase + 'white-space: pre;', id: 'TX-10-box' },
                'Line 1\n  Line 2 (indented)\n    Line 3 (more indent)')
        ),

        // TX-11 text-overflow: ellipsis
        h('div', { style: testContainerStyle, id: 'TX-11' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-11 text-overflow: ellipsis'),
            h('div', { style: textBase + 'width: 200px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;', id: 'TX-11-box' },
                'This long text should show ellipsis at the end when it overflows')
        ),

        // TX-12 vertical-align
        h('div', { style: testContainerStyle, id: 'TX-12' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'TX-12 vertical-align'),
            h('div', { style: textBase + 'line-height: 60px;', id: 'TX-12-box' },
                'Text ',
                h('span', { style: boxSizing + 'display: inline-block; width: 30px; height: 30px; background: #4CAF50; vertical-align: top;', id: 'TX-12-top' }),
                ' top ',
                h('span', { style: boxSizing + 'display: inline-block; width: 30px; height: 30px; background: #2196F3; vertical-align: middle;', id: 'TX-12-middle' }),
                ' middle ',
                h('span', { style: boxSizing + 'display: inline-block; width: 30px; height: 30px; background: #FF9800; vertical-align: bottom;', id: 'TX-12-bottom' }),
                ' bottom'
            )
        )
    );
}

// ========== 主应用 ==========
function App() {
    return h('div', { style: 'padding: 20px; font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto;' },
        h('h1', { style: 'margin: 0 0 20px 0; color: #333;' }, '🔬 Layout Compare Test'),
        h('p', { style: 'margin: 0 0 20px 0; color: #666;' },
            '完整布局测试 - Box Model / Block / Flexbox / Grid / Positioning / Text'),

        h(BlockBasicTest),
        h(BlockLayoutTest),
        h(FlexboxRowTest),
        h(FlexboxAlignTest),
        h(FlexGrowShrinkTest),
        h(FlexboxColumnTest),
        h(FlexboxExtendedTest),
        h(SizeConstraintTest),
        h(NestedLayoutTest),
        h(GridLayoutTest),
        h(PositioningTest),
        h(TextLayoutTest),

        h('footer', { style: 'margin-top: 30px; padding: 20px; text-align: center; background: #f5f5f5; border-radius: 4px;' },
            h('p', { style: 'margin: 0; color: #666;' }, '✅ Layout Compare Test Complete - Powered by MBink')
        )
    );
}

// 渲染
render(h(App), document.body);

