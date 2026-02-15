/**
 * @file showcase.js
 * @brief 能力展示页面 - CSS 伪类、动画、过渡、表单输入、视觉效果
 *
 * 展示 MBink 引擎的全部 CSS 能力：
 * 1. CSS @keyframes 动画（bounce, spin, pulse, fadeIn, colorChange）
 * 2. 伪类交互（:hover, :active, :focus, :disabled, :checked）
 * 3. 表单输入（input text, checkbox, textarea, button）
 * 4. CSS transition 过渡（hover 触发平滑变化）
 * 5. 视觉效果（transform, box-shadow, text-shadow, text-transform, opacity）
 */
import { h } from 'preact';
import { useState, useEffect, useCallback } from 'preact/hooks';
import { colors, spacing, radius, fonts, shadows } from './theme.js';
import { Card } from './components.js';

// ========== 注入 <style> 标签的 CSS 文本 ==========
var SHOWCASE_CSS = [
    // --- @keyframes 动画 ---
    '@keyframes showcase-bounce {',
    '  0%, 100% { transform: translateY(0px); }',
    '  50% { transform: translateY(-18px); }',
    '}',
    '@keyframes showcase-spin {',
    '  from { transform: rotate(0deg); }',
    '  to { transform: rotate(360deg); }',
    '}',
    '@keyframes showcase-pulse {',
    '  0%, 100% { transform: scale(1); }',
    '  50% { transform: scale(1.15); }',
    '}',
    '@keyframes showcase-fadeIn {',
    '  from { opacity: 0; }',
    '  to { opacity: 1; }',
    '}',
    '@keyframes showcase-colorShift {',
    '  0%   { background-color: #6c5ce7; }',
    '  33%  { background-color: #00cec9; }',
    '  66%  { background-color: #ff6b6b; }',
    '  100% { background-color: #6c5ce7; }',
    '}',
    '@keyframes showcase-slideIn {',
    '  0%   { transform: translateX(-40px); opacity: 0; }',
    '  100% { transform: translateX(0px); opacity: 1; }',
    '}',

    // --- 动画类 ---
    '.anim-bounce  { animation: showcase-bounce 1s ease-in-out infinite; }',
    '.anim-spin    { animation: showcase-spin 2s linear infinite; }',
    '.anim-pulse   { animation: showcase-pulse 1.2s ease-in-out infinite; }',
    '.anim-fadeIn  { animation: showcase-fadeIn 2s ease-in-out; }',
    '.anim-color   { animation: showcase-colorShift 3s linear infinite; }',
    '.anim-slideIn { animation: showcase-slideIn 0.8s ease-out; }',

    // --- 自定义 :hover 过渡效果（需要 <style> 标签） ---
    '.trans-card {',
    '  transition: all 0.3s ease;',
    '  background-color: #252535;',
    '  border: 1px solid #2a2a3a;',
    '  border-radius: 10px;',
    '  padding: 16px;',
    '}',
    '.trans-card:hover {',
    '  background-color: #2c2c3e;',
    '  border-color: #6c5ce7;',
    '  box-shadow: 0 0 20px rgba(108,92,231,0.3);',
    '  transform: translateY(-4px);',
    '}',
    '.trans-grow {',
    '  transition: transform 0.3s ease, box-shadow 0.3s ease;',
    '}',
    '.trans-grow:hover {',
    '  transform: scale(1.08);',
    '  box-shadow: 0 8px 24px rgba(0,0,0,0.5);',
    '}',
    '.trans-glow {',
    '  transition: box-shadow 0.4s ease, border-color 0.4s ease;',
    '  border: 2px solid #2a2a3a;',
    '}',
    '.trans-glow:hover {',
    '  border-color: #00cec9;',
    '  box-shadow: 0 0 16px rgba(0,206,201,0.4), 0 0 32px rgba(0,206,201,0.2);',
    '}',
].join('\n');

