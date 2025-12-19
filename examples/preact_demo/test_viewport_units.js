/**
 * @file test_viewport_units.js
 * @brief 视口单位 (vh, vw, vmin, vmax) 测试用例
 * 
 * 此测试用于验证视口单位是否正确解析和计算。
 * 预期行为：
 * - height: 100vh 应该等于视口高度
 * - width: 100vw 应该等于视口宽度
 * - 50vh/50vw 应该等于视口尺寸的一半
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;

    function App() {
        return h(
            'div',
            { 
                style: { 
                    padding: '0',
                    margin: '0',
                    backgroundColor: '#f0f0f0'
                } 
            },
            // 测试 1: 100vh 高度容器
            h('div', { 
                id: 'test-100vh',
                style: { 
                    height: '100vh',
                    width: '200px',
                    backgroundColor: 'rgba(255, 0, 0, 0.3)',
                    position: 'absolute',
                    top: '0',
                    left: '0',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                } 
            }, h('span', { style: { color: 'white', fontWeight: 'bold' } }, '100vh')),
            
            // 测试 2: 50vh 高度容器
            h('div', { 
                id: 'test-50vh',
                style: { 
                    height: '50vh',
                    width: '200px',
                    backgroundColor: 'rgba(0, 255, 0, 0.3)',
                    position: 'absolute',
                    top: '0',
                    left: '220px',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                } 
            }, h('span', { style: { color: 'white', fontWeight: 'bold' } }, '50vh')),
            
            // 测试 3: 100vw 宽度容器
            h('div', { 
                id: 'test-100vw',
                style: { 
                    height: '50px',
                    width: '100vw',
                    backgroundColor: 'rgba(0, 0, 255, 0.3)',
                    position: 'absolute',
                    bottom: '100px',
                    left: '0',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                } 
            }, h('span', { style: { color: 'white', fontWeight: 'bold' } }, '100vw')),
            
            // 测试 4: 50vw 宽度容器
            h('div', { 
                id: 'test-50vw',
                style: { 
                    height: '50px',
                    width: '50vw',
                    backgroundColor: 'rgba(255, 255, 0, 0.5)',
                    position: 'absolute',
                    bottom: '40px',
                    left: '0',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                } 
            }, h('span', { style: { fontWeight: 'bold' } }, '50vw')),
            
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
                    fontSize: '14px'
                } 
            }, 
                h('h3', { style: { margin: '0 0 10px 0' } }, '视口单位测试'),
                h('p', { style: { margin: '5px 0' } }, '红色: height: 100vh'),
                h('p', { style: { margin: '5px 0' } }, '绿色: height: 50vh'),
                h('p', { style: { margin: '5px 0' } }, '蓝色: width: 100vw'),
                h('p', { style: { margin: '5px 0' } }, '黄色: width: 50vw'),
                h('hr', { style: { margin: '10px 0' } }),
                h('p', { style: { margin: '5px 0', fontSize: '12px', color: '#666' } }, 
                    '检查控制台输出以查看调试日志')
            )
        );
    }

    console.log('=== Viewport Units Test ===');
    console.log('Starting viewport units test...');
    render(h(App, null), document.body);
    console.log('Viewport units test rendered!');
    console.log('Check the debug output above for viewport unit resolution details.');
})();
