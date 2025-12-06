/**
 * 高级布局渲染对比测试 - 复杂场景和边界情况
 * 用于验证原生布局引擎在高级场景下与浏览器的一致性
 *
 * 测试类别:
 * - ADV-1: 深度嵌套组合
 * - ADV-2: 极限尺寸约束
 * - ADV-3: 复杂对齐场景
 * - ADV-4: 动态内容溢出
 * - ADV-5: 高级 Grid 功能
 * - ADV-6: Flexbox 边界情况
 * - ADV-7: 实用布局模式
 */

var h = Preact.h;
var render = Preact.render;

// ========== 测试容器样式 ==========
var boxSizing = 'box-sizing: border-box; ';
var testContainerStyle = boxSizing + 'margin: 10px 0; padding: 10px; border: 1px solid #ddd; background: #f9f9f9;';
var testBoxStyle = boxSizing + 'background: #4CAF50; color: white; padding: 10px; text-align: center;';
var testBoxAltStyle = boxSizing + 'background: #2196F3; color: white; padding: 10px; text-align: center;';
var testBoxHighlightStyle = boxSizing + 'background: #FF9800; color: white; padding: 10px; text-align: center;';
var testBoxPurpleStyle = boxSizing + 'background: #9C27B0; color: white; padding: 10px; text-align: center;';
var testBoxGrayStyle = boxSizing + 'background: #607D8B; color: white; padding: 10px; text-align: center;';

// ========== ADV-1: 深度嵌套组合测试 ==========
function DeepNestingTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-adv1' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-1: 深度嵌套组合'),

        // ADV-1-01: Flex 内嵌 Grid 内嵌 Flex
        h('div', { style: testContainerStyle, id: 'ADV-1-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-1-01 Flex > Grid > Flex'),
            h('div', { style: boxSizing + 'display: flex; gap: 10px; padding: 10px; background: #eee;', id: 'ADV-1-01-L1' },
                h('div', { style: boxSizing + 'flex: 1; display: grid; grid-template-columns: 1fr 1fr; gap: 5px; background: #ddd; padding: 5px;', id: 'ADV-1-01-L2' },
                    h('div', { style: boxSizing + 'display: flex; justify-content: center; align-items: center;' + testBoxStyle, id: 'ADV-1-01-A' }, 'A'),
                    h('div', { style: boxSizing + 'display: flex; justify-content: center; align-items: center;' + testBoxAltStyle, id: 'ADV-1-01-B' }, 'B')
                ),
                h('div', { style: testBoxHighlightStyle + 'flex: 1;', id: 'ADV-1-01-C' }, 'C')
            )
        ),

        // ADV-1-02: Grid 内嵌 Flex (stretch vs center)
        h('div', { style: testContainerStyle, id: 'ADV-1-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-1-02 Grid(stretch) > Flex(center)'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: 150px 1fr; height: 150px; align-items: stretch; gap: 10px; background: #eee; padding: 10px;', id: 'ADV-1-02-grid' },
                h('div', { style: boxSizing + 'display: flex; align-items: center; background: #ddd;', id: 'ADV-1-02-left' },
                    h('div', { style: testBoxStyle, id: 'ADV-1-02-A' }, '垂直居中')
                ),
                h('div', { style: boxSizing + 'display: flex; justify-content: flex-end; align-items: flex-start; background: #ccc;', id: 'ADV-1-02-right' },
                    h('div', { style: testBoxAltStyle, id: 'ADV-1-02-B' }, '右上角')
                )
            )
        ),

        // ADV-1-03: 5层嵌套布局
        h('div', { style: testContainerStyle, id: 'ADV-1-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-1-03 五层嵌套 (Flex>Grid>Flex>Grid>Flex)'),
            h('div', { style: boxSizing + 'display: flex; background: #e0e0e0; padding: 10px;', id: 'ADV-1-03-L1' },
                h('div', { style: boxSizing + 'flex: 1; display: grid; grid-template-columns: 1fr; background: #d0d0d0; padding: 8px;', id: 'ADV-1-03-L2' },
                    h('div', { style: boxSizing + 'display: flex; flex-direction: column; background: #c0c0c0; padding: 6px;', id: 'ADV-1-03-L3' },
                        h('div', { style: boxSizing + 'display: grid; grid-template-columns: 1fr 1fr; gap: 5px; background: #b0b0b0; padding: 4px;', id: 'ADV-1-03-L4' },
                            h('div', { style: boxSizing + 'display: flex; justify-content: center;' + testBoxStyle, id: 'ADV-1-03-A' }, 'L5-A'),
                            h('div', { style: boxSizing + 'display: flex; justify-content: center;' + testBoxAltStyle, id: 'ADV-1-03-B' }, 'L5-B')
                        )
                    )
                )
            )
        )
    );
}

