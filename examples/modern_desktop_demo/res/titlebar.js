/**
 * @file titlebar.js
 * @brief 自定义无边框标题栏 - macOS 风格
 */
import { h } from 'preact';
import { colors, spacing, fonts, shadows } from './theme.js';

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
        // 标题
        h('div', {
            style: {
                flex: 1, textAlign: 'center', color: colors.textSec,
                fontSize: fonts.sizes.sm, fontWeight: '500', letterSpacing: '0.5px'
            }
        }, title || 'Modern Desktop App'),
        // 右侧占位（保持标题居中）
        h('div', { style: { width: '70px' } })
    );
}

