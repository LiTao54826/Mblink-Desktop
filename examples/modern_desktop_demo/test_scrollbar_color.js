/**
 * @file test_scrollbar_color.js
 * @brief CSS scrollbar-color 属性测试脚本
 * 
 * 测试场景：
 * 1. 默认滚动条颜色 (auto)
 * 2. 自定义 scrollbar-color: <thumb> <track>
 * 3. 暗色主题滚动条
 * 4. rgb()/rgba() 颜色函数
 */
import { h, render } from 'preact';
import { useState } from 'preact/hooks';

var presets = [
    { name: '默认 (auto)', thumb: null, track: null },
    { name: '暗色主题', thumb: '#555570', track: '#1a1a2e' },
    { name: '蓝色主题', thumb: '#4a90d9', track: '#1e2a3a' },
    { name: '绿色主题', thumb: '#4caf50', track: '#1b2e1b' },
    { name: '红色主题', thumb: '#e57373', track: '#2e1b1b' },
    { name: 'rgba 半透明', thumb: 'rgba(255,255,255,0.4)', track: 'rgba(0,0,0,0.2)' },
];

function ScrollBox({ scrollbarColor, label, children }) {
    var style = {
        width: '100%', height: '200px',
        overflowY: 'auto',
        border: '1px solid #444',
        borderRadius: '8px',
        padding: '12px',
        backgroundColor: '#1e1e2e',
        color: '#cdd6f4',
        fontSize: '14px',
        marginBottom: '16px',
    };
    if (scrollbarColor) {
        style['scrollbar-color'] = scrollbarColor;
    }
    return h('div', null,
        h('div', { style: { color: '#89b4fa', fontSize: '13px', marginBottom: '6px', fontWeight: 'bold' } }, label),
        h('div', { style: style }, children)
    );
}

function generateContent(count) {
    var items = [];
    for (var i = 1; i <= count; i++) {
        items.push(h('div', {
            key: i,
            style: {
                padding: '8px 12px',
                marginBottom: '4px',
                backgroundColor: i % 2 === 0 ? 'rgba(255,255,255,0.03)' : 'transparent',
                borderRadius: '4px'
            }
        }, '📝 Item #' + i + ' — scrollbar-color 测试内容行'));
    }
    return items;
}

function App() {
    var st = useState(1);
    var activePreset = st[0];
    var setActivePreset = st[1];

    return h('div', {
        style: {
            width: '100vw', height: '100vh',
            backgroundColor: '#11111b', color: '#cdd6f4',
            fontFamily: 'Microsoft YaHei, sans-serif',
            display: 'flex', flexDirection: 'column',
            padding: '20px', boxSizing: 'border-box',
            overflowY: 'auto',
            'scrollbar-color': '#555570 #11111b'
        }
    },
        h('h2', { style: { color: '#cba6f7', margin: '0 0 8px 0', fontSize: '20px' } },
            '🎨 CSS scrollbar-color 属性测试'),
        h('p', { style: { color: '#a6adc8', margin: '0 0 20px 0', fontSize: '13px' } },
            '标准格式: scrollbar-color: <thumb-color> <track-color>'),

        // 预设按钮
        h('div', { style: { display: 'flex', gap: '8px', marginBottom: '20px', flexWrap: 'wrap' } },
            presets.map(function (p, i) {
                return h('button', {
                    key: i,
                    onclick: function () { setActivePreset(i); },
                    style: {
                        padding: '6px 14px',
                        borderRadius: '6px',
                        border: activePreset === i ? '2px solid #cba6f7' : '1px solid #444',
                        backgroundColor: activePreset === i ? '#313244' : '#1e1e2e',
                        color: activePreset === i ? '#cba6f7' : '#a6adc8',
                        cursor: 'pointer', fontSize: '13px'
                    }
                }, p.name);
            })
        ),

        // 当前选中的预设测试
        (function () {
            var p = presets[activePreset];
            var colorVal = p.thumb ? (p.thumb + ' ' + p.track) : null;
            return h(ScrollBox, {
                scrollbarColor: colorVal,
                label: '当前: ' + p.name + (colorVal ? ' → scrollbar-color: ' + colorVal : ' → auto (引擎默认)')
            }, generateContent(30));
        })(),

        // 并排对比
        h('div', { style: { color: '#89b4fa', fontSize: '14px', fontWeight: 'bold', marginBottom: '8px' } },
            '📊 并排对比'),
        h('div', { style: { display: 'flex', gap: '16px' } },
            h('div', { style: { flex: 1 } },
                h(ScrollBox, { scrollbarColor: null, label: '默认 auto' }, generateContent(20))
            ),
            h('div', { style: { flex: 1 } },
                h(ScrollBox, { scrollbarColor: '#cba6f7 #313244', label: '紫色: #cba6f7 #313244' }, generateContent(20))
            ),
            h('div', { style: { flex: 1 } },
                h(ScrollBox, { scrollbarColor: '#f38ba8 #1e1e2e', label: '粉色: #f38ba8 #1e1e2e' }, generateContent(20))
            )
        )
    );
}

render(h(App, null), document.body);