// ========== ADV-2: 极限尺寸约束测试 ==========
function SizeConstraintAdvancedTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-adv2' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-2: 极限尺寸约束'),

        // ADV-2-01: min-width > max-width 冲突
        h('div', { style: testContainerStyle, id: 'ADV-2-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-2-01 min-width > max-width (200px > 100px)'),
            h('div', { style: boxSizing + 'width: 300px; background: #ddd; padding: 5px;', id: 'ADV-2-01-wrap' },
                h('div', { style: testBoxStyle + 'min-width: 200px; max-width: 100px;', id: 'ADV-2-01-box' }, 'min优先=200px')
            )
        ),

        // ADV-2-02: 百分比 + min/max 组合
        h('div', { style: testContainerStyle, id: 'ADV-2-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-2-02 width:50% + min:200px'),
            h('div', { style: boxSizing + 'width: 300px; background: #ddd; padding: 5px;', id: 'ADV-2-02-wrap' },
                h('div', { style: testBoxStyle + 'width: 50%; min-width: 200px;', id: 'ADV-2-02-box' }, '150→200')
            )
        ),

        // ADV-2-03: Flex 子项 flex-basis vs min-width
        h('div', { style: testContainerStyle, id: 'ADV-2-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-2-03 flex-basis:100px + min-width:150px'),
            h('div', { style: boxSizing + 'display: flex; width: 400px; background: #ddd; padding: 5px;', id: 'ADV-2-03-flex' },
                h('div', { style: testBoxStyle + 'flex: 1 1 100px; min-width: 150px;', id: 'ADV-2-03-A' }, 'min:150'),
                h('div', { style: testBoxAltStyle + 'flex: 1 1 100px;', id: 'ADV-2-03-B' }, 'B')
            )
        ),

        // ADV-2-04: 嵌套百分比高度
        h('div', { style: testContainerStyle, id: 'ADV-2-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-2-04 嵌套百分比高度 (50%→25%→12.5%)'),
            h('div', { style: boxSizing + 'height: 200px; background: #e0e0e0; padding: 5px;', id: 'ADV-2-04-L0' },
                h('div', { style: boxSizing + 'height: 50%; background: #c0c0c0;', id: 'ADV-2-04-L1' },
                    h('div', { style: boxSizing + 'height: 50%; background: #a0a0a0;', id: 'ADV-2-04-L2' },
                        h('div', { style: testBoxStyle + 'height: 50%;', id: 'ADV-2-04-box' }, '12.5%')
                    )
                )
            )
        ),

        // ADV-2-05: width:auto + min/max
        h('div', { style: testContainerStyle, id: 'ADV-2-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-2-05 width:auto + max-width:200px'),
            h('div', { style: boxSizing + 'width: 400px; background: #ddd; padding: 5px;', id: 'ADV-2-05-wrap' },
                h('div', { style: testBoxStyle + 'max-width: 200px;', id: 'ADV-2-05-box' }, '被max限制')
            )
        )
    );
}

