/**
 * @file settings.js
 * @brief 设置页面 - 开关切换、选项分组、useReducer 状态管理
 */
import { h } from 'preact';
import { useReducer, useCallback } from 'preact/hooks';
import { colors, spacing, radius, fonts } from './theme.js';
import { Switch, Card } from './components.js';

const initialSettings = {
    darkMode: true, animations: true, notifications: true,
    autoSave: true, compactMode: false, devTools: false,
    hardwareAccel: true, vsync: true, antiAlias: true,
    fontSize: 'medium', language: 'en'
};

function settingsReducer(state, action) {
    switch (action.type) {
        case 'TOGGLE': return Object.assign({}, state, { [action.key]: !state[action.key] });
        case 'SET': return Object.assign({}, state, { [action.key]: action.value });
        default: return state;
    }
}

function SettingGroup({ title, children }) {
    return h(Card, { style: { marginBottom: spacing.lg } },
        h('div', {
            style: {
                fontSize: fonts.sizes.md, fontWeight: '600', color: colors.text,
                marginBottom: spacing.lg, paddingBottom: spacing.sm,
                borderBottom: '1px solid ' + colors.border
            }
        }, title),
        h('div', { style: { display: 'flex', flexDirection: 'column', gap: spacing.lg } }, children)
    );
}

function SettingRow({ label, desc, children }) {
    return h('div', {
        style: { display: 'flex', justifyContent: 'space-between', alignItems: 'center' }
    },
        h('div', { style: { flex: 1 } },
            h('div', { style: { color: colors.text, fontSize: fonts.sizes.sm, fontWeight: '500' } }, label),
            desc ? h('div', { style: { color: colors.textMuted, fontSize: fonts.sizes.xs, marginTop: '2px' } }, desc) : null
        ),
        children
    );
}

function OptionPicker({ options, value, onChange }) {
    return h('div', { style: { display: 'flex', gap: spacing.xs } },
        options.map(function (opt) {
            var active = value === opt.value;
            return h('div', {
                key: opt.value,
                style: {
                    padding: spacing.xs + ' ' + spacing.md, borderRadius: radius.sm,
                    fontSize: fonts.sizes.xs, cursor: 'pointer', fontWeight: '500',
                    backgroundColor: active ? colors.primary : colors.surface2,
                    color: active ? '#fff' : colors.textSec,
                    border: '1px solid ' + (active ? colors.primary : colors.border)
                },
                onClick: function () { onChange(opt.value); }
            }, opt.label);
        })
    );
}

export function SettingsPage({ addToast }) {
    var ref = useReducer(settingsReducer, initialSettings);
    var settings = ref[0];
    var dispatch = ref[1];

    var toggle = useCallback(function (key) {
        dispatch({ type: 'TOGGLE', key: key });
        if (addToast) addToast('Setting updated: ' + key, 'success');
    }, [addToast]);

    var setVal = useCallback(function (key, value) {
        dispatch({ type: 'SET', key: key, value: value });
    }, []);

    return h('div', null,
        h('div', { style: { marginBottom: spacing.xl } },
            h('div', { style: { fontSize: fonts.sizes.xl, fontWeight: '700', color: colors.text } }, 'Settings'),
            h('div', { style: { fontSize: fonts.sizes.sm, color: colors.textSec, marginTop: '4px' } }, 'Customize your workspace preferences')
        ),

        h(SettingGroup, { title: '🎨 Appearance' },
            h(SettingRow, { label: 'Dark Mode', desc: 'Use dark color theme' },
                h(Switch, { checked: settings.darkMode, onChange: function () { toggle('darkMode'); } })
            ),
            h(SettingRow, { label: 'Animations', desc: 'Enable UI transitions' },
                h(Switch, { checked: settings.animations, onChange: function () { toggle('animations'); } })
            ),
            h(SettingRow, { label: 'Compact Mode', desc: 'Reduce spacing and padding' },
                h(Switch, { checked: settings.compactMode, onChange: function () { toggle('compactMode'); } })
            ),
            h(SettingRow, { label: 'Font Size', desc: 'Adjust text size' },
                h(OptionPicker, {
                    options: [{ label: 'S', value: 'small' }, { label: 'M', value: 'medium' }, { label: 'L', value: 'large' }],
                    value: settings.fontSize, onChange: function (v) { setVal('fontSize', v); }
                })
            )
        ),

        h(SettingGroup, { title: '⚡ Performance' },
            h(SettingRow, { label: 'Hardware Acceleration', desc: 'Use GPU for rendering' },
                h(Switch, { checked: settings.hardwareAccel, onChange: function () { toggle('hardwareAccel'); } })
            ),
            h(SettingRow, { label: 'VSync', desc: 'Synchronize with display refresh' },
                h(Switch, { checked: settings.vsync, onChange: function () { toggle('vsync'); } })
            ),
            h(SettingRow, { label: 'Anti-Aliasing', desc: 'Smooth edges on rendered elements' },
                h(Switch, { checked: settings.antiAlias, onChange: function () { toggle('antiAlias'); } })
            )
        ),

        h(SettingGroup, { title: '🔔 Notifications' },
            h(SettingRow, { label: 'Enable Notifications', desc: 'Show toast notifications' },
                h(Switch, { checked: settings.notifications, onChange: function () { toggle('notifications'); } })
            ),
            h(SettingRow, { label: 'Auto Save', desc: 'Automatically save changes' },
                h(Switch, { checked: settings.autoSave, onChange: function () { toggle('autoSave'); } })
            ),
            h(SettingRow, { label: 'Developer Tools', desc: 'Show debug panel' },
                h(Switch, { checked: settings.devTools, onChange: function () { toggle('devTools'); } })
            )
        )
    );
}

