/**
 * @file app.js
 * @brief HTML Tags Comprehensive Test - 测试所有已实现的 HTML 标签
 * 
 * 测试覆盖:
 * - Phase 1: 语义化标签 (30+个)
 * - Phase 2: 列表标签 (ol, li)
 * - Phase 3: 表格标签 (10个)
 * - Phase 4: SVG 支持 (10个)
 * - Phase 5: 表单增强标签 (8个)
 */

var h = Preact.h;
var render = Preact.render;
var useState = PreactHooks.useState;

// ========== Phase 1: 语义化标签测试 ==========

function SemanticTagsTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #4CAF50;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #4CAF50;' }, '📝 Phase 1: 语义化标签'),
        // 布局标签
        h('h3', { style: 'color: #666;' }, '布局标签'),
        h('header', { style: 'background: #e3f2fd; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'header'), ' - 页面头部'
        ),
        h('nav', { style: 'background: #e8f5e9; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'nav'), ' - 导航区域'
        ),
        h('main', { style: 'background: #fff3e0; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'main'), ' - 主内容区域'
        ),
        h('article', { style: 'background: #fce4ec; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'article'), ' - 文章内容'
        ),
        h('section', { style: 'background: #f3e5f5; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'section'), ' - 章节'
        ),
        h('aside', { style: 'background: #e0f7fa; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'aside'), ' - 侧边栏'
        ),
        h('footer', { style: 'background: #efebe9; padding: 10px; margin: 5px 0;' }, 
            h('strong', {}, 'footer'), ' - 页脚'
        ),
        
        // 文本标签
        h('h3', { style: 'color: #666; margin-top: 20px;' }, '文本标签'),
        h('p', { style: 'margin: 5px 0;' },
            h('strong', {}, '加粗'), ' | ',
            h('b', {}, 'bold'), ' | ',
            h('em', {}, '斜体'), ' | ',
            h('i', {}, 'italic'), ' | ',
            h('u', {}, '下划线'), ' | ',
            h('s', {}, '删除线'), ' | ',
            h('mark', {}, '高亮'), ' | ',
            h('small', {}, '小字'), ' | ',
            h('sub', {}, '下标'), ' | ',
            h('sup', {}, '上标')
        ),
        h('p', { style: 'margin: 5px 0;' },
            h('code', {}, 'code 代码'), ' | ',
            h('kbd', {}, 'kbd 键盘'), ' | ',
            h('samp', {}, 'samp 输出'), ' | ',
            h('var', {}, 'var 变量')
        ),
        h('p', { style: 'margin: 5px 0;' },
            h('cite', {}, 'cite 引用'), ' | ',
            h('q', {}, 'q 引用'), ' | ',
            h('abbr', { title: 'HyperText Markup Language' }, 'abbr HTML'), ' | ',
            h('time', { datetime: '2025-12-01' }, 'time 时间'), ' | ',
            h('dfn', {}, 'dfn 定义')
        ),
        
        // 其他语义标签
        h('h3', { style: 'color: #666; margin-top: 20px;' }, '其他标签'),
        h('pre', { style: 'background: #f5f5f5; padding: 10px; margin: 5px 0;' }, 
            'pre 预格式化文本\n  保留空格和换行'
        ),
        h('blockquote', { style: 'border-left: 4px solid #ccc; padding-left: 15px; margin: 10px 0; color: #666;' }, 
            'blockquote - 这是一段引用文字'
        ),
        h('address', { style: 'font-style: italic; margin: 5px 0;' }, 
            'address - 联系地址信息'
        ),
        h('figure', { style: 'border: 1px solid #ddd; padding: 10px; margin: 10px 0;' },
            h('div', { style: 'background: #eee; height: 50px; display: flex; align-items: center; justify-content: center;' }, '[图片占位]'),
            h('figcaption', { style: 'text-align: center; color: #666; margin-top: 5px;' }, 'figcaption - 图片说明')
        ),
        h('hr', { style: 'margin: 15px 0;' }),
        h('p', {}, '水平分割线 ', h('code', {}, 'hr'), ' 在上面')
    );
}

// ========== Phase 2: 列表标签测试 ==========

function ListTagsTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #FF9800;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #FF9800;' }, '📋 Phase 2: 列表标签'),
        h('div', { style: 'display: flex; gap: 40px;' },
            // 无序列表
            h('div', {},
                h('h3', { style: 'color: #666;' }, '无序列表 ul'),
                h('ul', { style: 'margin: 10px 0; padding-left: 20px;' },
                    h('li', {}, '第一项'),
                    h('li', {}, '第二项'),
                    h('li', {}, '第三项'),
                    h('li', {}, '嵌套列表：',
                        h('ul', { style: 'padding-left: 20px;' },
                            h('li', {}, '嵌套项 A'),
                            h('li', {}, '嵌套项 B')
                        )
                    )
                )
            ),
            // 有序列表
            h('div', {},
                h('h3', { style: 'color: #666;' }, '有序列表 ol'),
                h('ol', { start: 1, style: 'margin: 10px 0; padding-left: 20px;' },
                    h('li', {}, '步骤一'),
                    h('li', {}, '步骤二'),
                    h('li', {}, '步骤三'),
                    h('li', { value: 10 }, '跳到10')
                )
            ),
            // 定义列表
            h('div', {},
                h('h3', { style: 'color: #666;' }, '定义列表 dl'),
                h('dl', { style: 'margin: 10px 0;' },
                    h('dt', { style: 'font-weight: bold;' }, 'HTML'),
                    h('dd', { style: 'margin-left: 20px; color: #666;' }, '超文本标记语言'),
                    h('dt', { style: 'font-weight: bold;' }, 'CSS'),
                    h('dd', { style: 'margin-left: 20px; color: #666;' }, '层叠样式表'),
                    h('dt', { style: 'font-weight: bold;' }, 'JS'),
                    h('dd', { style: 'margin-left: 20px; color: #666;' }, 'JavaScript')
                )
            )
        )
    );
}

// ========== Phase 3: 表格标签测试 ==========

function TableTagsTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #9C27B0;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #9C27B0;' }, '📊 Phase 3: 表格标签'),
        h('table', { style: 'width: 100%; border-collapse: collapse; margin: 10px 0;' },
            h('caption', { style: 'caption-side: top; font-weight: bold; padding: 10px; background: #f5f5f5;' }, 
                'caption - 用户信息表'
            ),
            h('colgroup', {},
                h('col', { style: 'background: #e3f2fd;' }),
                h('col', { style: 'background: #fff;' }),
                h('col', { style: 'background: #e8f5e9;' }),
                h('col', { style: 'background: #fff;' })
            ),
            h('thead', {},
                h('tr', {},
                    h('th', { style: 'border: 1px solid #ddd; padding: 12px; background: #2196F3; color: white;' }, 'ID'),
                    h('th', { style: 'border: 1px solid #ddd; padding: 12px; background: #2196F3; color: white;' }, '姓名'),
                    h('th', { style: 'border: 1px solid #ddd; padding: 12px; background: #2196F3; color: white;' }, '部门'),
                    h('th', { style: 'border: 1px solid #ddd; padding: 12px; background: #2196F3; color: white;' }, '状态')
                )
            ),
            h('tbody', {},
                h('tr', {},
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; text-align: center;' }, '001'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '张三'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '技术部'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; color: green;' }, '在职')
                ),
                h('tr', {},
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; text-align: center;' }, '002'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '李四'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '产品部'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; color: green;' }, '在职')
                ),
                h('tr', {},
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; text-align: center;' }, '003'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '王五'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px;' }, '设计部'),
                    h('td', { style: 'border: 1px solid #ddd; padding: 10px; color: orange;' }, '休假')
                )
            ),
            h('tfoot', {},
                h('tr', {},
                    h('td', { colspan: 4, style: 'border: 1px solid #ddd; padding: 10px; text-align: center; background: #f5f5f5;' }, 
                        'tfoot - 共 3 条记录'
                    )
                )
            )
        ),
        
        // colspan/rowspan 测试
        h('h3', { style: 'color: #666; margin-top: 20px;' }, 'colspan/rowspan 测试'),
        h('table', { style: 'border-collapse: collapse; margin: 10px 0;' },
            h('tr', {},
                h('th', { colspan: 2, style: 'border: 1px solid #9C27B0; padding: 10px; background: #9C27B0; color: white;' }, 'colspan=2 合并列'),
                h('th', { rowspan: 2, style: 'border: 1px solid #9C27B0; padding: 10px; background: #CE93D8;' }, 'rowspan=2')
            ),
            h('tr', {},
                h('td', { style: 'border: 1px solid #9C27B0; padding: 10px;' }, 'A1'),
                h('td', { style: 'border: 1px solid #9C27B0; padding: 10px;' }, 'B1')
            ),
            h('tr', {},
                h('td', { style: 'border: 1px solid #9C27B0; padding: 10px;' }, 'A2'),
                h('td', { style: 'border: 1px solid #9C27B0; padding: 10px;' }, 'B2'),
                h('td', { style: 'border: 1px solid #9C27B0; padding: 10px;' }, 'C2')
            )
        )
    );
}