// ========== ADV-3: 复杂对齐场景测试 ==========
function ComplexAlignmentTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-adv3' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-3: 复杂对齐场景'),

        // ADV-3-01: margin:auto 在 Flex 中完全居中
        h('div', { style: testContainerStyle, id: 'ADV-3-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-3-01 Flex + margin:auto 完全居中'),
            h('div', { style: boxSizing + 'display: flex; width: 300px; height: 150px; background: #ddd;', id: 'ADV-3-01-flex' },
                h('div', { style: testBoxStyle + 'margin: auto; width: 80px; height: 40px;', id: 'ADV-3-01-box' }, '居中')
            )
        ),

        // ADV-3-02: 部分 margin:auto 推动
        h('div', { style: testContainerStyle, id: 'ADV-3-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-3-02 margin-left:auto 推动'),
            h('div', { style: boxSizing + 'display: flex; width: 350px; background: #ddd; padding: 5px;', id: 'ADV-3-02-flex' },
                h('div', { style: testBoxStyle + 'width: 50px;', id: 'ADV-3-02-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'margin-left: auto; width: 50px;', id: 'ADV-3-02-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'width: 50px;', id: 'ADV-3-02-C' }, 'C')
            )
        ),

        // ADV-3-03: Grid + place-items: center
        h('div', { style: testContainerStyle, id: 'ADV-3-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-3-03 Grid + place-items:center'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: 1fr 1fr; height: 120px; place-items: center; background: #ddd; gap: 10px;', id: 'ADV-3-03-grid' },
                h('div', { style: testBoxStyle + 'width: 50px; height: 40px;', id: 'ADV-3-03-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'width: 50px; height: 40px;', id: 'ADV-3-03-B' }, 'B')
            )
        ),

        // ADV-3-04: justify-self / align-self 覆盖
        h('div', { style: testContainerStyle, id: 'ADV-3-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-3-04 Grid self覆盖容器对齐'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: 1fr 1fr; height: 120px; place-items: start; background: #ddd; gap: 10px;', id: 'ADV-3-04-grid' },
                h('div', { style: testBoxStyle + 'justify-self: end; align-self: end; width: 50px;', id: 'ADV-3-04-A' }, '右下'),
                h('div', { style: testBoxAltStyle + 'width: 50px;', id: 'ADV-3-04-B' }, '左上')
            )
        ),

        // ADV-3-05: Flex align-items:baseline + 不同高度
        h('div', { style: testContainerStyle, id: 'ADV-3-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-3-05 Flex baseline对齐'),
            h('div', { style: boxSizing + 'display: flex; align-items: baseline; gap: 10px; background: #ddd; padding: 10px;', id: 'ADV-3-05-flex' },
                h('div', { style: testBoxStyle + 'font-size: 12px;', id: 'ADV-3-05-A' }, '小'),
                h('div', { style: testBoxAltStyle + 'font-size: 24px;', id: 'ADV-3-05-B' }, '大'),
                h('div', { style: testBoxHighlightStyle + 'font-size: 16px; padding-top: 15px;', id: 'ADV-3-05-C' }, '有padding')
            )
        )
    );
}

