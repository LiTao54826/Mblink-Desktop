/**
 * @file dashboard.js
 * @brief 仪表盘页面 - 统计卡片、进度条、活动时间线
 */
import { h } from 'preact';
import { colors, spacing, radius, fonts } from './theme.js';
import { StatCard, ProgressBar, Card, Badge } from './components.js';

// 活动时间线项
function TimelineItem({ icon, title, desc, time, color }) {
    return h('div', {
        style: { display: 'flex', gap: spacing.md, padding: spacing.md + ' 0' }
    },
        h('div', {
            style: {
                width: '32px', height: '32px', borderRadius: '50%', flexShrink: 0,
                backgroundColor: color || colors.surface2,
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                fontSize: '14px'
            }
        }, icon),
        h('div', { style: { flex: 1 } },
            h('div', { style: { color: colors.text, fontSize: fonts.sizes.sm, fontWeight: '500' } }, title),
            h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginTop: '2px' } }, desc)
        ),
        h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, flexShrink: 0 } }, time)
    );
}

// 项目进度项
function ProjectProgress({ name, progress, color }) {
    return h('div', { style: { marginBottom: spacing.lg } },
        h('div', {
            style: {
                display: 'flex', justifyContent: 'space-between', marginBottom: spacing.xs,
                fontSize: fonts.sizes.sm
            }
        },
            h('span', { style: { color: colors.text } }, name),
            h('span', { style: { color: colors.textSec } }, progress + '%')
        ),
        h(ProgressBar, { value: progress, gradient: color })
    );
}

export function DashboardPage({ addToast }) {
    return h('div', null,
        // 页面标题
        h('div', { style: { marginBottom: spacing.xl } },
            h('div', { style: { fontSize: fonts.sizes.xl, fontWeight: '700', color: colors.text } }, 'Dashboard'),
            h('div', { style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' } }, 'Welcome back! Here is your project overview.')
        ),

        // 统计卡片行
        h('div', { style: { display: 'flex', gap: spacing.lg, marginBottom: spacing.xl } },
            h(StatCard, { icon: '📊', label: 'Total Tasks', value: '2,847', trend: '+12%', gradient: colors.gradPrimary }),
            h(StatCard, { icon: '✅', label: 'Completed', value: '1,523', trend: '+8%', gradient: colors.gradSuccess }),
            h(StatCard, { icon: '⏳', label: 'In Progress', value: '384', trend: '-3%', gradient: colors.gradWarning }),
            h(StatCard, { icon: '🐛', label: 'Open Bugs', value: '42', trend: '-15%', gradient: colors.gradDanger })
        ),

        // 下半部分：项目进度 + 活动时间线
        h('div', { style: { display: 'flex', gap: spacing.lg } },
            // 项目进度
            h(Card, { style: { flex: 1 } },
                h('div', {
                    style: {
                        fontSize: fonts.sizes.lg, fontWeight: '600', color: colors.text,
                        marginBottom: spacing.lg, paddingBottom: spacing.md,
                        borderBottom: '1px solid ' + colors.border
                    }
                }, 'Project Progress'),
                h(ProjectProgress, { name: 'MBink Core Engine', progress: 78, color: colors.gradPrimary }),
                h(ProjectProgress, { name: 'Preact Integration', progress: 92, color: colors.gradSuccess }),
                h(ProjectProgress, { name: 'CSS Layout System', progress: 65, color: colors.gradWarning }),
                h(ProjectProgress, { name: 'DevTools Panel', progress: 45, color: colors.gradInfo }),
                h(ProjectProgress, { name: 'Documentation', progress: 30, color: colors.gradDanger })
            ),

            // 活动时间线
            h(Card, { style: { flex: 1 } },
                h('div', {
                    style: {
                        fontSize: fonts.sizes.lg, fontWeight: '600', color: colors.text,
                        marginBottom: spacing.md, paddingBottom: spacing.md,
                        borderBottom: '1px solid ' + colors.border
                    }
                }, 'Recent Activity'),
                h(TimelineItem, { icon: '🚀', title: 'New release v2.1.0', desc: 'Published to production', time: '2m ago', color: colors.surface3 }),
                h(TimelineItem, { icon: '🔧', title: 'Bug fix merged', desc: 'Fixed flex layout overflow', time: '15m ago', color: colors.surface3 }),
                h(TimelineItem, { icon: '👤', title: 'New team member', desc: 'Alice joined the project', time: '1h ago', color: colors.surface3 }),
                h(TimelineItem, { icon: '📝', title: 'Code review', desc: 'Reviewed PR #142 - ESM loader', time: '3h ago', color: colors.surface3 }),
                h(TimelineItem, { icon: '🎨', title: 'UI redesign', desc: 'Updated dark theme colors', time: '5h ago', color: colors.surface3 }),
                h(TimelineItem, { icon: '📦', title: 'Dependency update', desc: 'Updated QuickJS to latest', time: '1d ago', color: colors.surface3 })
            )
        )
    );
}