// ========== 区域标题组件 ==========
function SectionTitle({ icon, title, desc }) {
    return h('div', { style: { marginBottom: spacing.lg } },
        h('div', {
            style: {
                fontSize: fonts.sizes.lg, fontWeight: '700', color: colors.text,
                display: 'flex', alignItems: 'center', gap: spacing.sm
            }
        }, icon + ' ' + title),
        desc ? h('div', {
            style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' }
        }, desc) : null
    );
}

// ========== 动画盒子组件 ==========
function AnimBox({ className, label, bg }) {
    return h('div', {
        className: className,
        style: {
            width: '72px', height: '72px', borderRadius: radius.md,
            background: bg || colors.gradPrimary,
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            color: '#fff', fontSize: fonts.sizes.xs, fontWeight: '600',
            textAlign: 'center', lineHeight: '1.2'
        }
    }, label);
}

// ========== 标签组件 ==========
function Tag({ text, color }) {
    return h('span', {
        style: {
            display: 'inline-block', padding: '2px 8px', borderRadius: '10px',
            fontSize: fonts.sizes.xs, fontWeight: '600',
            backgroundColor: color || colors.surface3, color: '#fff'
        }
    }, text);
}

// ========== 主页面 ==========
function createSectionEnabledChecker(enabledSections) {
    if (!enabledSections || !enabledSections.length) {
        return function () { return true; };
    }
    var map = {};
    for (var i = 0; i < enabledSections.length; i++) {
        map[String(enabledSections[i])] = true;
    }
    return function (idx) {
        return !!map[String(idx)];
    };
}

