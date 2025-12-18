/**
 * @file test_incremental_layout.js
 * @brief 增量布局优化渲染测试
 *
 * 使用方法: app_loader.exe examples/preact_demo/test_incremental_layout.js
 *
 * 测试场景:
 * 1. 时钟更新 - 验证只有文本节点被重新布局
 * 2. 列表项添加 - 验证新项触发布局，现有项使用缓存
 * 3. 固定尺寸容器 - 验证内部变化不影响外部布局
 * 4. 样式变化 - 验证布局相关样式 vs 纯绘制样式
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;
    var useEffect = PreactHooks.useEffect;

    // ========== 样式定义 ==========
    var styles = {
        app: {
            fontFamily: 'Arial, sans-serif',
            padding: '20px',
            maxWidth: '800px',
            margin: '0 auto'
        },
        card: {
            backgroundColor: '#f5f5f5',
            borderRadius: '8px',
            padding: '16px',
            marginBottom: '16px'
        },
        button: {
            backgroundColor: '#007bff',
            color: 'white',
            border: 'none',
            padding: '8px 16px',
            borderRadius: '4px',
            cursor: 'pointer',
            marginRight: '8px',
            marginBottom: '8px'
        },
        fixedContainer: {
            width: '300px',
            height: '150px',
            backgroundColor: '#e0e0e0',
            borderRadius: '4px',
            padding: '10px',
            overflow: 'hidden'
        },
        autoContainer: {
            backgroundColor: '#d0d0d0',
            borderRadius: '4px',
            padding: '10px'
        },
        statsBox: {
            backgroundColor: '#333',
            color: '#0f0',
            fontFamily: 'monospace',
            padding: '10px',
            borderRadius: '4px',
            marginTop: '10px',
            fontSize: '12px'
        }
    };

    function formatTime(date) {
        var hours = date.getHours();
        var minutes = date.getMinutes();
        var seconds = date.getSeconds();
        var ms = date.getMilliseconds();
        var hh = hours < 10 ? '0' + hours : '' + hours;
        var mm = minutes < 10 ? '0' + minutes : '' + minutes;
        var ss = seconds < 10 ? '0' + seconds : '' + seconds;
        var msStr = ms < 10 ? '00' + ms : (ms < 100 ? '0' + ms : '' + ms);
        return hh + ':' + mm + ':' + ss + '.' + msStr;
    }

    // ========== 测试1: 高频时钟更新 ==========
    function ClockTest() {
        var timeState = useState(formatTime(new Date()));
        var time = timeState[0];
        var setTime = timeState[1];

        var updateCountState = useState(0);
        var updateCount = updateCountState[0];
        var setUpdateCount = updateCountState[1];

        useEffect(function () {
            var timer = setInterval(function () {
                setTime(formatTime(new Date()));
                setUpdateCount(function (c) { return c + 1; });
            }, 100); // 100ms 更新一次，测试高频更新
            return function () { clearInterval(timer); };
        }, []);

        return h('div', { style: styles.card },
            h('h3', null, '测试1: 高频时钟更新'),
            h('p', { style: { color: '#666', fontSize: '12px' } },
                '验证: 只有时钟文本节点被重新布局，周围元素使用缓存'),
            h('div', { style: { display: 'flex', alignItems: 'center' } },
                h('div', { style: { marginRight: '20px' } },
                    h('span', { style: { color: '#999' } }, '静态标签: '),
                    h('span', null, '当前时间')
                ),
                h('div', {
                    style: {
                        fontSize: '32px',
                        fontFamily: 'monospace',
                        color: '#28a745',
                        backgroundColor: '#fff',
                        padding: '10px 20px',
                        borderRadius: '4px'
                    }
                }, time)
            ),
            h('div', { style: styles.statsBox },
                '更新次数: ' + updateCount + ' | 更新频率: 100ms'
            )
        );
    }

    // ========== 测试2: 动态列表 ==========
    function ListTest() {
        var itemsState = useState([
            { id: 1, text: '初始项目 1' },
            { id: 2, text: '初始项目 2' },
            { id: 3, text: '初始项目 3' }
        ]);
        var items = itemsState[0];
        var setItems = itemsState[1];

        var nextIdState = useState(4);
        var nextId = nextIdState[0];
        var setNextId = nextIdState[1];

        function addItem() {
            setItems(items.concat([{ id: nextId, text: '新增项目 ' + nextId }]));
            setNextId(nextId + 1);
        }

        function removeItem(id) {
            setItems(items.filter(function (item) { return item.id !== id; }));
        }

        return h('div', { style: styles.card },
            h('h3', null, '测试2: 动态列表'),
            h('p', { style: { color: '#666', fontSize: '12px' } },
                '验证: 添加新项时，现有项使用缓存'),
            h('button', { style: styles.button, onClick: addItem }, '添加项目'),
            h('div', { style: { marginTop: '10px' } },
                items.map(function (item) {
                    return h('div', {
                        key: item.id,
                        style: {
                            display: 'flex',
                            alignItems: 'center',
                            padding: '8px',
                            backgroundColor: '#fff',
                            marginBottom: '4px',
                            borderRadius: '4px'
                        }
                    },
                        h('span', { style: { flex: '1' } }, item.text),
                        h('button', {
                            style: {
                                backgroundColor: '#dc3545',
                                color: 'white',
                                border: 'none',
                                padding: '4px 8px',
                                borderRadius: '4px',
                                cursor: 'pointer'
                            },
                            onClick: function () { removeItem(item.id); }
                        }, '删除')
                    );
                })
            ),
            h('div', { style: styles.statsBox },
                '当前项目数: ' + items.length
            )
        );
    }

    // ========== 测试3: 固定尺寸容器隔离 ==========
    function FixedContainerTest() {
        var contentState = useState('初始内容');
        var content = contentState[0];
        var setContent = contentState[1];

        var countState = useState(0);
        var count = countState[0];
        var setCount = countState[1];

        function changeContent() {
            var contents = [
                '短文本',
                '这是一段较长的文本内容，用于测试固定容器',
                '中等长度的内容',
                '非常非常非常非常非常非常非常非常长的文本内容'
            ];
            setCount(count + 1);
            setContent(contents[count % contents.length]);
        }

        return h('div', { style: styles.card },
            h('h3', null, '测试3: 固定尺寸容器隔离'),
            h('p', { style: { color: '#666', fontSize: '12px' } },
                '验证: 固定尺寸容器内部变化不影响外部布局'),
            h('div', { style: { display: 'flex' } },
                h('div', { style: styles.fixedContainer },
                    h('div', { style: { fontWeight: 'bold', marginBottom: '10px' } },
                        '固定容器 (300x150)'),
                    h('div', { style: { color: '#333' } }, content)
                ),
                h('div', { style: { marginLeft: '20px' } },
                    h('div', { style: { marginBottom: '10px' } }, '外部元素 (不应重新布局)'),
                    h('button', { style: styles.button, onClick: changeContent },
                        '改变内部内容')
                )
            ),
            h('div', { style: styles.statsBox },
                '内容变化次数: ' + count
            )
        );
    }

    // ========== 测试4: Auto 尺寸容器传播 ==========
    function AutoContainerTest() {
        var textState = useState('初始');
        var text = textState[0];
        var setText = textState[1];

        var countState = useState(0);
        var count = countState[0];
        var setCount = countState[1];

        function changeText() {
            var texts = ['短', '中等长度', '这是一段很长很长的文本', '初始'];
            setCount(count + 1);
            setText(texts[count % texts.length]);
        }

        return h('div', { style: styles.card },
            h('h3', null, '测试4: Auto 尺寸容器传播'),
            h('p', { style: { color: '#666', fontSize: '12px' } },
                '验证: Auto 尺寸容器内容变化会向上传播'),
            h('div', { style: { display: 'flex', alignItems: 'flex-start' } },
                h('div', { style: styles.autoContainer },
                    h('div', { style: { fontWeight: 'bold', marginBottom: '10px' } },
                        'Auto 容器'),
                    h('div', {
                        style: {
                            backgroundColor: '#fff',
                            padding: '10px',
                            borderRadius: '4px'
                        }
                    }, text)
                ),
                h('div', { style: { marginLeft: '20px' } },
                    h('div', { style: { marginBottom: '10px' } }, '相邻元素 (可能重新布局)'),
                    h('button', { style: styles.button, onClick: changeText },
                        '改变内容')
                )
            ),
            h('div', { style: styles.statsBox },
                '内容变化次数: ' + count
            )
        );
    }

    // ========== 测试5: 纯绘制样式变化 ==========
    function PaintOnlyTest() {
        var colorState = useState('#007bff');
        var color = colorState[0];
        var setColor = colorState[1];

        var countState = useState(0);
        var count = countState[0];
        var setCount = countState[1];

        var colors = ['#007bff', '#28a745', '#dc3545', '#ffc107', '#17a2b8', '#6f42c1'];

        function changeColor() {
            setCount(count + 1);
            setColor(colors[count % colors.length]);
        }

        return h('div', { style: styles.card },
            h('h3', null, '测试5: 纯绘制样式变化'),
            h('p', { style: { color: '#666', fontSize: '12px' } },
                '验证: 颜色变化不触发布局，只触发重绘'),
            h('div', {
                style: {
                    width: '200px',
                    height: '100px',
                    backgroundColor: color,
                    borderRadius: '8px',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    color: 'white',
                    fontWeight: 'bold',
                    marginBottom: '10px'
                }
            }, '颜色块'),
            h('button', { style: styles.button, onClick: changeColor }, '切换颜色'),
            h('div', { style: styles.statsBox },
                '颜色变化次数: ' + count + ' | 当前颜色: ' + color
            )
        );
    }

    // ========== 主应用 ==========
    function App() {
        return h('div', { style: styles.app },
            h('h1', { style: { textAlign: 'center', color: '#333' } },
                '🔧 增量布局优化测试'),
            h('p', { style: { textAlign: 'center', color: '#666', marginBottom: '24px' } },
                '测试 ContentVersion、LayoutScope 和增量布局缓存机制'),
            h(ClockTest, null),
            h(ListTest, null),
            h(FixedContainerTest, null),
            h(AutoContainerTest, null),
            h(PaintOnlyTest, null),
            h('div', {
                style: {
                    textAlign: 'center',
                    marginTop: '20px',
                    padding: '12px',
                    backgroundColor: '#e9ecef',
                    borderRadius: '4px',
                    color: '#666'
                }
            }, '增量布局优化测试 - Phase 5 验证')
        );
    }

    // ========== 渲染 ==========
    console.log('Starting Incremental Layout Test...');
    render(h(App, null), document.body);
    console.log('Incremental Layout Test rendered!');
})();