// ========== Phase 4: SVG 支持测试 ==========

function SVGTagsTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #E91E63;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #E91E63;' }, '🎨 Phase 4: SVG 支持'),
        h('div', { style: 'display: flex; flex-wrap: wrap; gap: 20px;' },
            // 基础形状
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'circle 圆形'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('circle', { cx: 50, cy: 50, r: 40, fill: '#E91E63', stroke: '#C2185B', 'stroke-width': 3 })
                )
            ),
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'rect 矩形'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('rect', { x: 10, y: 10, width: 80, height: 80, rx: 10, ry: 10, fill: '#2196F3', stroke: '#1565C0', 'stroke-width': 3 })
                )
            ),
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'ellipse 椭圆'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('ellipse', { cx: 50, cy: 50, rx: 45, ry: 30, fill: '#4CAF50', stroke: '#2E7D32', 'stroke-width': 3 })
                )
            ),
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'line 直线'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('line', { x1: 10, y1: 10, x2: 90, y2: 90, stroke: '#FF9800', 'stroke-width': 4 }),
                    h('line', { x1: 90, y1: 10, x2: 10, y2: 90, stroke: '#FF5722', 'stroke-width': 4 })
                )
            ),
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'polygon 多边形'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('polygon', { points: '50,10 90,90 10,90', fill: '#9C27B0', stroke: '#6A1B9A', 'stroke-width': 3 })
                )
            ),
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'polyline 折线'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('polyline', { points: '10,80 30,20 50,60 70,30 90,70', fill: 'none', stroke: '#00BCD4', 'stroke-width': 3 })
                )
            ),
            // path 路径
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'path 路径 (心形)'),
                h('svg', { width: 100, height: 100, viewBox: '0 0 100 100', style: 'background: #f5f5f5;' },
                    h('path', { 
                        d: 'M50,30 C30,10 10,30 10,50 C10,70 30,90 50,95 C70,90 90,70 90,50 C90,30 70,10 50,30 Z',
                        fill: '#F44336', stroke: '#D32F2F', 'stroke-width': 2 
                    })
                )
            ),
            // g 分组
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'g 分组 + transform'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('g', { transform: 'translate(50,50)' },
                        h('rect', { x: -20, y: -20, width: 40, height: 40, fill: '#3F51B5' }),
                        h('circle', { cx: 0, cy: 0, r: 10, fill: '#FFC107' })
                    )
                )
            ),
            // text 文本
            h('div', { style: 'text-align: center;' },
                h('h4', { style: 'margin: 0 0 10px 0; color: #666;' }, 'text 文本'),
                h('svg', { width: 100, height: 100, style: 'background: #f5f5f5;' },
                    h('text', { x: 50, y: 55, fill: '#333', 'font-size': 16, 'text-anchor': 'middle' }, 'SVG')
                )
            )
        )
    );
}

