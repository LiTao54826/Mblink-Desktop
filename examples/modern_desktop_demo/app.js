/**
 * @file app.js
 * @brief 入口文件 - 主 App 组件，路由、Toast 通知系统
 */
import { h, render } from 'preact';
import { useState, useReducer, useCallback, useEffect } from 'preact/hooks';
import { colors, spacing, fonts, ToastContext } from './src/theme.js';
import { ToastContainer } from './src/components.js';
import { TitleBar } from './src/titlebar.js';
import { Sidebar } from './src/sidebar.js';
import { DashboardPage } from './src/dashboard.js';
import { KanbanPage } from './src/kanban.js';
import { TeamPage } from './src/team.js';
import { SettingsPage } from './src/settings.js';
import { ShowcasePage } from './src/showcase.js';

// ========== Toast 通知 Reducer ==========
var toastId = 0;
function toastReducer(state, action) {
    switch (action.type) {
        case 'ADD':
            return state.concat([{ id: ++toastId, message: action.message, type: action.toastType || 'info' }]);
        case 'REMOVE':
            return state.filter(function (t) { return t.id !== action.id; });
        default: return state;
    }
}

// ========== 页面路由 ==========
function PageContent({ page, addToast }) {
    switch (page) {
        case 'dashboard': return h(DashboardPage, { addToast: addToast });
        case 'kanban':    return h(KanbanPage, { addToast: addToast });
        case 'team':      return h(TeamPage, null);
        case 'settings':  return h(SettingsPage, { addToast: addToast });
        case 'showcase':  return h(ShowcasePage, { addToast: addToast });
        default:          return h(DashboardPage, { addToast: addToast });
    }
}

// ========== 主 App 组件 ==========
function App() {
    var pageState = useState('dashboard');
    var activePage = pageState[0];
    var setActivePage = pageState[1];

    var toastState = useReducer(toastReducer, []);
    var toasts = toastState[0];
    var dispatchToast = toastState[1];

    var addToast = useCallback(function (message, type) {
        var id = toastId + 1;
        dispatchToast({ type: 'ADD', message: message, toastType: type || 'info' });
        setTimeout(function () {
            dispatchToast({ type: 'REMOVE', id: id });
        }, 3000);
    }, [dispatchToast]);

    var onPageChange = useCallback(function (page) {
        setActivePage(page);
    }, []);

    // 设置 body 样式
    useEffect(function () {
        document.body.style.margin = '0';
        document.body.style.padding = '0';
        document.body.style.overflow = 'hidden';
        document.body.style.backgroundColor = colors.bg;
        document.body.style.fontFamily = fonts.base;
        document.body.style.color = colors.text;
    }, []);

    return h(ToastContext.Provider, { value: addToast },
        h('div', {
            style: {
                width: '100vw', height: '100vh',
                display: 'flex', flexDirection: 'column',
                backgroundColor: colors.bg, overflow: 'hidden'
            }
        },
            // 标题栏
            h(TitleBar, { title: 'MBlink Modern Desktop Demo' }),

            // 主体区域：侧边栏 + 内容
            h('div', {
                style: {
                    flex: 1, display: 'flex', overflow: 'hidden'
                }
            },
                // 侧边栏
                h(Sidebar, { activePage: activePage, onPageChange: onPageChange }),

                // 内容区域
                h('div', {
                    style: {
                        flex: 1, padding: spacing.xl, overflowY: 'auto',
                        backgroundColor: colors.bg,
                        'scrollbar-color': '#555570 #1a1a2e'
                    }
                },
                    h(PageContent, { page: activePage, addToast: addToast })
                )
            ),

            // Toast 通知层
            h(ToastContainer, { toasts: toasts })
        )
    );
}

// ========== 渲染 ==========
render(h(App, null), document.body);