// ========== ADV-4: 动态内容溢出测试 ==========
function OverflowAdvancedTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-adv4' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-4: 动态内容溢出'),

        // ADV-4-01: Flex nowrap 溢出
        h('div', { style: testContainerStyle, id: 'ADV-4-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-4-01 Flex nowrap 溢出 (300px in 200px)'),
            h('div', { style: boxSizing + 'display: flex; width: 200px; flex-wrap: nowrap; background: #ddd; overflow: visible;', id: 'ADV-4-01-flex' },
                h('div', { style: testBoxStyle + 'flex: 0 0 100px;', id: 'ADV-4-01-A' }, 'A'),
                h('div', { style: testBoxAltStyle + 'flex: 0 0 100px;', id: 'ADV-4-01-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'flex: 0 0 100px;', id: 'ADV-4-01-C' }, 'C')
            )
        ),

        // ADV-4-02: Grid 内容溢出单元格
        h('div', { style: testContainerStyle, id: 'ADV-4-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-4-02 Grid 子项超出单元格'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: 100px 100px; gap: 10px; background: #ddd; overflow: visible;', id: 'ADV-4-02-grid' },
                h('div', { style: testBoxStyle + 'width: 150px;', id: 'ADV-4-02-A' }, '150px溢出'),
                h('div', { style: testBoxAltStyle, id: 'ADV-4-02-B' }, '正常')
            )
        ),

        // ADV-4-03: 文本溢出 + flex-shrink
        h('div', { style: testContainerStyle, id: 'ADV-4-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-4-03 Flex + text-overflow:ellipsis'),
            h('div', { style: boxSizing + 'display: flex; width: 250px; background: #ddd;', id: 'ADV-4-03-flex' },
                h('div', { style: testBoxStyle + 'flex: 1 1 auto; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;', id: 'ADV-4-03-A' }, 'Very long text that should be truncated'),
                h('div', { style: testBoxAltStyle + 'flex: 0 0 60px;', id: 'ADV-4-03-B' }, 'Fixed')
            )
        ),

        // ADV-4-04: min-width:0 允许收缩
        h('div', { style: testContainerStyle, id: 'ADV-4-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-4-04 Flex min-width:0 允许收缩'),
            h('div', { style: boxSizing + 'display: flex; width: 200px; background: #ddd;', id: 'ADV-4-04-flex' },
                h('div', { style: testBoxStyle + 'flex: 1 1 auto; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;', id: 'ADV-4-04-A' }, 'This text can shrink below content size'),
                h('div', { style: testBoxAltStyle + 'flex: 0 0 80px;', id: 'ADV-4-04-B' }, 'Fixed')
            )
        )
    );
}

