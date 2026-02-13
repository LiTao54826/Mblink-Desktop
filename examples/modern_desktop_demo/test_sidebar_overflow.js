/**
 * @file test_sidebar_overflow.js
 * @brief 侧边栏滚动与滚动条样式最小复现测试
 */
import { h, render } from 'preact';
import { colors, spacing, radius, fonts } from './src/theme.js';

function NavItem({ i }) {
    return h('div', {
        style: {
            display: 'flex', alignItems: 'center', gap: spacing.md,
            padding: spacing.md + ' ' + spacing.lg,
            borderRadius: radius.sm, margin: '0 ' + spacing.sm,
            color: colors.textSec, fontSize: fonts.sizes.md,
            borderLeft: '3px solid transparent'
        }
    },
        h('span', { style: { fontSize: '18px', width: '24px', textAlign: 'center' } }, '◉'),
        h('span', null, 'Menu Item ' + i)
    );
}

function App() {
    const items = Array.from({ length: 40 }, function (_, i) { return i + 1; });

    return h('div', {
        style: {
            width: '100vw', height: '100vh',
            backgroundColor: colors.bg, display: 'flex', overflow: 'hidden',
            fontFamily: fonts.base
        }
    },
        h('div', {
            style: {
                width: '220px', minWidth: '220px', backgroundColor: colors.bgSidebar,
                display: 'flex', flexDirection: 'column',
                borderRight: '1px solid ' + colors.border,
                overflow: 'hidden'
            }
        },
            h('div', {
                style: {
                    padding: spacing.xl, color: colors.text,
                    borderBottom: '1px solid ' + colors.border,
                    fontWeight: '700'
                }
            }, 'Sidebar Scroll Test'),
            h('div', {
                style: {
                    flex: 1,
                    minHeight: 0,
                    paddingTop: spacing.sm,
                    paddingBottom: spacing.sm,
                    overflowY: 'auto',
                    overflowX: 'hidden',
                    'scrollbar-color': (colors.textMuted + ' ' + colors.bgSidebar)
                }
            },
                h('div', {
                    style: {
                        display: 'flex',
                        flexDirection: 'column',
                        gap: '2px'
                    }
                }, items.map(function (i) {
                    return h(NavItem, { key: i, i: i });
                }))
            ),
            h('div', {
                style: {
                    padding: spacing.lg,
                    borderTop: '1px solid ' + colors.border,
                    color: colors.text
                }
            }, 'Bottom User Area')
        ),
        h('div', { style: { flex: 1, color: colors.text, padding: spacing.xl } },
            'Resize window height to verify sidebar menu scroll behavior.'
        )
    );
}

render(h(App, null), document.body);

