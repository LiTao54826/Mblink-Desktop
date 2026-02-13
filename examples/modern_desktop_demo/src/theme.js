/**
 * @file theme.js
 * @brief 主题系统 - 暗色现代桌面主题
 */
import { createContext } from 'preact';
import { useContext } from 'preact/hooks';

// 暗色主题调色板
export const colors = {
    // 基础色
    bg:         '#0f0f13',
    bgCard:     '#1a1a24',
    bgSidebar:  '#13131a',
    bgHover:    '#22222e',
    bgActive:   '#2a2a3a',
    bgInput:    '#16161f',
    bgOverlay:  'rgba(0,0,0,0.6)',

    // 表面色
    surface1:   '#1e1e2a',
    surface2:   '#252535',
    surface3:   '#2c2c3e',

    // 文字色
    text:       '#e8e8f0',
    textSec:    '#8888a0',
    textMuted:  '#555570',
    textInv:    '#0f0f13',

    // 主色调 - 蓝紫渐变
    primary:    '#6c5ce7',
    primaryLt:  '#a29bfe',
    primaryDk:  '#5a4bd1',

    // 功能色
    success:    '#00cec9',
    successLt:  '#55efc4',
    warning:    '#fdcb6e',
    warningDk:  '#e17055',
    danger:     '#ff6b6b',
    dangerDk:   '#d63031',
    info:       '#74b9ff',

    // 边框
    border:     '#2a2a3a',
    borderLt:   '#353548',

    // 渐变
    gradPrimary:  'linear-gradient(135deg, #6c5ce7 0%, #a29bfe 100%)',
    gradSuccess:  'linear-gradient(135deg, #00cec9 0%, #55efc4 100%)',
    gradWarning:  'linear-gradient(135deg, #fdcb6e 0%, #e17055 100%)',
    gradDanger:   'linear-gradient(135deg, #ff6b6b 0%, #d63031 100%)',
    gradInfo:     'linear-gradient(135deg, #74b9ff 0%, #0984e3 100%)',
    gradDark:     'linear-gradient(135deg, #2d3436 0%, #636e72 100%)',
};

// 间距系统
export const spacing = {
    xs: '4px', sm: '8px', md: '12px', lg: '16px', xl: '24px', xxl: '32px'
};

// 圆角
export const radius = {
    sm: '6px', md: '10px', lg: '14px', xl: '20px', full: '50%'
};

// 阴影
export const shadows = {
    sm:   '0 2px 8px rgba(0,0,0,0.3)',
    md:   '0 4px 16px rgba(0,0,0,0.4)',
    lg:   '0 8px 32px rgba(0,0,0,0.5)',
    glow: '0 0 20px rgba(108,92,231,0.3)',
};

// 字体
export const fonts = {
    base: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    mono: '"Cascadia Code", "Fira Code", Consolas, monospace',
    sizes: { xs: '11px', sm: '12px', md: '14px', lg: '16px', xl: '20px', xxl: '28px', hero: '36px' }
};

// 通知 Context
export const ToastContext = createContext(null);

export function useToast() {
    return useContext(ToastContext);
}

