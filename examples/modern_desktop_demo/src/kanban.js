/**
 * @file kanban.js
 * @brief 看板页面 - 三列看板 (Todo / In Progress / Done)
 */
import { h } from 'preact';
import { useState, useCallback } from 'preact/hooks';
import { colors, spacing, radius, fonts, shadows } from './theme.js';
import { Badge, Avatar, Card } from './components.js';

const INITIAL_TASKS = {
    todo: [
        { id: 1, title: 'Implement CSS Grid', desc: 'Add CSS Grid layout support', priority: 'high', assignee: 'Alice', tags: ['layout'] },
        { id: 2, title: 'Fix scroll overflow', desc: 'Content clips on resize', priority: 'medium', assignee: 'Bob', tags: ['bug'] },
        { id: 3, title: 'Add dark mode toggle', desc: 'Theme switching support', priority: 'low', assignee: 'Carol', tags: ['ui'] },
    ],
    progress: [
        { id: 4, title: 'ESM module loader', desc: 'Support import/export syntax', priority: 'high', assignee: 'Dave', tags: ['core'] },
        { id: 5, title: 'Preact hooks polish', desc: 'useReducer & useContext', priority: 'medium', assignee: 'Alice', tags: ['framework'] },
    ],
    done: [
        { id: 6, title: 'Flexbox engine', desc: 'Complete flex layout', priority: 'high', assignee: 'Bob', tags: ['layout', 'core'] },
        { id: 7, title: 'Event system', desc: 'Click, hover, focus events', priority: 'high', assignee: 'Dave', tags: ['core'] },
        { id: 8, title: 'Font rendering', desc: 'Skia text rendering', priority: 'medium', assignee: 'Carol', tags: ['render'] },
    ]
};

const priorityColors = { high: colors.danger, medium: colors.warning, low: colors.success };
const tagColors = {
    layout: colors.gradPrimary, bug: colors.gradDanger, ui: colors.gradInfo,
    core: colors.gradSuccess, framework: colors.gradWarning, render: colors.gradDark
};

function TaskCard({ task }) {
    const pColor = priorityColors[task.priority] || colors.textMuted;
    return h('div', {
        style: {
            backgroundColor: colors.surface1, borderRadius: radius.md,
            padding: spacing.md, marginBottom: spacing.sm,
            border: '1px solid ' + colors.border, cursor: 'pointer'
        }
    },
        // 标题行
        h('div', {
            style: {
                display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start',
                marginBottom: spacing.xs
            }
        },
            h('div', { style: { color: colors.text, fontSize: fonts.sizes.sm, fontWeight: '600', flex: 1 } }, task.title),
            h('div', {
                style: {
                    width: '8px', height: '8px', borderRadius: '50%',
                    backgroundColor: pColor, flexShrink: 0, marginTop: '4px', marginLeft: spacing.xs
                }
            })
        ),
        // 描述
        h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginBottom: spacing.sm } }, task.desc),
        // 底部：标签 + 头像
        h('div', { style: { display: 'flex', justifyContent: 'space-between', alignItems: 'center' } },
            h('div', { style: { display: 'flex', gap: '4px', flexWrap: 'wrap' } },
                task.tags.map(function (tag) {
                    return h(Badge, { key: tag, text: tag, bg: colors.surface3 });
                })
            ),
            h(Avatar, { name: task.assignee, size: 22, color: tagColors[task.tags[0]] })
        )
    );
}

function KanbanColumn({ title, count, tasks, color }) {
    return h('div', {
        style: {
            flex: 1, backgroundColor: colors.bg, borderRadius: radius.md,
            padding: spacing.md, display: 'flex', flexDirection: 'column',
            border: '1px solid ' + colors.border, minHeight: '400px'
        }
    },
        // 列头
        h('div', {
            style: {
                display: 'flex', alignItems: 'center', justifyContent: 'space-between',
                marginBottom: spacing.md, paddingBottom: spacing.sm,
                borderBottom: '2px solid ' + (color || colors.primary)
            }
        },
            h('div', { style: { display: 'flex', alignItems: 'center', gap: spacing.sm } },
                h('span', { style: { color: colors.text, fontSize: fonts.sizes.md, fontWeight: '600' } }, title),
                h('span', {
                    style: {
                        backgroundColor: colors.surface3, color: colors.textSec,
                        padding: '1px 8px', borderRadius: '10px', fontSize: fonts.sizes.xs
                    }
                }, String(count))
            )
        ),
        // 任务列表
        h('div', { style: { flex: 1, overflowY: 'auto' } },
            tasks.map(function (task) {
                return h(TaskCard, { key: task.id, task: task });
            })
        )
    );
}

export function KanbanPage({ addToast }) {
    const state = useState(INITIAL_TASKS);
    const tasks = state[0];

    return h('div', null,
        h('div', { style: { marginBottom: spacing.xl } },
            h('div', { style: { fontSize: fonts.sizes.xl, fontWeight: '700', color: colors.text } }, 'Project Board'),
            h('div', { style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' } }, 'Drag tasks between columns to update status')
        ),
        h('div', { style: { display: 'flex', gap: spacing.lg } },
            h(KanbanColumn, { title: 'To Do', count: tasks.todo.length, tasks: tasks.todo, color: colors.info }),
            h(KanbanColumn, { title: 'In Progress', count: tasks.progress.length, tasks: tasks.progress, color: colors.warning }),
            h(KanbanColumn, { title: 'Done', count: tasks.done.length, tasks: tasks.done, color: colors.success })
        )
    );
}

