/**
 * @file sidebar.js
 * @brief 侧边栏导航组件
 */
import { h } from 'preact';
import { useMemo } from 'preact/hooks';
import { colors, spacing, radius, fonts } from './theme.js';
import { Avatar } from './components.js';

const MENU = [
    { id: 'dashboard', icon: '◉', text: 'Dashboard' },
    { id: 'kanban',    icon: '▦', text: 'Projects' },
    { id: 'team',      icon: '◎', text: 'Team' },
    { id: 'settings',  icon: '⚙', text: 'Settings' },
    { id: 'showcase',  icon: '★', text: 'Showcase' },
];

function NavItem({ item, active, onClick }) {
    return h('div', {
        style: {
            display: 'flex', alignItems: 'center', gap: spacing.md,
            padding: spacing.md + ' ' + spacing.lg, cursor: 'pointer',
            borderRadius: radius.sm, margin: '0 ' + spacing.sm,
            backgroundColor: active ? colors.bgActive : 'transparent',
            color: active ? colors.primary : colors.textSec,
            fontSize: fonts.sizes.md, fontWeight: active ? '600' : '400',
            borderLeft: active ? '3px solid ' + colors.primary : '3px solid transparent',
            transition: 'all 0.15s ease'
        },
        onClick: function () { onClick(item.id); }
    },
        h('span', { style: { fontSize: '18px', width: '24px', textAlign: 'center' } }, item.icon),
        h('span', null, item.text)
    );
}

export function Sidebar({ activePage, onPageChange }) {
    const items = useMemo(function () { return MENU; }, []);

    return h('div', {
        style: {
            width: '220px', minWidth: '220px', backgroundColor: colors.bgSidebar,
            display: 'flex', flexDirection: 'column',
            borderRight: '1px solid ' + colors.border,
            overflow: 'hidden'
        }
    },
        // Logo 区域
        h('div', {
            style: {
                padding: spacing.xl, display: 'flex', alignItems: 'center', gap: spacing.md
            }
        },
            h('div', {
                style: {
                    width: '32px', height: '32px', borderRadius: radius.sm,
                    background: colors.gradPrimary, display: 'flex',
                    alignItems: 'center', justifyContent: 'center',
                    color: '#fff', fontSize: '16px', fontWeight: '800'
                }
            }, 'M'),
            h('div', null,
                h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '700' } }, 'MBink'),
                h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs } }, 'Desktop UI')
            )
        ),

        // 导航菜单
        // 注意：某些引擎/实现对 "overflow" + "display:flex" 的滚动支持不完整。
        // 这里用外层 block 容器负责滚动，内层再用 flex 做纵向排列。
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
            }, items.map(function (item) {
                return h(NavItem, {
                    key: item.id, item: item,
                    active: activePage === item.id,
                    onClick: onPageChange
                });
            }))
        ),

        // 底部用户信息
        h('div', {
            style: {
                padding: spacing.lg, borderTop: '1px solid ' + colors.border,
                display: 'flex', alignItems: 'center', gap: spacing.md
            }
        },
            h(Avatar, { name: 'Admin User', size: 32, online: true }),
            h('div', null,
                h('div', { style: { color: colors.text, fontSize: fonts.sizes.sm, fontWeight: '500' } }, 'Admin'),
                h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs } }, 'Online')
            )
        )
    );
}

