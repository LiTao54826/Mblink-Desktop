/**
 * @file test_absolute_positioning.js
 * @brief 绝对定位 (bottom/right) 测试用例
 * 
 * 此测试用于验证绝对定位的 bottom 和 right 属性是否正确工作。
 * 预期行为：
 * - bottom: 0; right: 0 - 元素应该在容器右下角
 * - bottom: 0 - 元素应该在容器底部
 * - right: 0 - 元素应该在容器右侧
 * - top + bottom - 元素应该垂直拉伸
 * - left + right - 元素应该水平拉伸
 * 
 * 需求: 4.1, 4.2
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    // 容器样式
    var containerStyle = {
        position: 'relative',
        width: '400px',
        height: '300px',
        backgroundColor: '#f0f0f0',
        border: '2px solid #333',
        margin: '20px'
    };

    // 标签样式
    var labelStyle = {
        fontSize: '12px',
        color: 'white',
        padding: '5px'
    };

    function TestContainer(props) {
        return h('div', { 
            style: Object.assign({}, containerStyle, { 
                position: 'absolute', 
                top: props.top, 
                left: props.left 
            }) 
        },
            h('div', { 
                style: { 
                    position: 'absolute', 
                    top: '5px', 
                    left: '5px', 
                    fontSize: '14px', 
                    fontWeight: 'bold' 
                } 
            }, props.title),
            props.children
        );
    }

    function App() {
        return h('div', { 
            style: { 
                padding: '0',
                margin: '0',
                backgroundColor: '#e0e0e0',
                minHeight: '100vh'
            } 
        },
            // Test 1: bottom: 0; right: 0
            h(TestContainer, { top: '20px', left: '20px', title: 'Test 1: bottom: 0; right: 0' },
                h('div', { 
                    id: 'test-bottom-right',
                    style: { 
                        position: 'absolute',
                        bottom: '0',
                        right: '0',
                        width: '80px',
                        height: '60px',
                        backgroundColor: 'rgba(255, 0, 0, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'bottom:0\nright:0'))
            ),
            
            // Test 2: bottom: 0 only
            h(TestContainer, { top: '20px', left: '460px', title: 'Test 2: bottom: 0 only' },
                h('div', { 
                    id: 'test-bottom-only',
                    style: { 
                        position: 'absolute',
                        bottom: '0',
                        left: '50px',
                        width: '80px',
                        height: '60px',
                        backgroundColor: 'rgba(0, 255, 0, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'bottom:0'))
            ),
            
            // Test 3: right: 0 only
            h(TestContainer, { top: '350px', left: '20px', title: 'Test 3: right: 0 only' },
                h('div', { 
                    id: 'test-right-only',
                    style: { 
                        position: 'absolute',
                        right: '0',
                        top: '50px',
                        width: '80px',
                        height: '60px',
                        backgroundColor: 'rgba(0, 0, 255, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'right:0'))
            ),
            
            // Test 4: top + bottom stretch
            h(TestContainer, { top: '350px', left: '460px', title: 'Test 4: top: 40px; bottom: 20px (stretch)' },
                h('div', { 
                    id: 'test-top-bottom-stretch',
                    style: { 
                        position: 'absolute',
                        top: '40px',
                        bottom: '20px',
                        left: '50px',
                        width: '80px',
                        backgroundColor: 'rgba(255, 165, 0, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'top+bottom\nstretch'))
            ),
            
            // Test 5: left + right stretch
            h(TestContainer, { top: '680px', left: '20px', title: 'Test 5: left: 20px; right: 20px (stretch)' },
                h('div', { 
                    id: 'test-left-right-stretch',
                    style: { 
                        position: 'absolute',
                        left: '20px',
                        right: '20px',
                        top: '50px',
                        height: '60px',
                        backgroundColor: 'rgba(128, 0, 128, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'left+right stretch'))
            ),
            
            // Test 6: percentage inset
            h(TestContainer, { top: '680px', left: '460px', title: 'Test 6: bottom: 10%; right: 10%' },
                h('div', { 
                    id: 'test-percent-inset',
                    style: { 
                        position: 'absolute',
                        bottom: '10%',
                        right: '10%',
                        width: '80px',
                        height: '60px',
                        backgroundColor: 'rgba(0, 128, 128, 0.7)'
                    } 
                }, h('span', { style: labelStyle }, 'bottom:10%\nright:10%'))
            ),
            
            // 信息面板
            h('div', { 
                style: { 
                    position: 'absolute',
                    top: '10px',
                    right: '10px',
                    padding: '15px',
                    backgroundColor: 'white',
                    border: '1px solid #ccc',
                    borderRadius: '5px',
                    fontSize: '14px',
                    maxWidth: '300px'
                } 
            }, 
                h('h3', { style: { margin: '0 0 10px 0' } }, '绝对定位测试'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 1:'), ' bottom:0; right:0 - 右下角'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 2:'), ' bottom:0 - 底部'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 3:'), ' right:0 - 右侧'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 4:'), ' top+bottom - 垂直拉伸'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 5:'), ' left+right - 水平拉伸'),
                h('p', { style: { margin: '5px 0', fontSize: '12px' } }, 
                    h('strong', null, 'Test 6:'), ' 百分比 inset'),
                h('hr', { style: { margin: '10px 0' } }),
                h('p', { style: { margin: '5px 0', fontSize: '11px', color: '#666' } }, 
                    '检查控制台输出以查看调试日志'),
                h('p', { style: { margin: '5px 0', fontSize: '11px', color: '#666' } }, 
                    '(需要启用 LIGHTUI_DEBUG_ABSOLUTE_POSITIONING)')
            )
        );
    }

    console.log('=== Absolute Positioning Test ===');
    console.log('Starting absolute positioning test...');
    console.log('Testing bottom/right inset properties...');
    render(h(App, null), document.body);
    console.log('Absolute positioning test rendered!');
    console.log('Check the debug output above for absolute positioning details.');
    console.log('Expected behavior:');
    console.log('  - Test 1: Element should be at bottom-right corner of container');
    console.log('  - Test 2: Element should be at bottom of container');
    console.log('  - Test 3: Element should be at right side of container');
    console.log('  - Test 4: Element should stretch vertically (height = container_height - top - bottom)');
    console.log('  - Test 5: Element should stretch horizontally (width = container_width - left - right)');
    console.log('  - Test 6: Element should be positioned with percentage offsets');
})();
