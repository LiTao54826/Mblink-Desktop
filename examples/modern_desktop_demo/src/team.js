/**
 * @file team.js
 * @brief 团队页面 - 成员卡片、头像、在线状态、角色标签
 */
import { h } from 'preact';
import { colors, spacing, radius, fonts, shadows } from './theme.js';
import { Avatar, Badge, Card } from './components.js';

const MEMBERS = [
    { name: 'Alice Chen', role: 'Lead Engineer', status: 'online', tasks: 12, commits: 847, color: colors.gradPrimary },
    { name: 'Bob Wang', role: 'Frontend Dev', status: 'online', tasks: 8, commits: 523, color: colors.gradSuccess },
    { name: 'Carol Li', role: 'UI Designer', status: 'away', tasks: 5, commits: 234, color: colors.gradWarning },
    { name: 'Dave Zhang', role: 'Backend Dev', status: 'online', tasks: 15, commits: 1024, color: colors.gradInfo },
    { name: 'Eve Liu', role: 'QA Engineer', status: 'offline', tasks: 3, commits: 156, color: colors.gradDanger },
    { name: 'Frank Wu', role: 'DevOps', status: 'online', tasks: 7, commits: 389, color: colors.gradDark },
];

const statusMap = { online: colors.success, away: colors.warning, offline: colors.textMuted };
const statusText = { online: 'Online', away: 'Away', offline: 'Offline' };

function MemberStat({ label, value }) {
    return h('div', { style: { textAlign: 'center' } },
        h('div', { style: { fontSize: fonts.sizes.lg, fontWeight: '700', color: colors.text } }, String(value)),
        h('div', { style: { fontSize: fonts.sizes.xs, color: colors.textMuted, marginTop: '2px' } }, label)
    );
}

function MemberCard({ member }) {
    const isOnline = member.status === 'online';
    return h(Card, {
        style: {
            flex: '1 1 calc(33.333% - 12px)', minWidth: '220px',
            display: 'flex', flexDirection: 'column', alignItems: 'center',
            padding: spacing.xl, position: 'relative', overflow: 'hidden'
        }
    },
        // 顶部装饰条
        h('div', {
            style: {
                position: 'absolute', top: 0, left: 0, right: 0, height: '4px',
                background: member.color
            }
        }),
        // 头像
        h('div', { style: { marginBottom: spacing.md, marginTop: spacing.sm } },
            h(Avatar, { name: member.name, size: 56, color: member.color, online: isOnline })
        ),
        // 名字
        h('div', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '600', marginBottom: '4px' } }, member.name),
        // 角色
        h('div', { style: { color: colors.textSec, fontSize: fonts.sizes.sm, marginBottom: spacing.sm } }, member.role),
        // 状态标签
        h('div', { style: { display: 'flex', alignItems: 'center', gap: spacing.xs, marginBottom: spacing.lg } },
            h('div', {
                style: {
                    width: '6px', height: '6px', borderRadius: '50%',
                    backgroundColor: statusMap[member.status]
                }
            }),
            h('span', { style: { fontSize: fonts.sizes.xs, color: statusMap[member.status] } }, statusText[member.status])
        ),
        // 统计
        h('div', {
            style: {
                display: 'flex', gap: spacing.xl, width: '100%', justifyContent: 'center',
                paddingTop: spacing.md, borderTop: '1px solid ' + colors.border
            }
        },
            h(MemberStat, { label: 'Tasks', value: member.tasks }),
            h(MemberStat, { label: 'Commits', value: member.commits })
        )
    );
}

export function TeamPage() {
    return h('div', null,
        h('div', { style: { marginBottom: spacing.xl } },
            h('div', { style: { fontSize: fonts.sizes.xl, fontWeight: '700', color: colors.text } }, 'Team'),
            h('div', { style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' } },
                MEMBERS.filter(function (m) { return m.status === 'online'; }).length + ' of ' + MEMBERS.length + ' members online'
            )
        ),
        h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: spacing.lg } },
            MEMBERS.map(function (m) {
                return h(MemberCard, { key: m.name, member: m });
            })
        )
    );
}