// ========== Phase 5: 表单增强标签测试 ==========

function FormEnhancedTagsTest() {
    var state = useState(50);
    var progressValue = state[0];
    var setProgressValue = state[1];

    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #795548;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #795548;' }, '📝 Phase 5: 表单增强标签'),
        // fieldset + legend
        h('fieldset', { style: 'border: 2px solid #795548; border-radius: 8px; padding: 15px; margin: 10px 0;' },
            h('legend', { style: 'padding: 0 10px; color: #795548; font-weight: bold;' }, 'fieldset + legend 用户信息'),
            h('div', { style: 'display: flex; gap: 20px;' },
                h('label', {},
                    '用户名: ',
                    h('input', { type: 'text', placeholder: '请输入用户名', style: 'padding: 5px;' })
                ),
                h('label', {},
                    '密码: ',
                    h('input', { type: 'password', placeholder: '请输入密码', style: 'padding: 5px;' })
                )
            )
        ),

        // select + optgroup + option
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold;' }, 'select + optgroup: '),
            h('select', { style: 'padding: 8px; margin-left: 10px;' },
                h('optgroup', { label: '水果' },
                    h('option', { value: 'apple' }, '苹果'),
                    h('option', { value: 'banana' }, '香蕉'),
                    h('option', { value: 'orange' }, '橙子')
                ),
                h('optgroup', { label: '蔬菜' },
                    h('option', { value: 'carrot' }, '胡萝卜'),
                    h('option', { value: 'tomato' }, '番茄')
                )
            )
        ),

        // datalist
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold;' }, 'datalist: '),
            h('input', { list: 'browsers', placeholder: '选择浏览器', style: 'padding: 8px; margin-left: 10px;' }),
            h('datalist', { id: 'browsers' },
                h('option', { value: 'Chrome' }),
                h('option', { value: 'Firefox' }),
                h('option', { value: 'Safari' }),
                h('option', { value: 'Edge' })
            )
        ),

        // output
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold;' }, 'output: '),
            h('span', {}, ' 10 + 20 = '),
            h('output', { style: 'font-weight: bold; color: #4CAF50;' }, '30')
        ),

        // progress
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' },
                'progress: ' + progressValue + '%'
            ),
            h('progress', { value: progressValue, max: 100, style: 'width: 200px; height: 20px;' }),
            h('button', {
                onClick: function() { setProgressValue(Math.min(100, progressValue + 10)); },
                style: 'margin-left: 10px; padding: 5px 10px;'
            }, '+10'),
            h('button', {
                onClick: function() { setProgressValue(Math.max(0, progressValue - 10)); },
                style: 'margin-left: 5px; padding: 5px 10px;'
            }, '-10')
        ),

        // meter
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold;' }, 'meter: '),
            h('meter', { value: 0.7, min: 0, max: 1, low: 0.3, high: 0.7, optimum: 0.8, style: 'width: 200px; margin-left: 10px;' }),
            h('span', { style: 'margin-left: 10px; color: #666;' }, '70%')
        ),

        // dialog (模拟)
        h('div', { style: 'margin: 15px 0;' },
            h('label', { style: 'font-weight: bold;' }, 'dialog (示意): '),
            h('dialog', { open: true, style: 'border: 2px solid #795548; border-radius: 8px; padding: 15px; margin: 10px 0; background: #fff;' },
                h('p', { style: 'margin: 0 0 10px 0;' }, '这是一个 dialog 对话框'),
                h('button', { style: 'padding: 5px 15px;' }, '确定')
            )
        )
    );
}

