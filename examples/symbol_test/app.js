/**
 * @file app.js
 * @brief 符号字体测试示例
 * 
 * 测试各种 Unicode 符号的渲染支持
 */

import { h, render } from 'preact';

// 样式定义
const styles = {
    app: {
        fontFamily: 'Arial, sans-serif',
        padding: '20px',
        backgroundColor: '#f5f5f5',
        minHeight: '100vh'
    },
    title: {
        textAlign: 'center',
        color: '#333',
        marginBottom: '24px'
    },
    section: {
        backgroundColor: 'white',
        borderRadius: '8px',
        padding: '16px',
        marginBottom: '16px',
        boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
    },
    sectionTitle: {
        fontSize: '16px',
        fontWeight: 'bold',
        color: '#1890ff',
        marginBottom: '12px',
        borderBottom: '1px solid #e0e0e0',
        paddingBottom: '8px'
    },
    symbolGrid: {
        display: 'flex',
        flexWrap: 'wrap',
        gap: '8px'
    },
    symbolItem: {
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        padding: '8px',
        backgroundColor: '#f9f9f9',
        borderRadius: '4px',
        minWidth: '60px'
    },
    symbol: {
        fontSize: '24px',
        marginBottom: '4px'
    },
    code: {
        fontSize: '10px',
        color: '#666'
    }
};

// 符号数据
const symbolCategories = [
    {
        name: '八卦符号 (Trigrams)',
        symbols: [
            { char: '☰', code: 'U+2630' },
            { char: '☱', code: 'U+2631' },
            { char: '☲', code: 'U+2632' },
            { char: '☳', code: 'U+2633' },
            { char: '☴', code: 'U+2634' },
            { char: '☵', code: 'U+2635' },
            { char: '☶', code: 'U+2636' },
            { char: '☷', code: 'U+2637' }
        ]
    },
    {
        name: '箭头 (Arrows)',
        symbols: [
            { char: '←', code: 'U+2190' },
            { char: '↑', code: 'U+2191' },
            { char: '→', code: 'U+2192' },
            { char: '↓', code: 'U+2193' },
            { char: '↔', code: 'U+2194' },
            { char: '↕', code: 'U+2195' },
            { char: '⇐', code: 'U+21D0' },
            { char: '⇒', code: 'U+21D2' }
        ]
    },
    {
        name: '数学符号 (Math)',
        symbols: [
            { char: '∑', code: 'U+2211' },
            { char: '∏', code: 'U+220F' },
            { char: '∫', code: 'U+222B' },
            { char: '√', code: 'U+221A' },
            { char: '∞', code: 'U+221E' },
            { char: '≈', code: 'U+2248' },
            { char: '≠', code: 'U+2260' },
            { char: '≤', code: 'U+2264' }
        ]
    },
    {
        name: '几何图形 (Geometric)',
        symbols: [
            { char: '■', code: 'U+25A0' },
            { char: '□', code: 'U+25A1' },
            { char: '▲', code: 'U+25B2' },
            { char: '△', code: 'U+25B3' },
            { char: '●', code: 'U+25CF' },
            { char: '○', code: 'U+25CB' },
            { char: '◆', code: 'U+25C6' },
            { char: '◇', code: 'U+25C7' }
        ]
    },
    {
        name: '盒子绘制 (Box Drawing)',
        symbols: [
            { char: '─', code: 'U+2500' },
            { char: '│', code: 'U+2502' },
            { char: '┌', code: 'U+250C' },
            { char: '┐', code: 'U+2510' },
            { char: '└', code: 'U+2514' },
            { char: '┘', code: 'U+2518' },
            { char: '├', code: 'U+251C' },
            { char: '┤', code: 'U+2524' }
        ]
    },
    {
        name: '货币符号 (Currency)',
        symbols: [
            { char: '€', code: 'U+20AC' },
            { char: '£', code: 'U+00A3' },
            { char: '¥', code: 'U+00A5' },
            { char: '₹', code: 'U+20B9' },
            { char: '₽', code: 'U+20BD' },
            { char: '₿', code: 'U+20BF' }
        ]
    },
    {
        name: 'Emoji 表情',
        symbols: [
            { char: '😀', code: 'U+1F600' },
            { char: '🎉', code: 'U+1F389' },
            { char: '🚀', code: 'U+1F680' },
            { char: '❤', code: 'U+2764' },
            { char: '✓', code: 'U+2713' },
            { char: '⭐', code: 'U+2B50' }
        ]
    }
];

// 符号项组件
function SymbolItem({ char, code }) {
    return h('div', { style: styles.symbolItem }, [
        h('span', { style: styles.symbol }, char),
        h('span', { style: styles.code }, code)
    ]);
}

// 符号分类组件
function SymbolSection({ name, symbols }) {
    return h('div', { style: styles.section }, [
        h('div', { style: styles.sectionTitle }, name),
        h('div', { style: styles.symbolGrid },
            symbols.map(s => h(SymbolItem, { key: s.code, char: s.char, code: s.code }))
        )
    ]);
}

// 主应用组件
function App() {
    return h('div', { style: styles.app }, [
        h('h1', { style: styles.title }, '符号字体测试'),
        ...symbolCategories.map(cat =>
            h(SymbolSection, { key: cat.name, name: cat.name, symbols: cat.symbols })
        )
    ]);
}

// 渲染应用
console.log('Starting Symbol Test...');
render(h(App), document.body);
console.log('Symbol Test rendered!');
