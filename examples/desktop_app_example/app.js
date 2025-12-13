/**
 * @file app.js
 * @brief 桌面应用示例 (Desktop App Example)
 * 
 * 展示了一个典型的桌面应用布局，包含：
 * - 菜单栏 (MenuBar)
 * - 工具栏 (Toolbar)
 * - 侧边栏 (Sidebar)
 * - 主视图 (MainView)
 * - 状态栏 (StatusBar)
 */

// const { h, render, Component } = Preact; // 这些已经是全局变量，无需重新声明
// const { useState, useEffect } = PreactHooks; // 这些也是全局变量

// --- 样式定义 (内联以确保加载) ---
const STYLES = `
    * {
        box-sizing: border-box;
        user-select: none;
    }

    body {
        margin: 0;
        padding: 0;
        font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
        background-color: #f5f5f5;
        color: #333;
        height: 100vh;
        overflow: hidden;
    }

    .app-container {
        display: flex;
        flex-direction: column;
        height: 100vh;
        width: 100vw;
    }

    /* Menu Bar */
    .menubar {
        height: 30px;
        background-color: #f0f0f0;
        display: flex;
        align-items: center;
        padding: 0 5px;
        border-bottom: 1px solid #ddd;
    }

    .menu-item {
        padding: 5px 10px;
        font-size: 12px;
        cursor: default;
    }

    .menu-item:hover {
        background-color: #e0e0e0;
    }

    /* Toolbar */
    .toolbar {
        height: 40px;
        background-color: #f9f9f9;
        display: flex;
        align-items: center;
        padding: 0 10px;
        border-bottom: 1px solid #ddd;
        gap: 5px;
    }

    .tool-btn {
        padding: 6px 12px;
        border: 1px solid transparent;
        border-radius: 4px;
        cursor: pointer;
        background: none;
        font-size: 14px;
        color: #555;
        display: flex;
        align-items: center;
        gap: 6px;
        transition: all 0.2s;
    }

    .tool-btn:hover {
        background-color: #eee;
        border-color: #ddd;
        color: #000;
    }

    .tool-btn:active {
        background-color: #ddd;
    }

    /* Content Area */
    .content-area {
        display: flex;
        flex: 1;
        overflow: hidden;
    }

    /* Sidebar */
    .sidebar {
        width: 220px;
        background-color: #2c3e50;
        color: #ecf0f1;
        display: flex;
        flex-direction: column;
        border-right: 1px solid #2c3e50;
    }

    .sidebar-header {
        padding: 15px;
        background-color: #34495e;
        font-weight: bold;
        font-size: 14px;
        letter-spacing: 1px;
    }

    .sidebar-item {
        padding: 12px 15px;
        cursor: pointer;
        font-size: 14px;
        display: flex;
        align-items: center;
        gap: 10px;
        border-left: 3px solid transparent;
        transition: background 0.2s;
    }

    .sidebar-item:hover {
        background-color: #34495e;
    }

    .sidebar-item.active {
        background-color: #3e5871;
        border-left-color: #3498db;
    }

    .sidebar-icon {
        width: 20px;
        text-align: center;
    }

    /* Main View */
    .main-view {
        flex: 1;
        background-color: #fff;
        padding: 24px;
        overflow-y: auto;
    }

    .view-header {
        margin-bottom: 24px;
        border-bottom: 1px solid #eee;
        padding-bottom: 10px;
    }

    .view-title {
        font-size: 24px;
        font-weight: 300;
        margin: 0;
        color: #2c3e50;
    }

    /* Cards */
    .dashboard-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
        gap: 20px;
    }

    .card {
        background: #fff;
        border: 1px solid #eee;
        border-radius: 8px;
        padding: 20px;
        box-shadow: 0 2px 8px rgba(0,0,0,0.05);
        transition: transform 0.2s, box-shadow 0.2s;
    }

    .card:hover {
        transform: translateY(-2px);
        box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    }

    .card h3 {
        margin: 0 0 10px 0;
        color: #7f8c8d;
        font-size: 14px;
        text-transform: uppercase;
    }

    .card .value {
        font-size: 28px;
        font-weight: bold;
        color: #2c3e50;
    }

    /* List */
    .doc-list {
        list-style: none;
        padding: 0;
        margin: 0;
        border: 1px solid #eee;
        border-radius: 8px;
    }

    .doc-item {
        padding: 15px;
        border-bottom: 1px solid #eee;
        display: flex;
        align-items: center;
        gap: 15px;
        cursor: pointer;
    }

    .doc-item:last-child {
        border-bottom: none;
    }

    .doc-item:hover {
        background-color: #f9f9f9;
        color: #3498db;
    }

    /* Form */
    .form-group {
        margin-bottom: 15px;
    }

    .form-group label {
        display: block;
        margin-bottom: 5px;
        color: #7f8c8d;
    }

    .form-control {
        width: 100%;
        padding: 8px 12px;
        border: 1px solid #ddd;
        border-radius: 4px;
        font-size: 14px;
    }

    .btn-primary {
        background-color: #3498db;
        color: white;
        border: none;
        padding: 8px 16px;
        border-radius: 4px;
        cursor: pointer;
    }

    .btn-primary:hover {
        background-color: #2980b9;
    }

    /* Status Bar */
    .statusbar {
        height: 28px;
        background-color: #3498db;
        color: white;
        display: flex;
        align-items: center;
        padding: 0 12px;
        font-size: 12px;
        justify-content: space-between;
    }
`;