// ========== ADV-5: 高级 Grid 功能测试 ==========
function GridAdvancedTest() {
    var gridBase = boxSizing + 'display: grid; background: #ddd; padding: 10px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-adv5' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-5: 高级 Grid 功能'),

        // ADV-5-01: grid-template-rows 明确行高
        h('div', { style: testContainerStyle, id: 'ADV-5-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-5-01 grid-template-rows: 50px 80px 50px'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; grid-template-rows: 50px 80px 50px; gap: 5px;', id: 'ADV-5-01-grid' },
                h('div', { style: testBoxStyle, id: 'ADV-5-01-A' }, 'R1C1'),
                h('div', { style: testBoxAltStyle, id: 'ADV-5-01-B' }, 'R1C2'),
                h('div', { style: testBoxHighlightStyle, id: 'ADV-5-01-C' }, 'R2C1'),
                h('div', { style: testBoxPurpleStyle, id: 'ADV-5-01-D' }, 'R2C2'),
                h('div', { style: testBoxGrayStyle, id: 'ADV-5-01-E' }, 'R3C1'),
                h('div', { style: testBoxStyle, id: 'ADV-5-01-F' }, 'R3C2')
            )
        ),

        // ADV-5-02: grid-auto-rows
        h('div', { style: testContainerStyle, id: 'ADV-5-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-5-02 grid-auto-rows: 60px'),
            h('div', { style: gridBase + 'grid-template-columns: 1fr 1fr; grid-auto-rows: 60px; gap: 5px;', id: 'ADV-5-02-grid' },
                h('div', { style: testBoxStyle, id: 'ADV-5-02-A' }, '1'),
                h('div', { style: testBoxAltStyle, id: 'ADV-5-02-B' }, '2'),
                h('div', { style: testBoxHighlightStyle, id: 'ADV-5-02-C' }, '3'),
                h('div', { style: testBoxPurpleStyle, id: 'ADV-5-02-D' }, '4'),
                h('div', { style: testBoxGrayStyle, id: 'ADV-5-02-E' }, '5'),
                h('div', { style: testBoxStyle, id: 'ADV-5-02-F' }, '6')
            )
        ),

        // ADV-5-03: grid-column 指定位置
        h('div', { style: testContainerStyle, id: 'ADV-5-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-5-03 grid-column: 2/4 和 1/-1'),
            h('div', { style: gridBase + 'grid-template-columns: repeat(4, 1fr); gap: 5px;', id: 'ADV-5-03-grid' },
                h('div', { style: testBoxStyle + 'grid-column: 2 / 4;', id: 'ADV-5-03-A' }, '2-3列'),
                h('div', { style: testBoxAltStyle, id: 'ADV-5-03-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle + 'grid-column: 1 / -1;', id: 'ADV-5-03-C' }, '全宽')
            )
        ),

        // ADV-5-04: row + column span 组合
        h('div', { style: testContainerStyle, id: 'ADV-5-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-5-04 grid-column:span 2 + grid-row:span 2'),
            h('div', { style: gridBase + 'grid-template-columns: repeat(3, 1fr); gap: 5px;', id: 'ADV-5-04-grid' },
                h('div', { style: testBoxStyle + 'grid-column: span 2; grid-row: span 2;', id: 'ADV-5-04-A' }, '2x2'),
                h('div', { style: testBoxAltStyle, id: 'ADV-5-04-B' }, 'B'),
                h('div', { style: testBoxHighlightStyle, id: 'ADV-5-04-C' }, 'C'),
                h('div', { style: testBoxPurpleStyle, id: 'ADV-5-04-D' }, 'D'),
                h('div', { style: testBoxGrayStyle, id: 'ADV-5-04-E' }, 'E')
            )
        )
    );
}

// ========== ADV-6: Flexbox 边界情况测试 ==========
function FlexboxEdgeCaseTest() {
    var flexBase = boxSizing + 'display: flex; background: #ddd; padding: 10px;';

    return h('section', { style: 'margin: 20px 0;', id: 'section-adv6' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-6: Flexbox 边界情况'),

        // ADV-6-01: 负 margin 在 Flex 中
        h('div', { style: testContainerStyle, id: 'ADV-6-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-6-01 负margin减少间距'),
            h('div', { style: flexBase + 'gap: 20px;', id: 'ADV-6-01-flex' },
                h('div', { style: testBoxStyle + 'margin-right: -10px;', id: 'ADV-6-01-A' }, 'A(-10)'),
                h('div', { style: testBoxAltStyle, id: 'ADV-6-01-B' }, 'B')
            )
        ),

        // ADV-6-02: flex-basis: 0 vs auto
        h('div', { style: testContainerStyle, id: 'ADV-6-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-6-02 flex-basis:0 vs auto'),
            h('div', { style: flexBase + 'width: 300px;', id: 'ADV-6-02-flex' },
                h('div', { style: testBoxStyle + 'flex: 1 1 0;', id: 'ADV-6-02-A' }, 'basis:0'),
                h('div', { style: testBoxAltStyle + 'flex: 1 1 auto;', id: 'ADV-6-02-B' }, 'basis:auto有长内容')
            )
        ),

        // ADV-6-03: flex-shrink 嵌套
        h('div', { style: testContainerStyle, id: 'ADV-6-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-6-03 嵌套Flex收缩'),
            h('div', { style: flexBase + 'width: 200px;', id: 'ADV-6-03-outer' },
                h('div', { style: testBoxStyle + 'flex: 0 0 100px;', id: 'ADV-6-03-A' }, '不缩'),
                h('div', { style: boxSizing + 'flex: 1 1 auto; display: flex; background: #bbb;', id: 'ADV-6-03-inner' },
                    h('div', { style: testBoxAltStyle + 'flex: 1 1 auto;', id: 'ADV-6-03-B' }, '内部收缩')
                )
            )
        ),

        // ADV-6-04: flex 0 0 auto vs 0 1 auto
        h('div', { style: testContainerStyle, id: 'ADV-6-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-6-04 flex:0 0 auto vs 0 1 auto'),
            h('div', { style: flexBase + 'width: 200px;', id: 'ADV-6-04-flex' },
                h('div', { style: testBoxStyle + 'flex: 0 0 auto;', id: 'ADV-6-04-A' }, '不收缩也不增长'),
                h('div', { style: testBoxAltStyle + 'flex: 0 1 auto;', id: 'ADV-6-04-B' }, '只收缩Content')
            )
        )
    );
}

// ========== ADV-7: 实用布局模式测试 ==========
function PracticalLayoutTest() {
    return h('section', { style: 'margin: 20px 0;', id: 'section-adv7' },
        h('h2', { style: 'margin: 0 0 10px 0;' }, 'ADV-7: 实用布局模式'),

        // ADV-7-01: 粘性页脚 (Sticky Footer)
        h('div', { style: testContainerStyle, id: 'ADV-7-01' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-01 粘性页脚 (Sticky Footer)'),
            h('div', { style: boxSizing + 'display: flex; flex-direction: column; min-height: 250px; background: #eee;', id: 'ADV-7-01-container' },
                h('header', { style: testBoxStyle + 'height: 40px;', id: 'ADV-7-01-header' }, 'Header'),
                h('main', { style: testBoxAltStyle + 'flex: 1;', id: 'ADV-7-01-main' }, 'Main Content'),
                h('footer', { style: testBoxHighlightStyle + 'height: 30px;', id: 'ADV-7-01-footer' }, 'Footer')
            )
        ),

        // ADV-7-02: 圣杯布局 (Holy Grail)
        h('div', { style: testContainerStyle, id: 'ADV-7-02' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-02 圣杯布局 (Grid)'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: 100px 1fr 100px; grid-template-rows: auto 1fr auto; min-height: 200px; gap: 5px; background: #eee;', id: 'ADV-7-02-container' },
                h('header', { style: testBoxStyle + 'grid-column: 1 / -1;', id: 'ADV-7-02-header' }, 'Header'),
                h('aside', { style: testBoxPurpleStyle, id: 'ADV-7-02-left' }, 'Left'),
                h('main', { style: testBoxAltStyle, id: 'ADV-7-02-main' }, 'Main'),
                h('aside', { style: testBoxPurpleStyle, id: 'ADV-7-02-right' }, 'Right'),
                h('footer', { style: testBoxHighlightStyle + 'grid-column: 1 / -1;', id: 'ADV-7-02-footer' }, 'Footer')
            )
        ),

        // ADV-7-03: 等高列 (Equal Height Columns)
        h('div', { style: testContainerStyle, id: 'ADV-7-03' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-03 等高列 (Flex)'),
            h('div', { style: boxSizing + 'display: flex; gap: 10px; background: #eee; padding: 10px;', id: 'ADV-7-03-container' },
                h('div', { style: testBoxStyle + 'flex: 1;', id: 'ADV-7-03-A' }, '短'),
                h('div', { style: testBoxAltStyle + 'flex: 1;', id: 'ADV-7-03-B' }, '这是比较长的内容，会让列变高，其他列也会跟着变高'),
                h('div', { style: testBoxHighlightStyle + 'flex: 1;', id: 'ADV-7-03-C' }, '中等内容')
            )
        ),

        // ADV-7-04: 卡片网格 (Card Grid)
        h('div', { style: testContainerStyle, id: 'ADV-7-04' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-04 卡片网格 (Grid + Flex)'),
            h('div', { style: boxSizing + 'display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; background: #eee; padding: 10px;', id: 'ADV-7-04-container' },
                h('div', { style: boxSizing + 'display: flex; flex-direction: column; background: #fff; border: 1px solid #ddd;', id: 'ADV-7-04-card1' },
                    h('div', { style: testBoxStyle + 'flex: 1;', id: 'ADV-7-04-card1-content' }, '卡片1内容'),
                    h('div', { style: testBoxAltStyle, id: 'ADV-7-04-card1-btn' }, '按钮')
                ),
                h('div', { style: boxSizing + 'display: flex; flex-direction: column; background: #fff; border: 1px solid #ddd;', id: 'ADV-7-04-card2' },
                    h('div', { style: testBoxStyle + 'flex: 1;', id: 'ADV-7-04-card2-content' }, '卡片2有更多内容会更高'),
                    h('div', { style: testBoxAltStyle, id: 'ADV-7-04-card2-btn' }, '按钮')
                ),
                h('div', { style: boxSizing + 'display: flex; flex-direction: column; background: #fff; border: 1px solid #ddd;', id: 'ADV-7-04-card3' },
                    h('div', { style: testBoxStyle + 'flex: 1;', id: 'ADV-7-04-card3-content' }, '卡片3'),
                    h('div', { style: testBoxAltStyle, id: 'ADV-7-04-card3-btn' }, '按钮')
                )
            )
        ),

        // ADV-7-05: 媒体对象 (Media Object)
        h('div', { style: testContainerStyle, id: 'ADV-7-05' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-05 媒体对象 (Flex)'),
            h('div', { style: boxSizing + 'display: flex; gap: 15px; background: #eee; padding: 10px;', id: 'ADV-7-05-container' },
                h('div', { style: testBoxStyle + 'flex: 0 0 80px; height: 80px;', id: 'ADV-7-05-avatar' }, 'Avatar'),
                h('div', { style: boxSizing + 'flex: 1; display: flex; flex-direction: column; gap: 5px;', id: 'ADV-7-05-content' },
                    h('div', { style: testBoxAltStyle, id: 'ADV-7-05-title' }, '标题'),
                    h('div', { style: testBoxHighlightStyle + 'flex: 1;', id: 'ADV-7-05-body' }, '内容正文可以很长')
                )
            )
        ),

        // ADV-7-06: 双飞翼布局 (Flex 实现)
        h('div', { style: testContainerStyle, id: 'ADV-7-06' },
            h('h3', { style: 'margin: 0 0 5px 0; font-size: 14px;' }, 'ADV-7-06 双飞翼布局 (Flex)'),
            h('div', { style: boxSizing + 'display: flex; gap: 10px; background: #eee; padding: 10px;', id: 'ADV-7-06-container' },
                h('aside', { style: testBoxPurpleStyle + 'flex: 0 0 100px; order: -1;', id: 'ADV-7-06-left' }, 'Left'),
                h('main', { style: testBoxAltStyle + 'flex: 1;', id: 'ADV-7-06-main' }, 'Main (order: 0)'),
                h('aside', { style: testBoxPurpleStyle + 'flex: 0 0 100px;', id: 'ADV-7-06-right' }, 'Right')
            )
        )
    );
}

// ========== 主应用组件 ==========
function AdvancedApp() {
    return h('div', { style: 'padding: 20px; font-family: Arial, sans-serif;' },
        h('h1', { style: 'margin: 0 0 20px 0; color: #333;' }, '高级布局对比测试'),
        h('p', { style: 'margin: 0 0 20px 0; color: #666;' },
            '测试复杂布局场景和边界情况。共 7 个类别，约 30+ 个测试用例。'
        ),

        h(DeepNestingTest),
        h(SizeConstraintAdvancedTest),
        h(ComplexAlignmentTest),
        h(OverflowAdvancedTest),
        h(GridAdvancedTest),
        h(FlexboxEdgeCaseTest),
        h(PracticalLayoutTest),

        h('footer', { style: 'margin-top: 30px; padding: 20px; text-align: center; background: #f5f5f5; border-radius: 4px;' },
            h('p', { style: 'margin: 0; color: #666;' }, '✅ Advanced Layout Compare Test Complete - Powered by MBink')
        )
    );
}

// 渲染应用 - 直接渲染（MBink 模式或浏览器独立页面）
render(h(AdvancedApp), document.body);

// 导出组件供外部使用
if (typeof window !== 'undefined') {
    window.AdvancedLayoutTests = {
        DeepNestingTest: DeepNestingTest,
        SizeConstraintAdvancedTest: SizeConstraintAdvancedTest,
        ComplexAlignmentTest: ComplexAlignmentTest,
        OverflowAdvancedTest: OverflowAdvancedTest,
        GridAdvancedTest: GridAdvancedTest,
        FlexboxEdgeCaseTest: FlexboxEdgeCaseTest,
        PracticalLayoutTest: PracticalLayoutTest,
        AdvancedApp: AdvancedApp
    };
}

