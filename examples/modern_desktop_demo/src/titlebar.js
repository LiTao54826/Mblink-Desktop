/**
 * @file titlebar.js
 * @brief 自定义无边框标题栏 - macOS 风格
 */
import { h } from 'preact';
import { colors, spacing, fonts, shadows } from './theme.js';

// MBlink Logo (内联 SVG — 引擎不支持 img src 加载 SVG)
function Logo({ size }) {
    var s = size || 22;
    return h('svg', { xmlns: 'http://www.w3.org/2000/svg', viewBox: '0 0 100 100', width: s, height: s },
        h('defs', null,
            h('linearGradient', { id: 'digitalInk', x1: '0%', y1: '100%', x2: '100%', y2: '0%' },
                h('stop', { offset: '0%', style: 'stop-color:#0052D4;stop-opacity:1' }),
                h('stop', { offset: '50%', style: 'stop-color:#4364F7;stop-opacity:1' }),
                h('stop', { offset: '100%', style: 'stop-color:#6FB1FC;stop-opacity:1' })
            )
        ),
        h('path', { d: 'M25 85 Q 10 50 25 20 Q 45 40 50 55 L 50 85 Z', fill: 'url(#digitalInk)' }),
        h('rect', { x: '60', y: '20', width: '20', height: '20', rx: '2', ry: '2', fill: '#0052D4', opacity: '0.9' }),
        h('rect', { x: '60', y: '45', width: '20', height: '40', rx: '2', ry: '2', fill: '#4364F7', opacity: '0.8' }),
        h('path', { d: 'M 50 55 L 60 45 L 60 55 L 50 65 Z', fill: '#6FB1FC', opacity: '0.6' }),
        h('circle', { cx: '85', cy: '15', r: '3', fill: '#6FB1FC' })
    );
}

// 窗口控制按钮（红绿灯）
function TrafficLight({ color, control }) {
    return h('div', {
        style: {
            width: '13px', height: '13px', borderRadius: '50%',
            backgroundColor: color, cursor: 'pointer',
            '-webkit-window-control': control
        }
    });
}

export function TitleBar({ title }) {
    return h('div', {
        style: {
            height: '40px', display: 'flex', alignItems: 'center',
            padding: '0 ' + spacing.lg, backgroundColor: colors.bgSidebar,
            borderBottom: '1px solid ' + colors.border,
            '-webkit-app-region': 'drag', userSelect: 'none'
        }
    },
        // 红绿灯按钮区域（不可拖拽）
        h('div', {
            style: {
                display: 'flex', gap: '8px', alignItems: 'center',
                '-webkit-app-region': 'no-drag', marginRight: spacing.lg
            }
        },
            h(TrafficLight, { color: '#ff5f57', control: 'close' }),
            h(TrafficLight, { color: '#febc2e', control: 'minimize' }),
            h(TrafficLight, { color: '#28c840', control: 'maximize' })
        ),
        // Logo + 标题
        h('div', {
            style: {
                flex: 1, display: 'flex', alignItems: 'center',
                justifyContent: 'center', gap: '8px'
            }
        },
            h(Logo, { size: 22 }),
            h('div', {
                style: {
                    color: colors.textSec,
                    fontSize: fonts.sizes.sm, fontWeight: '500', letterSpacing: '0.5px'
                }
            }, title || 'Modern Desktop App')
        ),
        // 右侧占位（保持标题居中）
        h('div', { style: { width: '70px' } })
    );
}

