/**
 * DOM 渲染测试用例
 * 浏览器和 MBink 加载同一个文件，确保测试环境一致
 */

// 使用 IIFE 避免变量冲突
(function() {
    // 兼容多种 Preact 导出方式
    var preactLib = (typeof Preact !== 'undefined') ? Preact :
                    (typeof window !== 'undefined' && window.preact) ? window.preact :
                    (typeof preact !== 'undefined') ? preact : null;

    if (!preactLib) {
        console.error('[app.js] Preact library not found!');
        return;
    }

    var h = preactLib.h;
    var render = preactLib.render;
    var Component = preactLib.Component;

// ============================================
// 测试用例定义
// ============================================

const testCases = [
    // ==================== 基础渲染测试 ====================
    {
        id: 'BASIC-001',
        name: '默认 div 渲染',
        component: () => h('div', { id: 'test-BASIC-001' }, 'Hello World')
    },
    {
        id: 'BASIC-002',
        name: '嵌套 div 渲染',
        component: () => h('div', { id: 'test-BASIC-002', style: 'width: 200px; padding: 10px; background: #f0f0f0;' },
            h('div', { id: 'test-BASIC-002-inner', style: 'background: #ddd; padding: 5px;' }, 'Inner Content')
        )
    },
    {
        id: 'BASIC-003',
        name: '多层嵌套',
        component: () => h('div', { id: 'test-BASIC-003', style: 'padding: 10px; background: #e0e0e0;' },
            h('div', { style: 'padding: 10px; background: #d0d0d0;' },
                h('div', { style: 'padding: 10px; background: #c0c0c0;' },
                    h('div', { style: 'padding: 10px; background: #b0b0b0;' }, 'Deep Nested')
                )
            )
        )
    },

    // ==================== 布局测试 ====================
    {
        id: 'LAYOUT-001',
        name: 'Flexbox 行布局',
        component: () => h('div', { id: 'test-LAYOUT-001', style: 'display: flex; gap: 10px;' },
            h('div', { style: 'width: 100px; height: 50px; background: #ff6b6b;' }, '1'),
            h('div', { style: 'width: 100px; height: 50px; background: #4ecdc4;' }, '2'),
            h('div', { style: 'width: 100px; height: 50px; background: #45b7d1;' }, '3')
        )
    },
    {
        id: 'LAYOUT-002',
        name: 'Flexbox 列布局',
        component: () => h('div', { id: 'test-LAYOUT-002', style: 'display: flex; flex-direction: column; gap: 10px;' },
            h('div', { style: 'height: 30px; background: #ff6b6b;' }, 'Row 1'),
            h('div', { style: 'height: 30px; background: #4ecdc4;' }, 'Row 2'),
            h('div', { style: 'height: 30px; background: #45b7d1;' }, 'Row 3')
        )
    },
    {
        id: 'LAYOUT-003',
        name: 'Flexbox justify-content: center',
        component: () => h('div', { id: 'test-LAYOUT-003', style: 'display: flex; justify-content: center; width: 400px; background: #eee;' },
            h('div', { style: 'width: 100px; height: 50px; background: #ff6b6b;' }, 'Center')
        )
    },
    {
        id: 'LAYOUT-004',
        name: 'Flexbox align-items: center',
        component: () => h('div', { id: 'test-LAYOUT-004', style: 'display: flex; align-items: center; height: 100px; background: #eee;' },
            h('div', { style: 'width: 100px; height: 50px; background: #4ecdc4;' }, 'VCenter')
        )
    },

    // ==================== 盒模型测试 ====================
    {
        id: 'BOX-001',
        name: 'Margin 测试',
        component: () => h('div', { id: 'test-BOX-001', style: 'background: #eee;' },
            h('div', { style: 'margin: 20px; padding: 10px; background: #ff6b6b;' }, 'Margin 20px')
        )
    },
    {
        id: 'BOX-002',
        name: 'Padding 测试',
        component: () => h('div', { id: 'test-BOX-002', style: 'padding: 30px; background: #4ecdc4;' },
            h('div', { style: 'background: #fff;' }, 'Parent has padding 30px')
        )
    },
    {
        id: 'BOX-003',
        name: 'Border 测试',
        component: () => h('div', { id: 'test-BOX-003', style: 'border: 5px solid #333; padding: 15px; background: #f0f0f0;' },
            'Border 5px'
        )
    },
    {
        id: 'BOX-004',
        name: 'Box-sizing: border-box',
        component: () => h('div', { id: 'test-BOX-004', style: 'width: 200px; padding: 20px; border: 5px solid #333; box-sizing: border-box; background: #45b7d1;' },
            'Width should be 200px total'
        )
    },

    // ==================== 文本对齐测试 ====================
    {
        id: 'TEXT-001',
        name: '文本左对齐',
        component: () => h('div', { id: 'test-TEXT-001', style: 'width: 300px; text-align: left; background: #f0f0f0; padding: 10px;' },
            'Left aligned text'
        )
    },
    {
        id: 'TEXT-002',
        name: '文本居中对齐',
        component: () => h('div', { id: 'test-TEXT-002', style: 'width: 300px; text-align: center; background: #f0f0f0; padding: 10px;' },
            'Center aligned text'
        )
    },
    {
        id: 'TEXT-003',
        name: '文本右对齐',
        component: () => h('div', { id: 'test-TEXT-003', style: 'width: 300px; text-align: right; background: #f0f0f0; padding: 10px;' },
            'Right aligned text'
        )
    },

    // ==================== 尺寸约束测试 ====================
    {
        id: 'SIZE-001',
        name: '固定宽高',
        component: () => h('div', { id: 'test-SIZE-001', style: 'width: 150px; height: 80px; background: #ff6b6b;' },
            'Fixed 150x80'
        )
    },
    {
        id: 'SIZE-002',
        name: '百分比宽度',
        component: () => h('div', { id: 'test-SIZE-002', style: 'width: 400px; background: #eee;' },
            h('div', { style: 'width: 50%; height: 40px; background: #4ecdc4;' }, '50% width')
        )
    }
];

// ============================================
// 测试容器组件（使用函数组件）
// ============================================

function TestCase(props) {
    var testCase = props.testCase;
    var TestComponent = testCase.component;
    return h('div', {
        class: 'test-case',
        'data-test-id': testCase.id,
        style: 'margin-bottom: 15px; border: 1px dashed #ccc; padding: 10px;'
    },
        h('div', { style: 'font-size: 12px; color: #666; margin-bottom: 5px;' },
            testCase.id + ': ' + testCase.name
        ),
        h(TestComponent)
    );
}

function TestApp() {
    console.log('[TestApp] Rendering with ' + testCases.length + ' test cases');
    return h('div', { id: 'test-container', style: 'padding: 10px;' },
        testCases.map(function(tc) { return h(TestCase, { key: tc.id, testCase: tc }); })
    );
}

// ============================================
// 渲染数据提取函数（浏览器端使用）
// ============================================

function extractElementData(element, depth) {
    if (depth === undefined) depth = 0;
    
    var rect = element.getBoundingClientRect();
    var style = window.getComputedStyle(element);
    
    var data = {
        tag: element.tagName.toLowerCase(),
        id: element.id || null,
        className: element.className || null,
        depth: depth,
        layout: {
            x: Math.round(rect.x * 100) / 100,
            y: Math.round(rect.y * 100) / 100,
            width: Math.round(rect.width * 100) / 100,
            height: Math.round(rect.height * 100) / 100
        },
        box: {
            marginTop: parseFloat(style.marginTop) || 0,
            marginRight: parseFloat(style.marginRight) || 0,
            marginBottom: parseFloat(style.marginBottom) || 0,
            marginLeft: parseFloat(style.marginLeft) || 0,
            paddingTop: parseFloat(style.paddingTop) || 0,
            paddingRight: parseFloat(style.paddingRight) || 0,
            paddingBottom: parseFloat(style.paddingBottom) || 0,
            paddingLeft: parseFloat(style.paddingLeft) || 0,
            borderTop: parseFloat(style.borderTopWidth) || 0,
            borderRight: parseFloat(style.borderRightWidth) || 0,
            borderBottom: parseFloat(style.borderBottomWidth) || 0,
            borderLeft: parseFloat(style.borderLeftWidth) || 0
        },
        style: {
            display: style.display,
            position: style.position,
            textAlign: style.textAlign
        },
        children: []
    };
    
    // 递归提取子元素
    for (var i = 0; i < element.children.length; i++) {
        data.children.push(extractElementData(element.children[i], depth + 1));
    }
    
    return data;
}

function extractRenderData() {
    var container = document.getElementById('test-container');
    if (!container) {
        console.error('Test container not found');
        return null;
    }
    return extractElementData(container, 0);
}

// 输出 JSON（浏览器端调用）
function printRenderData() {
    var data = extractRenderData();
    console.log('__RENDER_DATA__' + JSON.stringify(data));
    return data;
}

// ============================================
// 启动渲染
// ============================================

render(h(TestApp), document.body);

// 导出供外部调用
if (typeof window !== 'undefined') {
    window.DOMRenderTest = {
        cases: testCases,
        extract: extractRenderData,
        print: printRenderData
    };
}

})(); // 关闭 IIFE