// ========== 原有表单元素测试 ==========

function BasicFormTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #607D8B;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #607D8B;' }, '📋 基础表单元素'),
        h('div', { style: 'display: flex; flex-wrap: wrap; gap: 20px;' },
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'input text:'),
                h('input', { type: 'text', placeholder: '文本输入', style: 'padding: 8px;' })
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'input number:'),
                h('input', { type: 'number', value: 42, style: 'padding: 8px;' })
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'input checkbox:'),
                h('input', { type: 'checkbox', checked: true })
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'input radio:'),
                h('input', { type: 'radio', name: 'test' }), ' A ',
                h('input', { type: 'radio', name: 'test', checked: true }), ' B'
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'textarea:'),
                h('textarea', { rows: 3, cols: 20, style: 'padding: 8px;' }, '多行文本')
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'button:'),
                h('button', { style: 'padding: 8px 16px; background: #607D8B; color: white; border: none; border-radius: 4px;' }, '点击按钮')
            ),
            h('div', {},
                h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'select:'),
                h('select', { style: 'padding: 8px;' },
                    h('option', {}, '选项 1'),
                    h('option', {}, '选项 2'),
                    h('option', {}, '选项 3')
                )
            )
        )
    );
}

// ========== 其他元素测试 ==========

function OtherElementsTest() {
    var sectionStyle = 'margin: 20px 0; padding: 15px; background: #fff; border-radius: 8px; border-left: 4px solid #00BCD4;';
    return h('div', { style: sectionStyle },
        h('h2', { style: 'margin: 0 0 15px 0; color: #00BCD4;' }, '🔗 其他元素'),
        h('p', {},
            h('a', { href: '#', style: 'color: #00BCD4;' }, 'a 链接'), ' | ',
            h('img', { src: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="50" height="20"><text y="15" fill="red">IMG</text></svg>', alt: '图片', style: 'vertical-align: middle;' }), ' (img) | ',
            h('span', { style: 'background: #e0f7fa; padding: 2px 8px;' }, 'span 行内')
        ),

        // details + summary
        h('details', { style: 'margin: 10px 0; padding: 10px; border: 1px solid #00BCD4; border-radius: 4px;' },
            h('summary', { style: 'cursor: pointer; font-weight: bold;' }, 'details + summary (点击展开)'),
            h('p', { style: 'margin: 10px 0 0 0;' }, '这是隐藏的详细内容，点击上方可展开/收起。')
        ),

        // ruby 注音
        h('p', { style: 'font-size: 24px; margin: 10px 0;' },
            'ruby 注音: ',
            h('ruby', {},
                '漢', h('rp', {}, '('), h('rt', {}, 'かん'), h('rp', {}, ')'),
                '字', h('rp', {}, '('), h('rt', {}, 'じ'), h('rp', {}, ')')
            )
        ),

        // bdo 文字方向
        h('p', { style: 'margin: 10px 0;' },
            'bdo 文字方向: ',
            h('bdo', { dir: 'rtl' }, 'This text is reversed')
        ),

        // ins + del
        h('p', { style: 'margin: 10px 0;' },
            '修订: ',
            h('del', { style: 'color: red;' }, '删除的内容'),
            ' → ',
            h('ins', { style: 'color: green;' }, '新增的内容')
        )
    );
}

// ========== 主应用组件 ==========

function App() {
    return h('div', { style: 'padding: 0; margin: 0; background-color: #e0e0e0; min-height: 100vh;' },
        // 头部
        h('div', { style: 'background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; text-align: center;' },
            h('h1', { style: 'margin: 0; font-size: 32px;' }, '🏷️ HTML 标签完整测试'),
            h('p', { style: 'margin: 10px 0 0 0; opacity: 0.9;' }, 'MBink + Preact - 测试所有已实现的 HTML 标签')
        ),

        // 主内容
        h('div', { style: 'max-width: 1000px; margin: 0 auto; padding: 20px;' },
            h(SemanticTagsTest),
            h(ListTagsTest),
            h(TableTagsTest),
            h(SVGTagsTest),
            h(FormEnhancedTagsTest),
            h(BasicFormTest),
            h(OtherElementsTest),

            // 页脚
            h('div', { style: 'margin-top: 30px; padding: 20px; background: #fff; border-radius: 8px; text-align: center;' },
                h('p', { style: 'margin: 0; color: #666;' }, '🚀 Powered by MBink + Preact + QuickJS + Skia'),
                h('p', { style: 'margin: 5px 0 0 0; color: #999; font-size: 14px;' },
                    '✅ Phase 1-5 全部完成 | 语义化标签 + 列表 + 表格 + SVG + 表单增强'
                )
            )
        )
    );
}

// ========== 渲染应用 ==========

console.log('Starting HTML Tags Test App...');
render(h(App), document.body);
console.log('HTML Tags Test App rendered!');