export function ShowcasePage({ addToast, enabledSections }) {
    var isSectionEnabled = createSectionEnabledChecker(enabledSections);
    var showSection1 = isSectionEnabled(1);
    var showSection2 = isSectionEnabled(2);
    var showSection3 = isSectionEnabled(3);
    var showSection4 = isSectionEnabled(4);
    var showSection5 = isSectionEnabled(5);

    var checkState = useState(false);
    var checked = checkState[0];
    var setChecked = checkState[1];

    var textState = useState('');
    var textVal = textState[0];
    var setTextVal = textState[1];

    // 注入 <style> 标签
    useEffect(function () {
        var style = document.createElement('style');
        style.textContent = SHOWCASE_CSS;
        document.head.appendChild(style);
        return function () { document.head.removeChild(style); };
    }, []);

    return h('div', null,
        // 页面标题
        h('div', { style: { marginBottom: spacing.xl } },
            h('div', { style: { fontSize: fonts.sizes.xl, fontWeight: '700', color: colors.text } },
                'CSS Showcase'),
            h('div', { style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' } },
                'Explore all CSS capabilities of the MBink engine')
        ),

        // ===== Section 1: CSS @keyframes 动画 =====
        showSection1 && h(Card, { style: { marginBottom: spacing.xl } },
            h(SectionTitle, {
                icon: '🎬', title: 'CSS @keyframes Animation',
                desc: '@keyframes rules with various timing functions and iteration modes'
            }),
            h('div', {
                style: { display: 'flex', gap: spacing.lg, flexWrap: 'wrap', alignItems: 'flex-end' }
            },
                h(AnimBox, { className: 'anim-bounce', label: 'Bounce', bg: colors.gradPrimary }),
                h(AnimBox, { className: 'anim-spin', label: 'Spin', bg: colors.gradDanger }),
                h(AnimBox, { className: 'anim-pulse', label: 'Pulse', bg: colors.gradSuccess }),
                h(AnimBox, { className: 'anim-fadeIn', label: 'Fade In', bg: colors.gradInfo }),
                h(AnimBox, { className: 'anim-color', label: 'Color', bg: '#6c5ce7' }),
                h(AnimBox, { className: 'anim-slideIn', label: 'Slide In', bg: colors.gradWarning })
            ),
            h('div', { style: { marginTop: spacing.md, display: 'flex', gap: spacing.xs, flexWrap: 'wrap' } },
                h(Tag, { text: '@keyframes', color: '#6c5ce7' }),
                h(Tag, { text: 'infinite', color: '#00cec9' }),
                h(Tag, { text: 'ease-in-out', color: '#e17055' }),
                h(Tag, { text: 'alternate', color: '#fdcb6e' })
            )
        ),

        // ===== Section 2: 伪类交互 =====
        showSection2 && h(Card, { style: { marginBottom: spacing.xl } },
            h(SectionTitle, {
                icon: '🖱️', title: 'Pseudo-class Interaction',
                desc: 'Built-in :hover/:active on <button>, :focus on <input>, :disabled, :checked'
            }),
            // 按钮行：展示 button 内置 :hover / :active
            h('div', { style: { display: 'flex', gap: spacing.md, flexWrap: 'wrap', marginBottom: spacing.lg } },
                h('button', {
                    style: {
                        padding: spacing.md + ' ' + spacing.xl, border: 'none',
                        borderRadius: radius.sm, fontSize: fonts.sizes.sm, fontWeight: '600',
                        backgroundColor: '#6c5ce7', color: '#fff', cursor: 'pointer'
                    },
                    onClick: function () { if (addToast) addToast(':hover + :active works!', 'success'); }
                }, 'Primary Button'),
                h('button', {
                    style: {
                        padding: spacing.md + ' ' + spacing.xl, border: 'none',
                        borderRadius: radius.sm, fontSize: fonts.sizes.sm, fontWeight: '600',
                        backgroundColor: '#00cec9', color: '#fff', cursor: 'pointer'
                    },
                    onClick: function () { if (addToast) addToast('Clicked teal button!', 'info'); }
                }, 'Teal Button'),
                h('button', {
                    style: {
                        padding: spacing.md + ' ' + spacing.xl, border: 'none',
                        borderRadius: radius.sm, fontSize: fonts.sizes.sm, fontWeight: '600',
                        backgroundColor: '#ff6b6b', color: '#fff', cursor: 'pointer'
                    },
                    onClick: function () { if (addToast) addToast('Danger clicked!', 'error'); }
                }, 'Danger Button'),
                // :disabled 按钮
                h('button', {
                    disabled: true,
                    style: {
                        padding: spacing.md + ' ' + spacing.xl, border: 'none',
                        borderRadius: radius.sm, fontSize: fonts.sizes.sm, fontWeight: '600',
                        backgroundColor: '#636e72', color: '#fff', cursor: 'not-allowed'
                    }
                }, 'Disabled')
            ),
            h('div', { style: { display: 'flex', gap: spacing.xs, flexWrap: 'wrap' } },
                h(Tag, { text: ':hover', color: '#6c5ce7' }),
                h(Tag, { text: ':active', color: '#d63031' }),
                h(Tag, { text: ':focus', color: '#0984e3' }),
                h(Tag, { text: ':disabled', color: '#636e72' }),
                h(Tag, { text: ':checked', color: '#00cec9' })
            )
        ),

        // ===== Section 3: 表单输入 =====
        showSection3 && h(Card, { style: { marginBottom: spacing.xl } },
            h(SectionTitle, {
                icon: '📝', title: 'Form Elements',
                desc: '<input>, <textarea>, <button>, checkbox with :focus and :checked'
            }),
            h('div', { style: { display: 'flex', flexDirection: 'column', gap: spacing.lg } },
                // 文本输入
                h('div', null,
                    h('div', {
                        style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginBottom: spacing.xs }
                    }, 'Text Input (click to see :focus outline)'),
                    h('input', {
                        type: 'text', placeholder: 'Type something here...',
                        value: textVal,
                        onInput: function (e) { setTextVal(e.target.value); },
                        style: {
                            width: '320px', padding: spacing.md, fontSize: fonts.sizes.md,
                            backgroundColor: colors.bgInput, color: colors.text,
                            border: '1px solid ' + colors.border, borderRadius: radius.sm,
                            outline: 'none'
                        }
                    })
                ),
                // Checkbox
                h('div', { style: { display: 'flex', alignItems: 'center', gap: spacing.md } },
                    h('input', {
                        type: 'checkbox', checked: checked,
                        onChange: function () { setChecked(!checked); },
                        style: { width: '18px', height: '18px', cursor: 'pointer' }
                    }),
                    h('span', { style: { color: colors.text, fontSize: fonts.sizes.md } },
                        checked ? '✅ Checked (:checked pseudo-class active)' : 'Click to check'
                    )
                ),
                // Textarea
                h('div', null,
                    h('div', {
                        style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginBottom: spacing.xs }
                    }, 'Textarea (multi-line input with :focus)'),
                    h('textarea', {
                        placeholder: 'Multi-line text area...\nSupports :focus outline.',
                        rows: 3,
                        style: {
                            width: '320px', padding: spacing.md, fontSize: fonts.sizes.sm,
                            backgroundColor: colors.bgInput, color: colors.text,
                            border: '1px solid ' + colors.border, borderRadius: radius.sm,
                            outline: 'none', resize: 'vertical', fontFamily: fonts.base
                        }
                    })
                )
            ),
            h('div', { style: { marginTop: spacing.md, display: 'flex', gap: spacing.xs, flexWrap: 'wrap' } },
                h(Tag, { text: '<input>', color: '#0984e3' }),
                h(Tag, { text: '<textarea>', color: '#6c5ce7' }),
                h(Tag, { text: '<checkbox>', color: '#00cec9' }),
                h(Tag, { text: ':focus outline', color: '#fdcb6e' })
            )
        ),

        // ===== Section 4: CSS Transition 过渡 =====
        showSection4 && h(Card, { style: { marginBottom: spacing.xl } },
            h(SectionTitle, {
                icon: '🔄', title: 'CSS Transition',
                desc: 'Hover these cards to see smooth transition effects via <style> rules'
            }),
            h('div', { style: { display: 'flex', gap: spacing.lg, flexWrap: 'wrap' } },
                // 卡片上浮 + 发光
                h('div', { className: 'trans-card', style: { flex: '1', minWidth: '160px' } },
                    h('div', { style: { fontSize: '24px', marginBottom: spacing.sm } }, '🚀'),
                    h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '600' } },
                        'Lift & Glow'),
                    h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginTop: '4px' } },
                        'translateY + box-shadow + border-color')
                ),
                // 放大
                h('div', {
                    className: 'trans-grow',
                    style: {
                        flex: '1', minWidth: '160px', padding: spacing.lg,
                        backgroundColor: colors.surface1, borderRadius: radius.md,
                        border: '1px solid ' + colors.border
                    }
                },
                    h('div', { style: { fontSize: '24px', marginBottom: spacing.sm } }, '🔍'),
                    h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '600' } },
                        'Scale Up'),
                    h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginTop: '4px' } },
                        'transform: scale(1.08)')
                ),
                // 边框发光
                h('div', {
                    className: 'trans-glow',
                    style: {
                        flex: '1', minWidth: '160px', padding: spacing.lg,
                        backgroundColor: colors.surface1, borderRadius: radius.md
                    }
                },
                    h('div', { style: { fontSize: '24px', marginBottom: spacing.sm } }, '✨'),
                    h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '600' } },
                        'Neon Glow'),
                    h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginTop: '4px' } },
                        'border-color + box-shadow glow')
                )
            ),
            h('div', { style: { marginTop: spacing.md, display: 'flex', gap: spacing.xs, flexWrap: 'wrap' } },
                h(Tag, { text: 'transition', color: '#6c5ce7' }),
                h(Tag, { text: ':hover', color: '#00cec9' }),
                h(Tag, { text: 'ease / ease-in-out', color: '#e17055' })
            )
        ),

        // ===== Section 5: 视觉效果 =====
        showSection5 && h(Card, { style: { marginBottom: spacing.xl } },
            h(SectionTitle, {
                icon: '🎨', title: 'Visual Effects',
                desc: 'transform, box-shadow, text-shadow, text-transform, opacity'
            }),
            h('div', { style: { display: 'flex', gap: spacing.lg, flexWrap: 'wrap', marginBottom: spacing.lg } },
                // Transform: rotate
                h('div', {
                    style: {
                        width: '72px', height: '72px', borderRadius: radius.md,
                        background: colors.gradPrimary, display: 'flex',
                        alignItems: 'center', justifyContent: 'center',
                        color: '#fff', fontSize: fonts.sizes.xs, fontWeight: '600',
                        transform: 'rotate(15deg)'
                    }
                }, 'Rotate\n15°'),
                // Transform: scale
                h('div', {
                    style: {
                        width: '72px', height: '72px', borderRadius: radius.md,
                        background: colors.gradSuccess, display: 'flex',
                        alignItems: 'center', justifyContent: 'center',
                        color: '#fff', fontSize: fonts.sizes.xs, fontWeight: '600',
                        transform: 'scale(1.2)'
                    }
                }, 'Scale\n1.2x'),
                // box-shadow
                h('div', {
                    style: {
                        width: '72px', height: '72px', borderRadius: radius.md,
                        backgroundColor: colors.surface1, display: 'flex',
                        alignItems: 'center', justifyContent: 'center',
                        color: colors.text, fontSize: fonts.sizes.xs, fontWeight: '600',
                        boxShadow: '0 0 20px rgba(108,92,231,0.5), 0 0 40px rgba(108,92,231,0.2)'
                    }
                }, 'Box\nShadow'),
                // opacity
                h('div', {
                    style: {
                        width: '72px', height: '72px', borderRadius: radius.md,
                        background: colors.gradDanger, display: 'flex',
                        alignItems: 'center', justifyContent: 'center',
                        color: '#fff', fontSize: fonts.sizes.xs, fontWeight: '600',
                        opacity: '0.5'
                    }
                }, 'Opacity\n0.5')
            ),
            // text-shadow + text-transform
            h('div', { style: { display: 'flex', flexDirection: 'column', gap: spacing.md } },
                h('div', {
                    style: {
                        fontSize: fonts.sizes.xl, fontWeight: '800', color: '#a29bfe',
                        textShadow: '0 0 10px rgba(162,155,254,0.6), 0 0 20px rgba(162,155,254,0.3)'
                    }
                }, 'Glowing Text Shadow'),
                h('div', { style: { display: 'flex', gap: spacing.xl } },
                    h('div', null,
                        h('div', { style: { fontSize: fonts.sizes.xs, color: colors.textMuted, marginBottom: '2px' } }, 'text-transform: uppercase'),
                        h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, textTransform: 'uppercase' } }, 'hello world')
                    ),
                    h('div', null,
                        h('div', { style: { fontSize: fonts.sizes.xs, color: colors.textMuted, marginBottom: '2px' } }, 'text-transform: capitalize'),
                        h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, textTransform: 'capitalize' } }, 'hello world')
                    )
                )
            ),
            h('div', { style: { marginTop: spacing.md, display: 'flex', gap: spacing.xs, flexWrap: 'wrap' } },
                h(Tag, { text: 'transform', color: '#6c5ce7' }),
                h(Tag, { text: 'box-shadow', color: '#0984e3' }),
                h(Tag, { text: 'text-shadow', color: '#a29bfe' }),
                h(Tag, { text: 'text-transform', color: '#fdcb6e' }),
                h(Tag, { text: 'opacity', color: '#636e72' })
            )
        )
    );
}