// --- Components ---

const MenuBar = () => {
    const menus = ['文件 (F)', '编辑 (E)', '视图 (V)', '工具 (T)', '帮助 (H)'];
    return h('div', { className: 'menubar' },
        menus.map(m => h('div', { className: 'menu-item' }, m))
    );
};

const Toolbar = () => {
    const tools = [
        { icon: '📄', label: '新建' },
        { icon: '📂', label: '打开' },
        { icon: '💾', label: '保存' },
        { icon: '🖨️', label: '打印' },
        { icon: '|', label: '', separator: true },
        { icon: '↩️', label: '撤销' },
        { icon: '↪️', label: '重做' },
        { icon: '|', label: '', separator: true },
        { icon: '🗑️', label: '删除' },
    ];

    return h('div', { className: 'toolbar' },
        tools.map((t, i) => {
            if (t.separator) {
                return h('div', {
                    style: 'width: 1px; height: 20px; background: #ddd; margin: 0 5px;'
                });
            }
            return h('button', {
                className: 'tool-btn',
                title: t.label,
                onClick: () => console.log('Toolbar action:', t.label)
            }, t.icon + ' ' + t.label);
        })
    );
};

const Sidebar = ({ activeView, onViewChange }) => {
    const items = [
        { id: 'dashboard', icon: '📊', label: '概览 (Dashboard)' },
        { id: 'documents', icon: '📝', label: '文档 (Documents)' },
        { id: 'reports', icon: '📈', label: '报表 (Reports)' },
        { id: 'users', icon: '👥', label: '用户 (Users)' },
        { id: 'settings', icon: '⚙️', label: '设置 (Settings)' },
    ];

    return h('div', { className: 'sidebar' },
        h('div', { className: 'sidebar-header' }, 'MBink App'),
        items.map(item =>
            h('div', {
                className: `sidebar-item ${activeView === item.id ? 'active' : ''}`,
                onClick: () => onViewChange(item.id)
            },
                h('span', { className: 'sidebar-icon' }, item.icon),
                h('span', null, item.label)
            )
        )
    );
};

const DashboardView = () => h('div', null,
    h('div', { className: 'view-header' },
        h('h2', { className: 'view-title' }, '概览 Dashboard')
    ),
    h('div', { className: 'dashboard-grid' },
        h('div', { className: 'card' },
            h('h3', null, '活跃用户'),
            h('div', { className: 'value' }, '1,234')
        ),
        h('div', { className: 'card' },
            h('h3', null, '今日收入'),
            h('div', { className: 'value' }, '¥ 8,888')
        ),
        h('div', { className: 'card' },
            h('h3', null, '待处理任务'),
            h('div', { className: 'value' }, '42')
        ),
        h('div', { className: 'card' },
            h('h3', null, '系统负载'),
            h('div', { className: 'value' }, '35%')
        )
    ),
    h('div', { style: 'margin-top: 24px;' },
        h('h3', { style: 'color: #2c3e50; margin-bottom: 15px;' }, '最近活动'),
        h('div', { style: 'background: white; border: 1px solid #eee; padding: 15px; border-radius: 8px;' },
            '图表区域预留位置...'
        )
    )
);

