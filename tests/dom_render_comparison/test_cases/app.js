/**
 * DOM 渲染测试用例
 * 浏览器和 MBink 加载同一个文件，确保测试环境一致
 * 使用 data-test 属性标识测试元素，使用视口绝对坐标
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
// 测试用例定义 - 使用 data-test 属性标识元素
// ============================================

const testCases = [
    // ==================== 基础渲染测试 ====================
    {
        id: 'BASIC-001',
        name: '默认 div 渲染',
        component: () => h('div', { 'data-test': 'BASIC-001-content' }, 'Hello World')
    },
    {
        id: 'BASIC-002',
        name: '嵌套 div 渲染',
        component: () => h('div', { 'data-test': 'BASIC-002-outer', style: 'width: 200px; padding: 10px; background: #f0f0f0;' },
            h('div', { 'data-test': 'BASIC-002-inner', style: 'background: #ddd; padding: 5px;' }, 'Inner Content')
        )
    },
    {
        id: 'BASIC-003',
        name: '多层嵌套',
        component: () => h('div', { 'data-test': 'BASIC-003-level1', style: 'padding: 10px; background: #e0e0e0;' },
            h('div', { 'data-test': 'BASIC-003-level2', style: 'padding: 10px; background: #d0d0d0;' },
                h('div', { 'data-test': 'BASIC-003-level3', style: 'padding: 10px; background: #c0c0c0;' },
                    h('div', { 'data-test': 'BASIC-003-level4', style: 'padding: 10px; background: #b0b0b0;' }, 'Deep Nested')
                )
            )
        )
    },

    // ==================== 布局测试 ====================
    {
        id: 'LAYOUT-001',
        name: 'Flexbox 行布局',
        component: () => h('div', { 'data-test': 'LAYOUT-001-container', style: 'display: flex; gap: 10px;' },
            h('div', { 'data-test': 'LAYOUT-001-item1', style: 'width: 100px; height: 50px; background: #ff6b6b;' }, '1'),
            h('div', { 'data-test': 'LAYOUT-001-item2', style: 'width: 100px; height: 50px; background: #4ecdc4;' }, '2'),
            h('div', { 'data-test': 'LAYOUT-001-item3', style: 'width: 100px; height: 50px; background: #45b7d1;' }, '3')
        )
    },
    {
        id: 'LAYOUT-002',
        name: 'Flexbox 列布局',
        component: () => h('div', { 'data-test': 'LAYOUT-002-container', style: 'display: flex; flex-direction: column; gap: 10px;' },
            h('div', { 'data-test': 'LAYOUT-002-item1', style: 'height: 30px; background: #ff6b6b;' }, 'Row 1'),
            h('div', { 'data-test': 'LAYOUT-002-item2', style: 'height: 30px; background: #4ecdc4;' }, 'Row 2'),
            h('div', { 'data-test': 'LAYOUT-002-item3', style: 'height: 30px; background: #45b7d1;' }, 'Row 3')
        )
    },
    {
        id: 'LAYOUT-003',
        name: 'Flexbox justify-content: center',
        component: () => h('div', { 'data-test': 'LAYOUT-003-container', style: 'display: flex; justify-content: center; width: 400px; background: #eee;' },
            h('div', { 'data-test': 'LAYOUT-003-item', style: 'width: 100px; height: 50px; background: #ff6b6b;' }, 'Center')
        )
    },
    {
        id: 'LAYOUT-004',
        name: 'Flexbox align-items: center',
        component: () => h('div', { 'data-test': 'LAYOUT-004-container', style: 'display: flex; align-items: center; height: 100px; background: #eee;' },
            h('div', { 'data-test': 'LAYOUT-004-item', style: 'width: 100px; height: 50px; background: #4ecdc4;' }, 'VCenter')
        )
    },

    // ==================== 盒模型测试 ====================
    {
        id: 'BOX-001',
        name: 'Margin 测试',
        component: () => h('div', { 'data-test': 'BOX-001-outer', style: 'background: #eee;' },
            h('div', { 'data-test': 'BOX-001-inner', style: 'margin: 20px; padding: 10px; background: #ff6b6b;' }, 'Margin 20px')
        )
    },
    {
        id: 'BOX-002',
        name: 'Padding 测试',
        component: () => h('div', { 'data-test': 'BOX-002-outer', style: 'padding: 30px; background: #4ecdc4;' },
            h('div', { 'data-test': 'BOX-002-inner', style: 'background: #fff;' }, 'Parent has padding 30px')
        )
    },
    {
        id: 'BOX-003',
        name: 'Border 测试',
        component: () => h('div', { 'data-test': 'BOX-003-content', style: 'border: 5px solid #333; padding: 15px; background: #f0f0f0;' },
            'Border 5px'
        )
    },
    {
        id: 'BOX-004',
        name: 'Box-sizing: border-box',
        component: () => h('div', { 'data-test': 'BOX-004-content', style: 'width: 200px; padding: 20px; border: 5px solid #333; box-sizing: border-box; background: #45b7d1;' },
            'Width should be 200px total'
        )
    },

    // ==================== 文本对齐测试 ====================
    {
        id: 'TEXT-001',
        name: '文本左对齐',
        component: () => h('div', { 'data-test': 'TEXT-001-content', style: 'width: 300px; text-align: left; background: #f0f0f0; padding: 10px;' },
            'Left aligned text'
        )
    },
    {
        id: 'TEXT-002',
        name: '文本居中对齐',
        component: () => h('div', { 'data-test': 'TEXT-002-content', style: 'width: 300px; text-align: center; background: #f0f0f0; padding: 10px;' },
            'Center aligned text'
        )
    },
    {
        id: 'TEXT-003',
        name: '文本右对齐',
        component: () => h('div', { 'data-test': 'TEXT-003-content', style: 'width: 300px; text-align: right; background: #f0f0f0; padding: 10px;' },
            'Right aligned text'
        )
    },

    // ==================== 尺寸约束测试 ====================
    {
        id: 'SIZE-001',
        name: '固定宽高',
        component: () => h('div', { 'data-test': 'SIZE-001-content', style: 'width: 150px; height: 80px; background: #ff6b6b;' },
            'Fixed 150x80'
        )
    },
    {
        id: 'SIZE-002',
        name: '百分比宽度',
        component: () => h('div', { 'data-test': 'SIZE-002-outer', style: 'width: 400px; background: #eee;' },
            h('div', { 'data-test': 'SIZE-002-inner', style: 'width: 50%; height: 40px; background: #4ecdc4;' }, '50% width')
        )
    },

    // ==================== 块级元素测试 ====================
    {
        id: 'BLOCK-001',
        name: '段落 p',
        component: () => h('div', { 'data-test': 'BLOCK-001-container' },
            h('p', { 'data-test': 'BLOCK-001-p1' }, 'First paragraph'),
            h('p', { 'data-test': 'BLOCK-001-p2' }, 'Second paragraph')
        )
    },
    {
        id: 'BLOCK-002',
        name: '标题 h1-h6',
        component: () => h('div', { 'data-test': 'BLOCK-002-container' },
            h('h1', { 'data-test': 'BLOCK-002-h1', style: 'margin: 0;' }, 'H1'),
            h('h2', { 'data-test': 'BLOCK-002-h2', style: 'margin: 0;' }, 'H2'),
            h('h3', { 'data-test': 'BLOCK-002-h3', style: 'margin: 0;' }, 'H3'),
            h('h4', { 'data-test': 'BLOCK-002-h4', style: 'margin: 0;' }, 'H4'),
            h('h5', { 'data-test': 'BLOCK-002-h5', style: 'margin: 0;' }, 'H5'),
            h('h6', { 'data-test': 'BLOCK-002-h6', style: 'margin: 0;' }, 'H6')
        )
    },
    {
        id: 'BLOCK-003',
        name: '语义化布局标签',
        component: () => h('div', { 'data-test': 'BLOCK-003-container', style: 'display: flex; flex-direction: column; gap: 5px;' },
            h('header', { 'data-test': 'BLOCK-003-header', style: 'background: #f0f0f0; padding: 5px;' }, 'Header'),
            h('nav', { 'data-test': 'BLOCK-003-nav', style: 'background: #e0e0e0; padding: 5px;' }, 'Nav'),
            h('main', { 'data-test': 'BLOCK-003-main', style: 'background: #d0d0d0; padding: 5px;' }, 'Main'),
            h('aside', { 'data-test': 'BLOCK-003-aside', style: 'background: #c0c0c0; padding: 5px;' }, 'Aside'),
            h('footer', { 'data-test': 'BLOCK-003-footer', style: 'background: #b0b0b0; padding: 5px;' }, 'Footer')
        )
    },
    {
        id: 'BLOCK-004',
        name: 'pre 和 blockquote',
        component: () => h('div', { 'data-test': 'BLOCK-004-container' },
            h('pre', { 'data-test': 'BLOCK-004-pre', style: 'margin: 0; padding: 10px; background: #f5f5f5;' }, 'Preformatted text'),
            h('blockquote', { 'data-test': 'BLOCK-004-blockquote', style: 'margin: 10px 0;' }, 'This is a blockquote')
        )
    },
    {
        id: 'BLOCK-005',
        name: '水平线 hr',
        component: () => h('div', { 'data-test': 'BLOCK-005-container' },
            h('div', { 'data-test': 'BLOCK-005-before' }, 'Before HR'),
            h('hr', { 'data-test': 'BLOCK-005-hr' }),
            h('div', { 'data-test': 'BLOCK-005-after' }, 'After HR')
        )
    },
    {
        id: 'BLOCK-006',
        name: '无序列表 ul',
        component: () => h('ul', { 'data-test': 'BLOCK-006-ul', style: 'margin: 0;' },
            h('li', { 'data-test': 'BLOCK-006-li1' }, 'Item 1'),
            h('li', { 'data-test': 'BLOCK-006-li2' }, 'Item 2'),
            h('li', { 'data-test': 'BLOCK-006-li3' }, 'Item 3')
        )
    },
    {
        id: 'BLOCK-007',
        name: '有序列表 ol',
        component: () => h('ol', { 'data-test': 'BLOCK-007-ol', style: 'margin: 0;' },
            h('li', { 'data-test': 'BLOCK-007-li1' }, 'First'),
            h('li', { 'data-test': 'BLOCK-007-li2' }, 'Second'),
            h('li', { 'data-test': 'BLOCK-007-li3' }, 'Third')
        )
    },
    {
        id: 'BLOCK-008',
        name: '定义列表 dl',
        component: () => h('dl', { 'data-test': 'BLOCK-008-dl', style: 'margin: 0;' },
            h('dt', { 'data-test': 'BLOCK-008-dt' }, 'Term'),
            h('dd', { 'data-test': 'BLOCK-008-dd' }, 'Definition')
        )
    },
    {
        id: 'BLOCK-009',
        name: 'section 和 article',
        component: () => h('div', { 'data-test': 'BLOCK-009-container' },
            h('section', { 'data-test': 'BLOCK-009-section', style: 'padding: 10px; background: #f0f0f0;' },
                h('article', { 'data-test': 'BLOCK-009-article', style: 'padding: 10px; background: #e0e0e0;' }, 'Article content')
            )
        )
    },
    {
        id: 'BLOCK-010',
        name: 'figure 和 figcaption',
        component: () => h('figure', { 'data-test': 'BLOCK-010-figure', style: 'margin: 0; padding: 10px; background: #f0f0f0;' },
            h('div', { 'data-test': 'BLOCK-010-content', style: 'width: 100px; height: 60px; background: #ccc;' }),
            h('figcaption', { 'data-test': 'BLOCK-010-figcaption' }, 'Figure caption')
        )
    },

    // ==================== 内联元素测试 ====================
    {
        id: 'INLINE-001',
        name: '链接 a',
        component: () => h('div', { 'data-test': 'INLINE-001-container' },
            h('a', { 'data-test': 'INLINE-001-a', href: '#' }, 'Click me')
        )
    },
    {
        id: 'INLINE-002',
        name: '文本样式 strong/em/u/s',
        component: () => h('div', { 'data-test': 'INLINE-002-container' },
            h('strong', { 'data-test': 'INLINE-002-strong' }, 'Bold'),
            ' ',
            h('em', { 'data-test': 'INLINE-002-em' }, 'Italic'),
            ' ',
            h('u', { 'data-test': 'INLINE-002-u' }, 'Underline'),
            ' ',
            h('s', { 'data-test': 'INLINE-002-s' }, 'Strike')
        )
    },
    {
        id: 'INLINE-003',
        name: '上下标 sub/sup',
        component: () => h('div', { 'data-test': 'INLINE-003-container' },
            'H',
            h('sub', { 'data-test': 'INLINE-003-sub' }, '2'),
            'O and E=mc',
            h('sup', { 'data-test': 'INLINE-003-sup' }, '2')
        )
    },
    {
        id: 'INLINE-004',
        name: '代码 code/kbd',
        component: () => h('div', { 'data-test': 'INLINE-004-container' },
            h('code', { 'data-test': 'INLINE-004-code' }, 'const x = 1;'),
            ' Press ',
            h('kbd', { 'data-test': 'INLINE-004-kbd' }, 'Ctrl+C')
        )
    },
    {
        id: 'INLINE-005',
        name: '标记和小字 mark/small',
        component: () => h('div', { 'data-test': 'INLINE-005-container' },
            h('mark', { 'data-test': 'INLINE-005-mark' }, 'Highlighted'),
            ' ',
            h('small', { 'data-test': 'INLINE-005-small' }, 'Small text')
        )
    },
    {
        id: 'INLINE-006',
        name: '引用 q/cite',
        component: () => h('div', { 'data-test': 'INLINE-006-container' },
            h('q', { 'data-test': 'INLINE-006-q' }, 'Quote'),
            ' - ',
            h('cite', { 'data-test': 'INLINE-006-cite' }, 'Citation')
        )
    },
    {
        id: 'INLINE-007',
        name: '缩写和定义 abbr/dfn',
        component: () => h('div', { 'data-test': 'INLINE-007-container' },
            h('abbr', { 'data-test': 'INLINE-007-abbr', title: 'HyperText Markup Language' }, 'HTML'),
            ' is a ',
            h('dfn', { 'data-test': 'INLINE-007-dfn' }, 'markup language')
        )
    },
    {
        id: 'INLINE-008',
        name: 'time 和 data',
        component: () => h('div', { 'data-test': 'INLINE-008-container' },
            h('time', { 'data-test': 'INLINE-008-time', datetime: '2025-12-07' }, 'December 7'),
            ' ',
            h('data', { 'data-test': 'INLINE-008-data', value: '42' }, 'Forty-two')
        )
    },
    {
        id: 'INLINE-009',
        name: 'label',
        component: () => h('div', { 'data-test': 'INLINE-009-container' },
            h('label', { 'data-test': 'INLINE-009-label' }, 'Label text')
        )
    },

    // ==================== 表格元素测试 ====================
    {
        id: 'TABLE-001',
        name: '基础表格',
        component: () => h('table', { 'data-test': 'TABLE-001-table', style: 'border-collapse: collapse; width: 300px;' },
            h('thead', { 'data-test': 'TABLE-001-thead' },
                h('tr', { 'data-test': 'TABLE-001-tr-head' },
                    h('th', { 'data-test': 'TABLE-001-th1', style: 'border: 1px solid #333; padding: 8px;' }, 'Header 1'),
                    h('th', { 'data-test': 'TABLE-001-th2', style: 'border: 1px solid #333; padding: 8px;' }, 'Header 2')
                )
            ),
            h('tbody', { 'data-test': 'TABLE-001-tbody' },
                h('tr', { 'data-test': 'TABLE-001-tr1' },
                    h('td', { 'data-test': 'TABLE-001-td1', style: 'border: 1px solid #333; padding: 8px;' }, 'Cell 1'),
                    h('td', { 'data-test': 'TABLE-001-td2', style: 'border: 1px solid #333; padding: 8px;' }, 'Cell 2')
                ),
                h('tr', { 'data-test': 'TABLE-001-tr2' },
                    h('td', { 'data-test': 'TABLE-001-td3', style: 'border: 1px solid #333; padding: 8px;' }, 'Cell 3'),
                    h('td', { 'data-test': 'TABLE-001-td4', style: 'border: 1px solid #333; padding: 8px;' }, 'Cell 4')
                )
            )
        )
    },
    {
        id: 'TABLE-002',
        name: '表格标题 caption',
        component: () => h('table', { 'data-test': 'TABLE-002-table', style: 'border-collapse: collapse; width: 250px;' },
            h('caption', { 'data-test': 'TABLE-002-caption', style: 'padding: 5px; font-weight: bold;' }, 'Table Caption'),
            h('tbody', null,
                h('tr', { 'data-test': 'TABLE-002-tr' },
                    h('td', { 'data-test': 'TABLE-002-td1', style: 'border: 1px solid #333; padding: 5px;' }, 'A'),
                    h('td', { 'data-test': 'TABLE-002-td2', style: 'border: 1px solid #333; padding: 5px;' }, 'B')
                )
            )
        )
    },
    {
        id: 'TABLE-003',
        name: '表格 tfoot',
        component: () => h('table', { 'data-test': 'TABLE-003-table', style: 'border-collapse: collapse; width: 200px;' },
            h('thead', null,
                h('tr', { 'data-test': 'TABLE-003-tr-head' },
                    h('th', { 'data-test': 'TABLE-003-th', style: 'border: 1px solid #333; padding: 5px;' }, 'Name')
                )
            ),
            h('tbody', null,
                h('tr', { 'data-test': 'TABLE-003-tr-body' },
                    h('td', { 'data-test': 'TABLE-003-td', style: 'border: 1px solid #333; padding: 5px;' }, 'Value')
                )
            ),
            h('tfoot', { 'data-test': 'TABLE-003-tfoot' },
                h('tr', { 'data-test': 'TABLE-003-tr-foot' },
                    h('td', { 'data-test': 'TABLE-003-td-foot', style: 'border: 1px solid #333; padding: 5px; background: #eee;' }, 'Footer')
                )
            )
        )
    },
    {
        id: 'TABLE-004',
        name: '表格单元格合并 colspan',
        component: () => h('table', { 'data-test': 'TABLE-004-table', style: 'border-collapse: collapse; width: 300px;' },
            h('tbody', null,
                h('tr', { 'data-test': 'TABLE-004-tr1' },
                    h('td', { 'data-test': 'TABLE-004-td-span', colspan: '2', style: 'border: 1px solid #333; padding: 5px; text-align: center;' }, 'Span 2 columns')
                ),
                h('tr', { 'data-test': 'TABLE-004-tr2' },
                    h('td', { 'data-test': 'TABLE-004-td1', style: 'border: 1px solid #333; padding: 5px;' }, 'Left'),
                    h('td', { 'data-test': 'TABLE-004-td2', style: 'border: 1px solid #333; padding: 5px;' }, 'Right')
                )
            )
        )
    },

    // ==================== 表单元素测试 ====================
    {
        id: 'FORM-001',
        name: '文本输入框 input[text]',
        component: () => h('div', { 'data-test': 'FORM-001-container' },
            h('input', { 'data-test': 'FORM-001-input', type: 'text', value: 'Hello', style: 'width: 200px; padding: 5px;' })
        )
    },
    {
        id: 'FORM-002',
        name: '按钮 button',
        component: () => h('div', { 'data-test': 'FORM-002-container', style: 'display: flex; gap: 10px;' },
            h('button', { 'data-test': 'FORM-002-button1', style: 'padding: 10px 20px;' }, 'Click Me'),
            h('button', { 'data-test': 'FORM-002-button2', type: 'submit', style: 'padding: 10px 20px;' }, 'Submit')
        )
    },
    {
        id: 'FORM-003',
        name: '复选框 checkbox',
        component: () => h('div', { 'data-test': 'FORM-003-container' },
            h('label', { 'data-test': 'FORM-003-label', style: 'display: flex; align-items: center; gap: 5px;' },
                h('input', { 'data-test': 'FORM-003-checkbox', type: 'checkbox' }),
                'Check me'
            )
        )
    },
    {
        id: 'FORM-004',
        name: '单选按钮 radio',
        component: () => h('div', { 'data-test': 'FORM-004-container', style: 'display: flex; flex-direction: column; gap: 5px;' },
            h('label', { 'data-test': 'FORM-004-label1', style: 'display: flex; align-items: center; gap: 5px;' },
                h('input', { 'data-test': 'FORM-004-radio1', type: 'radio', name: 'option' }),
                'Option 1'
            ),
            h('label', { 'data-test': 'FORM-004-label2', style: 'display: flex; align-items: center; gap: 5px;' },
                h('input', { 'data-test': 'FORM-004-radio2', type: 'radio', name: 'option' }),
                'Option 2'
            )
        )
    },
    {
        id: 'FORM-005',
        name: '下拉框 select',
        component: () => h('div', { 'data-test': 'FORM-005-container' },
            h('select', { 'data-test': 'FORM-005-select', style: 'width: 150px; padding: 5px;' },
                h('option', { value: '1' }, 'Option 1'),
                h('option', { value: '2' }, 'Option 2'),
                h('option', { value: '3' }, 'Option 3')
            )
        )
    },
    {
        id: 'FORM-006',
        name: '多行文本 textarea',
        component: () => h('div', { 'data-test': 'FORM-006-container' },
            h('textarea', { 'data-test': 'FORM-006-textarea', style: 'width: 250px; height: 80px; padding: 5px;' }, 'Multi-line text')
        )
    },
    {
        id: 'FORM-007',
        name: '表单布局 form+fieldset',
        component: () => h('form', { 'data-test': 'FORM-007-form', style: 'margin: 0;' },
            h('fieldset', { 'data-test': 'FORM-007-fieldset', style: 'padding: 10px;' },
                h('legend', { 'data-test': 'FORM-007-legend' }, 'User Info'),
                h('div', { 'data-test': 'FORM-007-field', style: 'margin-top: 5px;' },
                    h('label', null, 'Name: '),
                    h('input', { 'data-test': 'FORM-007-input', type: 'text', style: 'width: 150px;' })
                )
            )
        )
    },
    {
        id: 'FORM-008',
        name: '范围滑块 input[range]',
        component: () => h('div', { 'data-test': 'FORM-008-container' },
            h('input', { 'data-test': 'FORM-008-range', type: 'range', min: '0', max: '100', value: '50', style: 'width: 200px;' })
        )
    },
    {
        id: 'FORM-009',
        name: '进度条 progress',
        component: () => h('div', { 'data-test': 'FORM-009-container' },
            h('progress', { 'data-test': 'FORM-009-progress', value: '70', max: '100', style: 'width: 200px;' })
        )
    },
    {
        id: 'FORM-010',
        name: '度量 meter',
        component: () => h('div', { 'data-test': 'FORM-010-container' },
            h('meter', { 'data-test': 'FORM-010-meter', value: '0.6', min: '0', max: '1', style: 'width: 200px;' })
        )
    },

    // ==================== SVG 元素测试 ====================
    // 注意：SVG 容器使用 display: flex 或 line-height: 0 来避免 inline SVG 的行高空间差异
    {
        id: 'SVG-001',
        name: '基础 SVG 圆形',
        component: () => h('div', { 'data-test': 'SVG-001-container', style: 'line-height: 0;' },
            h('svg', { 'data-test': 'SVG-001-svg', width: '100', height: '100', style: 'background: #f0f0f0;' },
                h('circle', { 'data-test': 'SVG-001-circle', cx: '50', cy: '50', r: '40', fill: 'red' })
            )
        )
    },
    {
        id: 'SVG-002',
        name: 'SVG 在 flex 容器中',
        component: () => h('div', { 'data-test': 'SVG-002-container', style: 'display: flex; gap: 10px; align-items: center;' },
            h('svg', { 'data-test': 'SVG-002-svg', width: '60', height: '60' },
                h('circle', { cx: '30', cy: '30', r: '25', fill: 'blue' })
            ),
            h('button', { 'data-test': 'SVG-002-button', style: 'padding: 10px 20px;' }, 'Button')
        )
    },
    {
        id: 'SVG-003',
        name: 'SVG 矩形和文本',
        component: () => h('div', { 'data-test': 'SVG-003-container', style: 'line-height: 0;' },
            h('svg', { 'data-test': 'SVG-003-svg', width: '150', height: '80', style: 'background: #eee;' },
                h('rect', { 'data-test': 'SVG-003-rect', x: '10', y: '10', width: '130', height: '60', fill: '#4CAF50', rx: '5' }),
                h('text', { 'data-test': 'SVG-003-text', x: '75', y: '45', 'text-anchor': 'middle', fill: 'white', 'font-size': '14' }, 'SVG Text')
            )
        )
    },
    {
        id: 'SVG-004',
        name: 'SVG 多个形状',
        component: () => h('div', { 'data-test': 'SVG-004-container', style: 'line-height: 0;' },
            h('svg', { 'data-test': 'SVG-004-svg', width: '200', height: '100', style: 'background: #fafafa;' },
                h('circle', { cx: '30', cy: '50', r: '25', fill: 'red' }),
                h('rect', { x: '70', y: '25', width: '50', height: '50', fill: 'green' }),
                h('ellipse', { cx: '160', cy: '50', rx: '30', ry: '20', fill: 'blue' })
            )
        )
    },
    {
        id: 'SVG-005',
        name: 'SVG 在 flex 容器中居中',
        component: () => h('div', { 'data-test': 'SVG-005-container', style: 'display: flex; justify-content: center; align-items: center; height: 120px; background: #f5f5f5;' },
            h('svg', { 'data-test': 'SVG-005-svg', width: '80', height: '80' },
                h('rect', { x: '0', y: '0', width: '80', height: '80', fill: 'none', stroke: '#333', 'stroke-width': '2' }),
                h('line', { x1: '0', y1: '0', x2: '80', y2: '80', stroke: '#333', 'stroke-width': '2' }),
                h('line', { x1: '80', y1: '0', x2: '0', y2: '80', stroke: '#333', 'stroke-width': '2' })
            )
        )
    },
    {
        id: 'SVG-006',
        name: 'SVG path 路径',
        component: () => h('div', { 'data-test': 'SVG-006-container', style: 'line-height: 0;' },
            h('svg', { 'data-test': 'SVG-006-svg', width: '100', height: '100', viewBox: '0 0 100 100' },
                h('path', { 'data-test': 'SVG-006-path', d: 'M10,50 Q50,10 90,50 T90,90', stroke: 'purple', fill: 'none', 'stroke-width': '3' })
            )
        )
    },
    {
        id: 'SVG-007',
        name: 'SVG 与按钮并排 (flex)',
        component: () => h('div', { 'data-test': 'SVG-007-container', style: 'display: flex; gap: 15px; align-items: center;' },
            h('button', { 'data-test': 'SVG-007-button1', style: 'padding: 8px 16px;' }, 'Left'),
            h('svg', { 'data-test': 'SVG-007-svg', width: '50', height: '50' },
                h('polygon', { points: '25,5 45,45 5,45', fill: 'orange' })
            ),
            h('button', { 'data-test': 'SVG-007-button2', style: 'padding: 8px 16px;' }, 'Right')
        )
    },
    {
        id: 'SVG-008',
        name: 'SVG 图标按钮',
        component: () => h('div', { 'data-test': 'SVG-008-container', style: 'display: flex; gap: 10px;' },
            h('button', { 'data-test': 'SVG-008-button', style: 'display: flex; align-items: center; gap: 5px; padding: 8px 12px;' },
                h('svg', { 'data-test': 'SVG-008-icon', width: '16', height: '16', viewBox: '0 0 24 24' },
                    h('path', { d: 'M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5', stroke: 'currentColor', fill: 'none', 'stroke-width': '2' })
                ),
                'Icon Button'
            )
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
        'data-test': 'case-' + testCase.id,
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
    return h('div', { 'data-test': 'test-container', style: 'padding: 10px;' },
        testCases.map(function(tc) { return h(TestCase, { key: tc.id, testCase: tc }); })
    );
}

// ============================================
// 渲染数据提取函数（浏览器端使用）
// 使用 data-test 属性标识元素，输出视口绝对坐标
// ============================================

function extractAllElements() {
    var elements = document.querySelectorAll('[data-test]');
    var result = {};

    elements.forEach(function(element) {
        var testId = element.getAttribute('data-test');
        var rect = element.getBoundingClientRect();
        var style = window.getComputedStyle(element);

        result[testId] = {
            viewport: {
                x: Math.round(rect.left * 100) / 100,
                y: Math.round(rect.top * 100) / 100,
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
            }
        };
    });

    return result;
}

function extractRenderData() {
    return {
        source: 'Browser',
        viewport: {
            width: window.innerWidth,
            height: window.innerHeight
        },
        elements: extractAllElements()
    };
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
        extractAll: extractAllElements,
        print: printRenderData
    };
}

})(); // 关闭 IIFE
