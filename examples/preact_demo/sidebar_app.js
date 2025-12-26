/**
 * @file sidebar_app.js
 * @brief Sidebar Desktop App Demo
 * Usage: app_loader.exe examples/preact_demo/sidebar_app.js
 */

(function () {
    'use strict';

    var h = Preact.h;
    var render = Preact.render;
    var useState = PreactHooks.useState;

    // Colors
    var PRIMARY = '#3498db';
    var SUCCESS = '#27ae60';
    var DANGER = '#e74c3c';
    var SIDEBAR_BG = '#2c3e50';

    var WARNING = '#f39c12';
    var INFO = '#9b59b6';

    // Menu items
    var MENU = [
        { id: 'home', icon: '[H]', text: 'Home' },
        { id: 'layout', icon: '[L]', text: 'Layout Tests' },
        { id: 'users', icon: '[U]', text: 'Users' },
        { id: 'files', icon: '[F]', text: 'Files' },
        { id: 'settings', icon: '[S]', text: 'Settings' }
    ];

    // Set body styles to hide default scrollbar
    document.body.style.margin = '0';
    document.body.style.padding = '0';
    document.body.style.overflow = 'hidden';

    // Sidebar component
    function Sidebar(props) {
        var items = [];
        for (var i = 0; i < MENU.length; i++) {
            var item = MENU[i];
            var isActive = props.activePage === item.id;
            items.push(
                h('div', {
                    key: item.id,
                    style: {
                        padding: '12px 16px',
                        cursor: 'pointer',
                        backgroundColor: isActive ? 'rgba(52,152,219,0.3)' : 'transparent',
                        borderLeft: isActive ? '3px solid ' + PRIMARY : '3px solid transparent',
                        color: isActive ? PRIMARY : '#bdc3c7'
                    },
                    onClick: (function(id) {
                        return function() { props.onPageChange(id); };
                    })(item.id)
                }, item.icon + ' ' + item.text)
            );
        }

        return h('div', {
            style: {
                width: '200px',
                minWidth: '200px',
                backgroundColor: SIDEBAR_BG,
                color: '#ecf0f1',
                display: 'flex',
                flexDirection: 'column'
            }
        },
            h('div', { style: { padding: '16px', fontSize: '18px', fontWeight: 'bold', borderBottom: '1px solid rgba(255,255,255,0.1)' } }, 'Dashboard'),
            h('div', { style: { paddingTop: '8px', flex: 1, overflowY: 'auto', overflowX: 'hidden' } }, items),
            h('div', { style: { padding: '16px', borderTop: '1px solid rgba(255,255,255,0.1)', fontSize: '13px' } }, 'User: Admin')
        );
    }

    // Home page
    function HomePage() {
        return h('div', null,
            h('h2', { style: { marginTop: 0, color: '#333' } }, 'Welcome!'),
            h('p', { style: { color: '#666' } }, 'This is your dashboard overview.'),
            h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', marginBottom: '16px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
                h('h3', { style: { marginTop: 0 } }, 'Statistics'),
                h('div', { style: { display: 'flex' } },
                    h('div', { style: { flex: 1, textAlign: 'center', padding: '10px' } },
                        h('div', { style: { fontSize: '28px', fontWeight: 'bold', color: PRIMARY } }, '1,234'),
                        h('div', { style: { fontSize: '12px', color: '#888' } }, 'Visits')
                    ),
                    h('div', { style: { flex: 1, textAlign: 'center', padding: '10px' } },
                        h('div', { style: { fontSize: '28px', fontWeight: 'bold', color: SUCCESS } }, '567'),
                        h('div', { style: { fontSize: '12px', color: '#888' } }, 'Users')
                    ),
                    h('div', { style: { flex: 1, textAlign: 'center', padding: '10px' } },
                        h('div', { style: { fontSize: '28px', fontWeight: 'bold', color: DANGER } }, '89'),
                        h('div', { style: { fontSize: '12px', color: '#888' } }, 'Orders')
                    )
                )
            ),
            h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
                h('h3', { style: { marginTop: 0 } }, 'Recent Activity'),
                h('div', { style: { padding: '8px 0', borderBottom: '1px solid #eee' } }, '* New user registered - 5 min ago'),
                h('div', { style: { padding: '8px 0', borderBottom: '1px solid #eee' } }, '* Order completed - 15 min ago'),
                h('div', { style: { padding: '8px 0' } }, '* System updated - 1 hour ago')
            )
        );
    }

    // Users page
    function UsersPage() {
        return h('div', null,
            h('h2', { style: { marginTop: 0, color: '#333' } }, 'User Management'),
            h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'John - Admin - Online'),
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Jane - Editor - Online'),
                h('div', { style: { padding: '12px 0' } }, 'Bob - Guest - Offline')
            )
        );
    }

    // Files page
    function FilesPage() {
        return h('div', null,
            h('h2', { style: { marginTop: 0, color: '#333' } }, 'File Management'),
            h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Document.docx - 2.3 MB'),
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Design.psd - 15.8 MB'),
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Report.xlsx - 1.2 MB'),
                h('div', { style: { padding: '12px 0' } }, 'Audio.mp3 - 8.5 MB')
            )
        );
    }

    // Settings page
    function SettingsPage() {
        return h('div', null,
            h('h2', { style: { marginTop: 0, color: '#333' } }, 'Settings'),
            h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Dark Mode - OFF'),
                h('div', { style: { padding: '12px 0', borderBottom: '1px solid #eee' } }, 'Notifications - ON'),
                h('div', { style: { padding: '12px 0' } }, 'Auto Update - ON')
            )
        );
    }

    // ========== Layout Tests Page ==========
    
    // Section wrapper component - uses standard props.children
    function TestSection(props) {
        return h('div', { style: { backgroundColor: '#fff', borderRadius: '8px', padding: '20px', marginBottom: '20px', boxShadow: '0 2px 4px rgba(0,0,0,0.1)' } },
            h('h3', { style: { marginTop: 0, marginBottom: '16px', color: '#333', borderBottom: '2px solid ' + PRIMARY, paddingBottom: '8px' } }, props.title),
            props.children
        );
    }

    // 1. Flexbox Layout Tests
    function FlexboxTests() {
        return h(TestSection, { title: '1. Flexbox 布局测试' },
            // Row with justify-content variations
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'justify-content: space-between'),
                h('div', { style: { display: 'flex', justifyContent: 'space-between', backgroundColor: '#ecf0f1', padding: '10px', borderRadius: '4px' } },
                    h('div', { style: { width: '60px', height: '40px', backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'A'),
                    h('div', { style: { width: '60px', height: '40px', backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'B'),
                    h('div', { style: { width: '60px', height: '40px', backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'C')
                )
            ),
            // Row with align-items variations
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'align-items: center (不同高度)'),
                h('div', { style: { display: 'flex', alignItems: 'center', backgroundColor: '#ecf0f1', padding: '10px', borderRadius: '4px', gap: '10px' } },
                    h('div', { style: { width: '60px', height: '30px', backgroundColor: PRIMARY, borderRadius: '4px' } }),
                    h('div', { style: { width: '60px', height: '50px', backgroundColor: SUCCESS, borderRadius: '4px' } }),
                    h('div', { style: { width: '60px', height: '70px', backgroundColor: WARNING, borderRadius: '4px' } }),
                    h('div', { style: { width: '60px', height: '40px', backgroundColor: DANGER, borderRadius: '4px' } })
                )
            ),
            // Flex grow/shrink
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'flex-grow: 1, 2, 1'),
                h('div', { style: { display: 'flex', backgroundColor: '#ecf0f1', padding: '10px', borderRadius: '4px', gap: '10px' } },
                    h('div', { style: { flex: 1, height: '40px', backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'flex:1'),
                    h('div', { style: { flex: 2, height: '40px', backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'flex:2'),
                    h('div', { style: { flex: 1, height: '40px', backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'flex:1')
                )
            ),
            // Column layout
            h('div', null,
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'flex-direction: column'),
                h('div', { style: { display: 'flex', flexDirection: 'column', backgroundColor: '#ecf0f1', padding: '10px', borderRadius: '4px', gap: '8px', height: '150px' } },
                    h('div', { style: { flex: 1, backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Top'),
                    h('div', { style: { flex: 2, backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Middle (flex:2)'),
                    h('div', { style: { flex: 1, backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Bottom')
                )
            )
        );
    }

    // 2. Absolute Positioning Tests
    function AbsolutePositionTests() {
        return h(TestSection, { title: '2. 绝对定位测试' },
            // Basic absolute positioning
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '基础绝对定位 (四角)'),
                h('div', { style: { position: 'relative', height: '120px', backgroundColor: '#ecf0f1', borderRadius: '4px', border: '2px dashed #bdc3c7' } },
                    h('div', { style: { position: 'absolute', top: '8px', left: '8px', width: '40px', height: '40px', backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, 'TL'),
                    h('div', { style: { position: 'absolute', top: '8px', right: '8px', width: '40px', height: '40px', backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, 'TR'),
                    h('div', { style: { position: 'absolute', bottom: '8px', left: '8px', width: '40px', height: '40px', backgroundColor: WARNING, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, 'BL'),
                    h('div', { style: { position: 'absolute', bottom: '8px', right: '8px', width: '40px', height: '40px', backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, 'BR'),
                    h('div', { style: { position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', width: '50px', height: '50px', backgroundColor: INFO, borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, 'Center')
                )
            ),
            // Overlapping elements
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'z-index 层叠'),
                h('div', { style: { position: 'relative', height: '100px', backgroundColor: '#ecf0f1', borderRadius: '4px' } },
                    h('div', { style: { position: 'absolute', top: '10px', left: '10px', width: '80px', height: '80px', backgroundColor: PRIMARY, borderRadius: '4px', zIndex: 1, opacity: 0.9 } }),
                    h('div', { style: { position: 'absolute', top: '20px', left: '40px', width: '80px', height: '80px', backgroundColor: SUCCESS, borderRadius: '4px', zIndex: 2, opacity: 0.9 } }),
                    h('div', { style: { position: 'absolute', top: '30px', left: '70px', width: '80px', height: '80px', backgroundColor: DANGER, borderRadius: '4px', zIndex: 3, opacity: 0.9 } })
                )
            ),
            // Badge/notification style
            h('div', null,
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '徽章/通知样式'),
                h('div', { style: { display: 'flex', gap: '30px', padding: '20px' } },
                    h('div', { style: { position: 'relative', width: '50px', height: '50px', backgroundColor: '#ecf0f1', borderRadius: '8px', display: 'flex', alignItems: 'center', justifyContent: 'center' } },
                        h('span', null, '[M]'),
                        h('div', { style: { position: 'absolute', top: '-5px', right: '-5px', width: '20px', height: '20px', backgroundColor: DANGER, borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, '3')
                    ),
                    h('div', { style: { position: 'relative', width: '50px', height: '50px', backgroundColor: '#ecf0f1', borderRadius: '8px', display: 'flex', alignItems: 'center', justifyContent: 'center' } },
                        h('span', null, '[N]'),
                        h('div', { style: { position: 'absolute', top: '-5px', right: '-5px', width: '20px', height: '20px', backgroundColor: SUCCESS, borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, '!')
                    )
                )
            )
        );
    }

    // 3. Viewport Units Tests
    function ViewportUnitsTests() {
        return h(TestSection, { title: '3. 视口单位测试' },
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'vw 单位宽度'),
                h('div', { style: { width: '50vw', height: '30px', backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '50vw'),
                h('div', { style: { width: '30vw', height: '30px', backgroundColor: SUCCESS, borderRadius: '4px', marginTop: '8px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '30vw'),
                h('div', { style: { width: '20vw', height: '30px', backgroundColor: WARNING, borderRadius: '4px', marginTop: '8px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '20vw')
            ),
            h('div', { style: { marginBottom: '16px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'vh 单位高度'),
                h('div', { style: { display: 'flex', gap: '10px', height: '15vh' } },
                    h('div', { style: { flex: 1, height: '15vh', backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '15vh'),
                    h('div', { style: { flex: 1, height: '10vh', backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '10vh'),
                    h('div', { style: { flex: 1, height: '5vh', backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, '5vh')
                )
            ),
            h('div', null,
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, 'vmin/vmax 单位'),
                h('div', { style: { display: 'flex', gap: '10px' } },
                    h('div', { style: { width: '15vmin', height: '15vmin', backgroundColor: INFO, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, '15vmin'),
                    h('div', { style: { width: '10vmax', height: '10vmax', backgroundColor: WARNING, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, '10vmax')
                )
            )
        );
    }

    // 4. Grid-like Layout Tests
    function GridLayoutTests() {
        var cards = [];
        for (var i = 1; i <= 6; i++) {
            var colors = [PRIMARY, SUCCESS, WARNING, DANGER, INFO, SIDEBAR_BG];
            cards.push(
                h('div', { key: i, style: { backgroundColor: colors[i-1], borderRadius: '8px', padding: '20px', color: '#fff', minHeight: '80px', display: 'flex', alignItems: 'center', justifyContent: 'center' } },
                    h('div', { style: { textAlign: 'center' } },
                        h('div', { style: { fontSize: '24px', fontWeight: 'bold' } }, 'Card ' + i),
                        h('div', { style: { fontSize: '12px', opacity: 0.8 } }, 'Description')
                    )
                )
            );
        }

        return h(TestSection, { title: '4. 网格布局 (Flexbox 模拟)' },
            h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '3列卡片网格'),
            h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: '16px' } },
                cards.map(function(card, idx) {
                    return h('div', { key: idx, style: { flex: '1 1 calc(33.333% - 16px)', minWidth: '150px' } }, card);
                })
            )
        );
    }

    // 5. Nested Layout Tests
    function NestedLayoutTests() {
        return h(TestSection, { title: '5. 嵌套布局测试' },
            h('div', { style: { display: 'flex', gap: '16px', height: '200px' } },
                // Left panel
                h('div', { style: { flex: 1, backgroundColor: '#ecf0f1', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column' } },
                    h('div', { style: { fontWeight: 'bold', marginBottom: '8px', color: '#333' } }, 'Left Panel'),
                    h('div', { style: { flex: 1, display: 'flex', flexDirection: 'column', gap: '8px' } },
                        h('div', { style: { flex: 1, backgroundColor: PRIMARY, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Item 1'),
                        h('div', { style: { flex: 1, backgroundColor: SUCCESS, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Item 2'),
                        h('div', { style: { flex: 1, backgroundColor: WARNING, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'Item 3')
                    )
                ),
                // Right panel with nested flex
                h('div', { style: { flex: 2, backgroundColor: '#ecf0f1', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column' } },
                    h('div', { style: { fontWeight: 'bold', marginBottom: '8px', color: '#333' } }, 'Right Panel (嵌套)'),
                    h('div', { style: { flex: 1, display: 'flex', gap: '8px' } },
                        h('div', { style: { flex: 1, display: 'flex', flexDirection: 'column', gap: '8px' } },
                            h('div', { style: { flex: 1, backgroundColor: DANGER, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'A'),
                            h('div', { style: { flex: 1, backgroundColor: INFO, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'B')
                        ),
                        h('div', { style: { flex: 1, backgroundColor: SIDEBAR_BG, borderRadius: '4px', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'C (Full)')
                    )
                )
            )
        );
    }

    // 6. Common UI Components
    function CommonUITests() {
        return h(TestSection, { title: '6. 常用 UI 组件场景' },
            // Form layout
            h('div', { style: { marginBottom: '20px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '表单布局'),
                h('div', { style: { display: 'flex', flexDirection: 'column', gap: '12px', maxWidth: '300px' } },
                    h('div', { style: { display: 'flex', alignItems: 'center' } },
                        h('label', { style: { width: '80px', fontSize: '14px', color: '#333' } }, 'Username'),
                        h('div', { style: { flex: 1, height: '32px', backgroundColor: '#fff', border: '1px solid #ddd', borderRadius: '4px', padding: '0 8px', display: 'flex', alignItems: 'center', color: '#999' } }, 'input...')
                    ),
                    h('div', { style: { display: 'flex', alignItems: 'center' } },
                        h('label', { style: { width: '80px', fontSize: '14px', color: '#333' } }, 'Password'),
                        h('div', { style: { flex: 1, height: '32px', backgroundColor: '#fff', border: '1px solid #ddd', borderRadius: '4px', padding: '0 8px', display: 'flex', alignItems: 'center', color: '#999' } }, '******')
                    ),
                    h('div', { style: { display: 'flex', justifyContent: 'flex-end', gap: '8px', marginTop: '8px' } },
                        h('div', { style: { padding: '8px 16px', backgroundColor: '#ecf0f1', borderRadius: '4px', cursor: 'pointer' } }, 'Cancel'),
                        h('div', { style: { padding: '8px 16px', backgroundColor: PRIMARY, color: '#fff', borderRadius: '4px', cursor: 'pointer' } }, 'Submit')
                    )
                )
            ),
            // Card with header/body/footer
            h('div', { style: { marginBottom: '20px' } },
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '卡片组件 (Header/Body/Footer)'),
                h('div', { style: { border: '1px solid #ddd', borderRadius: '8px', overflow: 'hidden', maxWidth: '300px' } },
                    h('div', { style: { backgroundColor: PRIMARY, color: '#fff', padding: '12px 16px', fontWeight: 'bold' } }, 'Card Header'),
                    h('div', { style: { padding: '16px', backgroundColor: '#fff' } },
                        h('p', { style: { margin: 0, color: '#666' } }, 'This is the card body content. It can contain any elements.')
                    ),
                    h('div', { style: { backgroundColor: '#f8f9fa', padding: '12px 16px', borderTop: '1px solid #ddd', display: 'flex', justifyContent: 'flex-end', gap: '8px' } },
                        h('div', { style: { padding: '6px 12px', backgroundColor: SUCCESS, color: '#fff', borderRadius: '4px', fontSize: '12px' } }, 'Action')
                    )
                )
            ),
            // Navigation bar
            h('div', null,
                h('div', { style: { fontSize: '12px', color: '#666', marginBottom: '8px' } }, '导航栏'),
                h('div', { style: { display: 'flex', alignItems: 'center', backgroundColor: SIDEBAR_BG, padding: '12px 16px', borderRadius: '8px' } },
                    h('div', { style: { fontWeight: 'bold', color: '#fff', marginRight: '24px' } }, 'Logo'),
                    h('div', { style: { display: 'flex', gap: '16px', flex: 1 } },
                        h('div', { style: { color: PRIMARY } }, 'Home'),
                        h('div', { style: { color: '#bdc3c7' } }, 'About'),
                        h('div', { style: { color: '#bdc3c7' } }, 'Contact')
                    ),
                    h('div', { style: { display: 'flex', alignItems: 'center', gap: '12px' } },
                        h('div', { style: { position: 'relative' } },
                            h('span', { style: { color: '#fff' } }, '[Bell]'),
                            h('div', { style: { position: 'absolute', top: '-5px', right: '-8px', width: '16px', height: '16px', backgroundColor: DANGER, borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff', fontSize: '10px' } }, '5')
                        ),
                        h('div', { style: { width: '32px', height: '32px', backgroundColor: PRIMARY, borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', color: '#fff' } }, 'U')
                    )
                )
            )
        );
    }

    // 7. Flex + Absolute Combined Tests
    function FlexAbsoluteCombinedTests() {
        return h(TestSection, { title: '7. Flex + 绝对定位组合测试' },
            h('div', { style: { display: 'flex', gap: '16px' } },
                // Flex container with absolute children
                h('div', { style: { flex: 1, position: 'relative', height: '150px', backgroundColor: '#ecf0f1', borderRadius: '8px', display: 'flex', alignItems: 'center', justifyContent: 'center' } },
                    h('div', { style: { display: 'flex', gap: '8px' } },
                        h('div', { style: { width: '40px', height: '40px', backgroundColor: PRIMARY, borderRadius: '4px' } }),
                        h('div', { style: { width: '40px', height: '40px', backgroundColor: SUCCESS, borderRadius: '4px' } }),
                        h('div', { style: { width: '40px', height: '40px', backgroundColor: WARNING, borderRadius: '4px' } })
                    ),
                    h('div', { style: { position: 'absolute', top: '8px', right: '8px', padding: '4px 8px', backgroundColor: DANGER, color: '#fff', borderRadius: '4px', fontSize: '10px' } }, 'Badge'),
                    h('div', { style: { position: 'absolute', bottom: '8px', left: '8px', fontSize: '10px', color: '#666' } }, 'Flex items + absolute badge')
                ),
                // Another combined example
                h('div', { style: { flex: 1, position: 'relative', height: '150px', backgroundColor: '#ecf0f1', borderRadius: '8px', overflow: 'hidden' } },
                    h('div', { style: { position: 'absolute', top: 0, left: 0, right: 0, padding: '8px', backgroundColor: 'rgba(0,0,0,0.5)', color: '#fff', fontSize: '12px' } }, 'Overlay Header'),
                    h('div', { style: { height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center' } },
                        h('div', { style: { fontSize: '24px', color: '#333' } }, 'Content')
                    ),
                    h('div', { style: { position: 'absolute', bottom: 0, left: 0, right: 0, padding: '8px', backgroundColor: 'rgba(0,0,0,0.5)', color: '#fff', fontSize: '12px', display: 'flex', justifyContent: 'space-between' } },
                        h('span', null, 'Footer Left'),
                        h('span', null, 'Footer Right')
                    )
                )
            )
        );
    }

    // Layout Tests Page
    function LayoutTestsPage() {
        return h('div', null,
            h('h2', { style: { marginTop: 0, color: '#333' } }, 'Layout Tests - 布局测试'),
            h('p', { style: { color: '#666', marginBottom: '20px' } }, '测试各种布局场景，包括 Flexbox、绝对定位、视口单位等'),
            h(FlexboxTests),
            h(AbsolutePositionTests),
            h(ViewportUnitsTests),
            h(GridLayoutTests),
            h(NestedLayoutTests),
            h(CommonUITests),
            h(FlexAbsoluteCombinedTests)
        );
    }

    // Page router
    function PageContent(props) {
        if (props.page === 'layout') return h(LayoutTestsPage);
        if (props.page === 'users') return h(UsersPage);
        if (props.page === 'files') return h(FilesPage);
        if (props.page === 'settings') return h(SettingsPage);
        return h(HomePage);
    }

    // Main App
    function App() {
        var state = useState('home');
        var activePage = state[0];
        var setActivePage = state[1];
        var contentRef = PreactHooks.useRef(null);

        // 页面切换时重置滚动位置
        function handlePageChange(page) {
            setActivePage(page);
            // 重置滚动位置到顶部
            if (contentRef.current) {
                contentRef.current.scrollTop = 0;
            }
        }

        return h('div', { style: { display: 'flex', fontFamily: 'Arial, sans-serif', height: '100vh' } },
            h(Sidebar, { activePage: activePage, onPageChange: handlePageChange }),
            h('div', { ref: contentRef, style: { flex: 1, backgroundColor: '#f5f5f5', padding: '24px', overflowY: 'auto', overflowX: 'hidden' } },
                h(PageContent, { page: activePage })
            )
        );
    }

    // Render
    console.log('Starting Sidebar App...');
    render(h(App, null), document.body);
    console.log('Sidebar App rendered!');
})();