const DocumentsView = () => h('div', null,
    h('div', { className: 'view-header' },
        h('h2', { className: 'view-title' }, '我的文档')
    ),
    h('ul', { className: 'doc-list' },
        [
            { name: '项目计划书 2025.docx', date: '2024-12-13', size: '2.4 MB', type: 'docx' },
            { name: '年度预算表.xlsx', date: '2024-12-12', size: '1.1 MB', type: 'xlsx' },
            { name: '会议纪要 (周一).txt', date: '2024-12-10', size: '4 KB', type: 'txt' },
            { name: '系统架构图.png', date: '2024-12-08', size: '3.5 MB', type: 'img' },
            { name: 'API 接口文档.md', date: '2024-12-01', size: '45 KB', type: 'md' }
        ].map(doc =>
            h('li', { className: 'doc-item' },
                h('span', { style: 'font-size: 20px;' }, doc.type === 'img' ? '🖼️' : '📄'),
                h('div', { style: 'flex: 1;' },
                    h('div', { style: 'font-weight: 500;' }, doc.name),
                    h('div', { style: 'font-size: 12px; color: #999;' }, `${doc.date} · ${doc.size}`)
                ),
                h('button', { className: 'tool-btn', style: 'padding: 4px 8px;' }, '查看')
            )
        )
    )
);

const SettingsView = () => h('div', null,
    h('div', { className: 'view-header' },
        h('h2', { className: 'view-title' }, '应用设置')
    ),
    h('div', { style: 'max-width: 500px; background: white; padding: 24px; border: 1px solid #eee; border-radius: 8px;' },
        h('div', { className: 'form-group' },
            h('label', null, '用户名'),
            h('input', { type: 'text', className: 'form-control', value: 'Administrator' })
        ),
        h('div', { className: 'form-group' },
            h('label', null, '电子邮件'),
            h('input', { type: 'email', className: 'form-control', value: 'admin@mbink.com' })
        ),
        h('div', { className: 'form-group' },
            h('label', null, '界面主题'),
            h('select', { className: 'form-control' },
                h('option', null, '浅色模式'),
                h('option', null, '深色模式'),
                h('option', null, '跟随系统')
            )
        ),
        h('div', { className: 'form-group', style: 'display: flex; align-items: center; gap: 10px;' },
            h('input', { type: 'checkbox', id: 'notif', checked: true }),
            h('label', { for: 'notif', style: 'margin: 0;' }, '启用桌面通知')
        ),
        h('div', { style: 'margin-top: 20px;' },
            h('button', { className: 'btn-primary' }, '保存更改')
        )
    )
);

const MainView = ({ view }) => {
    let Content;
    switch (view) {
        case 'dashboard': Content = DashboardView; break;
        case 'documents': Content = DocumentsView; break;
        case 'settings': Content = SettingsView; break;
        default: Content = () => h('div', { style: 'padding: 24px;' }, h('h2', null, '开发中 / Under Construction'));
    }
    return h('div', { className: 'main-view' }, h(Content));
};

const StatusBar = () => {
    const [time, setTime] = useState(new Date().toLocaleTimeString());

    useEffect(() => {
        const timer = setInterval(() => setTime(new Date().toLocaleTimeString()), 1000);
        return () => clearInterval(timer);
    }, []);

    return h('div', { className: 'statusbar' },
        h('span', null, '就绪 Ready'),
        h('div', { style: 'display: flex; gap: 15px;' },
            h('span', null, 'UTF-8'),
            h('span', null, 'Ln 1, Col 1'),
            h('span', null, time)
        )
    );
};

// --- App Root ---

const App = () => {
    // 注入样式
    useEffect(() => {
        const styleEl = document.createElement('style');
        styleEl.textContent = STYLES;
        document.head.appendChild(styleEl);
        console.log('Styles injected successfully');
    }, []);

    const [activeView, setActiveView] = useState('dashboard');

    return h('div', { className: 'app-container' },
        h(MenuBar),
        h(Toolbar),
        h('div', { className: 'content-area' },
            h(Sidebar, { activeView, onViewChange: setActiveView }),
            h(MainView, { view: activeView })
        ),
        h(StatusBar)
    );
};

// 渲染应用
try {
    const root = document.createElement('div');
    root.id = 'app-root';
    document.body.appendChild(root);

    // 清除默认的 body margin 如果样式注入稍微延迟
    document.body.style.margin = '0';
    document.body.style.overflow = 'hidden';

    render(h(App), root);
    console.log('Desktop App Example loaded successfully');
} catch (e) {
    console.error('Failed to render app:', e);
}
