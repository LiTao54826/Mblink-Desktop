/**
 * @file components.js
 * @brief 通用 UI 组件库
 */
import { h } from 'preact';
import { useState, useRef, useEffect, useMemo, useCallback } from 'preact/hooks';
import { colors, spacing, radius, shadows, fonts } from './theme.js';

// ========== Badge 徽章 ==========
export function Badge({ text, color, bg }) {
    return h('div', {
        style: {
            display: 'inline-flex', alignItems: 'center', justifyContent: 'center',
            padding: '2px 8px', borderRadius: '12px', fontSize: fonts.sizes.xs,
            fontWeight: '600', color: color || '#fff',
            backgroundColor: bg || colors.primary, whiteSpace: 'nowrap'
        }
    }, text);
}

// ========== Avatar 头像 ==========
export function Avatar({ name, size, color, online }) {
    const s = size || 36;
    const initials = (name || '?').split(' ').map(w => w[0]).join('').substring(0, 2).toUpperCase();
    return h('div', {
        style: { position: 'relative', width: s + 'px', height: s + 'px', flexShrink: 0 }
    },
        h('div', {
            style: {
                width: '100%', height: '100%', borderRadius: radius.full,
                background: color || colors.gradPrimary,
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                color: '#fff', fontSize: (s * 0.38) + 'px', fontWeight: '700'
            }
        }, initials),
        online !== undefined ? h('div', {
            style: {
                position: 'absolute', bottom: '0', right: '0',
                width: (s * 0.28) + 'px', height: (s * 0.28) + 'px',
                borderRadius: radius.full, border: '2px solid ' + colors.bgCard,
                backgroundColor: online ? colors.success : colors.textMuted
            }
        }) : null
    );
}

// ========== ProgressBar 进度条 ==========
export function ProgressBar({ value, max, gradient, height }) {
    const pct = Math.min(100, Math.max(0, (value / (max || 100)) * 100));
    return h('div', {
        style: {
            width: '100%', height: height || '6px', backgroundColor: colors.surface2,
            borderRadius: '3px', overflow: 'hidden'
        }
    },
        h('div', {
            style: {
                width: pct + '%', height: '100%',
                background: gradient || colors.gradPrimary,
                borderRadius: '3px', transition: 'width 0.3s ease'
            }
        })
    );
}

// ========== Switch 开关 ==========
export function Switch({ checked, onChange, label }) {
    return h('div', {
        style: { display: 'flex', alignItems: 'center', gap: spacing.md, cursor: 'pointer' },
        onClick: function () { if (onChange) onChange(!checked); }
    },
        label ? h('span', { style: { color: colors.text, fontSize: fonts.sizes.md } }, label) : null,
        h('div', {
            style: {
                width: '44px', height: '24px', borderRadius: '12px', padding: '2px',
                backgroundColor: checked ? colors.primary : colors.surface3,
                display: 'flex', alignItems: checked ? 'center' : 'center',
                justifyContent: checked ? 'flex-end' : 'flex-start',
                transition: 'background-color 0.2s ease', flexShrink: 0
            }
        },
            h('div', {
                style: {
                    width: '20px', height: '20px', borderRadius: radius.full,
                    backgroundColor: '#fff', boxShadow: shadows.sm,
                    transition: 'transform 0.2s ease'
                }
            })
        )
    );
}

// ===== 卡片 ==========
export function Card({ children, style: extraStyle, onClick }) {
    return h('div', {
        style: Object.assign({
            backgroundColor: colors.bgCard, borderRadius: radius.md,
            border: '1px solid ' + colors.border, padding: spacing.lg,
            boxShadow: shadows.sm
        }, extraStyle || {}),
        onClick: onClick
    }, children);
}

// ========== StatCard 统计卡片 ==========
export function StatCard({ icon, label, value, trend, gradient }) {
    return h('div', {
        style: {
            background: gradient || colors.gradPrimary, borderRadius: radius.lg,
            padding: spacing.xl, color: '#fff', position: 'relative', overflow: 'hidden',
            minWidth: '180px', flex: 1, boxShadow: shadows.md
        }
    },
        // 装饰圆
        h('div', {
            style: {
                position: 'absolute', top: '-20px', right: '-20px',
                width: '80px', height: '80px', borderRadius: radius.full,
                backgroundColor: 'rgba(255,255,255,0.1)'
            }
        }),
        h('div', { style: { fontSize: '24px', marginBottom: spacing.sm } }, icon),
        h('div', { style: { fontSize: fonts.sizes.hero, Weight: '800', marginBottom: '4px' } }, value),
        h('div', { style: { display: 'flex', alignItems: 'center', gap: spacing.xs } },
            h('span', { style: { fontSize: fonts.sizes.sm, opacity: 0.85 } }, label),
            trend ? h('span', {
                style: {
                    fontSize: fonts.sizes.xs, padding: '1px 6px', borderRadius: '8px',
                    backgroundColor: 'rgba(255,255,255,0.2)', marginLeft: spacing.xs
                }
            }, trend) : null
        )
    );
}

// ========== IconBtn 图标按钮 ==========
export function IconBtn({ icon, onClick, title, active }) {
    return h('div', {
        style: {
            width: '32px', height: '32px', borderRadius: radius.sm,
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            cursor: 'pointer', fontSize: '16px',
            backgroundColor: active ? colors.bgActive : 'transparent',
            color: active ? colors.primary : colors.textSec
        },
        onClick: onClick, title: title
    }, icon);
}

// ========== Toast 通知组件 ==========
export function ToastContainer({ toasts }) {
    if (!toasts || toasts.length === 0) return null;
    const typeColors = { success: colors.success, error: colors.danger, info: colors.info, warning: colors.warning };
    return h('div', {
        style: {
            position: 'absolute', top: '52px', right: spacing.lg,
            display: 'flex', flexDirection: 'column', gap: spacing.sm,
            zIndex: 1000, pointerEvents: 'none'
        }
    }, toasts.map(function (t, i) {
        const tc = typeColors[t.type] || colors.primary;
        return h('div', {
            key: t.id || i,
            style: {
                padding: spacing.md + ' ' + spacing.lg, borderRadius: radius.md,
                backgroundColor: colors.surface2, border: '1px solid ' + tc,
                borderLeft: '3px solid ' + tc,
                color: colors.text, fontSize: fonts.sizes.sm, boxShadow: shadows.md,
                minWidth: '240px', pointerEvents: 'auto'
            }
        }, t.message);
    }));
}

