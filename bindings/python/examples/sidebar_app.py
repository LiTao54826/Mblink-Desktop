"""
LightUI Python Binding - 侧边菜单桌面应用示例

展示常见的桌面应用布局：
- 左侧固定侧边栏导航
- 右侧内容区域
- 菜单切换交互
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 50)
    print("LightUI 侧边菜单桌面应用")
    print("=" * 50)
    
    with LightUIApp("侧边菜单应用", 900, 600) as app:
        # 创建状态
        current_page = app.state("currentPage", "home")
        
        # 绑定页面切换函数
        @app.bind("switchPage")
        def switch_page(page_id):
            """切换页面"""
            print(f"切换到页面: {page_id}")
            current_page.set(page_id)
            return page_id
        
        # 加载 HTML UI
        app.load_html('''
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            display: flex;
            height: 100vh;
            background: #f5f5f5;
        }
        
        /* 侧边栏 */
        .sidebar {
            width: 240px;
            background: linear-gradient(180deg, #1a1a2e 0%, #16213e 100%);
            color: white;
            display: flex;
            flex-direction: column;
            flex-shrink: 0;
        }
        
        .sidebar-header {
            padding: 24px 20px;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        
        .sidebar-header h1 {
            font-size: 20px;
            font-weight: 600;
            color: #e94560;
        }
        
        .sidebar-header p {
            font-size: 12px;
            color: rgba(255,255,255,0.6);
            margin-top: 4px;
        }
        
        .nav-menu {
            flex: 1;
            padding: 16px 0;
        }
        
        .nav-item {
            display: flex;
            align-items: center;
            padding: 12px 20px;
            color: rgba(255,255,255,0.7);
            cursor: pointer;
            transition: all 0.2s;
            border-left: 3px solid transparent;
        }
        
        .nav-item:hover {
            background: rgba(255,255,255,0.05);
            color: white;
        }
        
        .nav-item.active {
            background: rgba(233, 69, 96, 0.15);
            color: #e94560;
            border-left-color: #e94560;
        }
        
        .nav-icon {
            width: 20px;
            margin-right: 12px;
            text-align: center;
        }
        
        .nav-section {
            padding: 16px 20px 8px;
            font-size: 11px;
            text-transform: uppercase;
            color: rgba(255,255,255,0.4);
            letter-spacing: 1px;
        }
        
        .sidebar-footer {
            padding: 16px 20px;
            border-top: 1px solid rgba(255,255,255,0.1);
        }
        
        .user-info {
            display: flex;
            align-items: center;
        }
        
        .user-avatar {
            width: 36px;
            height: 36px;
            border-radius: 50%;
            background: linear-gradient(135deg, #e94560, #0f3460);
            display: flex;
            align-items: center;
            justify-content: center;
            font-weight: 600;
            margin-right: 12px;
        }
        
        .user-name {
            font-size: 14px;
            font-weight: 500;
        }
        
        .user-role {
            font-size: 12px;
            color: rgba(255,255,255,0.5);
        }
        
        /* 主内容区 */
        .main-content {
            flex: 1;
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }
        
        .header {
            height: 60px;
            background: white;
            border-bottom: 1px solid #e0e0e0;
            display: flex;
            align-items: center;
            padding: 0 24px;
            justify-content: space-between;
        }
        
        .header-title {
            font-size: 18px;
            font-weight: 600;
            color: #333;
        }
        
        .header-actions {
            display: flex;
            gap: 12px;
        }
        
        .header-btn {
            padding: 8px 16px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            transition: all 0.2s;
        }
        
        .header-btn.primary {
            background: #e94560;
            color: white;
        }
        
        .header-btn.primary:hover {
            background: #d63850;
        }
        
        .header-btn.secondary {
            background: #f0f0f0;
            color: #333;
        }
        
        .header-btn.secondary:hover {
            background: #e0e0e0;
        }
        
        .content {
            flex: 1;
            padding: 24px;
            overflow-y: auto;
        }
        
        .page {
            display: none;
        }
        
        .page.active {
            display: block;
        }
        
        /* 卡片样式 */
        .card {
            background: white;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 20px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.05);
        }
        
        .card-title {
            font-size: 16px;
            font-weight: 600;
            color: #333;
            margin-bottom: 16px;
        }
        
        .card-content {
            color: #666;
            line-height: 1.6;
        }
        
        /* 统计卡片 */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
            margin-bottom: 24px;
        }
        
        .stat-card {
            background: white;
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.05);
        }
        
        .stat-value {
            font-size: 32px;
            font-weight: 700;
            color: #333;
        }
        
        .stat-label {
            font-size: 14px;
            color: #888;
            margin-top: 4px;
        }
        
        .stat-card.highlight {
            background: linear-gradient(135deg, #e94560, #0f3460);
            color: white;
        }
        
        .stat-card.highlight .stat-value,
        .stat-card.highlight .stat-label {
            color: white;
        }
        
        /* 列表样式 */
        .list-item {
            display: flex;
            align-items: center;
            padding: 12px 0;
            border-bottom: 1px solid #f0f0f0;
        }
        
        .list-item:last-child {
            border-bottom: none;
        }
        
        .list-icon {
            width: 40px;
            height: 40px;
            border-radius: 8px;
            background: #f5f5f5;
            display: flex;
            align-items: center;
            justify-content: center;
            margin-right: 12px;
        }
        
        .list-info {
            flex: 1;
        }
        
        .list-title {
            font-weight: 500;
            color: #333;
        }
        
        .list-subtitle {
            font-size: 13px;
            color: #888;
        }
        
        .list-badge {
            padding: 4px 10px;
            border-radius: 12px;
            font-size: 12px;
            font-weight: 500;
        }
        
        .badge-success {
            background: #e8f5e9;
            color: #2e7d32;
        }
        
        .badge-warning {
            background: #fff3e0;
            color: #ef6c00;
        }
        
        .badge-info {
            background: #e3f2fd;
            color: #1565c0;
        }
    </style>
</head>
<body>
    <!-- 侧边栏 -->
    <div class="sidebar">
        <div class="sidebar-header">
            <h1>🚀 LightUI</h1>
            <p>Python 桌面应用框架</p>
        </div>
        
        <div class="nav-menu">
            <div class="nav-section">主菜单</div>
            <div class="nav-item active" id="nav-home" onclick="navigateTo('home')">
                <span class="nav-icon">🏠</span>
                <span>首页</span>
            </div>
            <div class="nav-item" id="nav-dashboard" onclick="navigateTo('dashboard')">
                <span class="nav-icon">📊</span>
                <span>数据面板</span>
            </div>
            <div class="nav-item" id="nav-projects" onclick="navigateTo('projects')">
                <span class="nav-icon">📁</span>
                <span>项目管理</span>
            </div>
            
            <div class="nav-section">系统</div>
            <div class="nav-item" id="nav-settings" onclick="navigateTo('settings')">
                <span class="nav-icon">⚙️</span>
                <span>设置</span>
            </div>
            <div class="nav-item" id="nav-help" onclick="navigateTo('help')">
                <span class="nav-icon">❓</span>
                <span>帮助</span>
            </div>
        </div>
        
        <div class="sidebar-footer">
            <div class="user-info">
                <div class="user-avatar">U</div>
                <div>
                    <div class="user-name">用户</div>
                    <div class="user-role">管理员</div>
                </div>
            </div>
        </div>
    </div>
    
    <!-- 主内容区 -->
    <div class="main-content">
        <div class="header">
            <div class="header-title" id="page-title">首页</div>
            <div class="header-actions">
                <button class="header-btn secondary">刷新</button>
                <button class="header-btn primary">新建</button>
            </div>
        </div>
        
        <div class="content">
            <!-- 首页 -->
            <div class="page active" id="page-home">
                <div class="stats-grid">
                    <div class="stat-card highlight">
                        <div class="stat-value">1,234</div>
                        <div class="stat-label">总访问量</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">56</div>
                        <div class="stat-label">活跃项目</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value">89%</div>
                        <div class="stat-label">完成率</div>
                    </div>
                </div>
                
                <div class="card">
                    <div class="card-title">欢迎使用 LightUI</div>
                    <div class="card-content">
                        这是一个使用 Python + LightUI 构建的桌面应用示例。
                        左侧是导航菜单，点击可以切换不同的页面。
                        LightUI 让你可以使用熟悉的 HTML/CSS 来构建漂亮的桌面应用界面。
                    </div>
                </div>
            </div>
            
            <!-- 数据面板 -->
            <div class="page" id="page-dashboard">
                <div class="card">
                    <div class="card-title">数据概览</div>
                    <div class="card-content">
                        <div class="list-item">
                            <div class="list-icon">📈</div>
                            <div class="list-info">
                                <div class="list-title">日活用户</div>
                                <div class="list-subtitle">较昨日增长 12%</div>
                            </div>
                            <span class="list-badge badge-success">+12%</span>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">💰</div>
                            <div class="list-info">
                                <div class="list-title">收入统计</div>
                                <div class="list-subtitle">本月累计</div>
                            </div>
                            <span class="list-badge badge-info">¥12,345</span>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">⚡</div>
                            <div class="list-info">
                                <div class="list-title">系统负载</div>
                                <div class="list-subtitle">CPU 使用率</div>
                            </div>
                            <span class="list-badge badge-warning">45%</span>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- 项目管理 -->
            <div class="page" id="page-projects">
                <div class="card">
                    <div class="card-title">项目列表</div>
                    <div class="card-content">
                        <div class="list-item">
                            <div class="list-icon">🎨</div>
                            <div class="list-info">
                                <div class="list-title">UI 重构项目</div>
                                <div class="list-subtitle">进行中 · 3 人参与</div>
                            </div>
                            <span class="list-badge badge-warning">进行中</span>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">🔧</div>
                            <div class="list-info">
                                <div class="list-title">后端优化</div>
                                <div class="list-subtitle">已完成 · 2 人参与</div>
                            </div>
                            <span class="list-badge badge-success">已完成</span>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">📱</div>
                            <div class="list-info">
                                <div class="list-title">移动端适配</div>
                                <div class="list-subtitle">待开始 · 1 人参与</div>
                            </div>
                            <span class="list-badge badge-info">待开始</span>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- 设置 -->
            <div class="page" id="page-settings">
                <div class="card">
                    <div class="card-title">应用设置</div>
                    <div class="card-content">
                        <div class="list-item">
                            <div class="list-icon">🌙</div>
                            <div class="list-info">
                                <div class="list-title">深色模式</div>
                                <div class="list-subtitle">切换应用主题</div>
                            </div>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">🔔</div>
                            <div class="list-info">
                                <div class="list-title">通知设置</div>
                                <div class="list-subtitle">管理推送通知</div>
                            </div>
                        </div>
                        <div class="list-item">
                            <div class="list-icon">🌐</div>
                            <div class="list-info">
                                <div class="list-title">语言设置</div>
                                <div class="list-subtitle">简体中文</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- 帮助 -->
            <div class="page" id="page-help">
                <div class="card">
                    <div class="card-title">帮助中心</div>
                    <div class="card-content">
                        LightUI 是一个轻量级的 Python 桌面应用框架，
                        让你可以使用 HTML/CSS/JS 来构建跨平台的桌面应用。
                        <br><br>
                        <strong>特性：</strong>
                        <br>• 使用熟悉的 Web 技术构建 UI
                        <br>• Python 后端逻辑处理
                        <br>• 双向通信（Python ↔ JS）
                        <br>• 状态管理
                        <br>• 跨平台支持
                    </div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        var pageTitles = {
            'home': '首页',
            'dashboard': '数据面板',
            'projects': '项目管理',
            'settings': '设置',
            'help': '帮助'
        };
        
        function navigateTo(pageId) {
            // 调用 Python 函数
            host.call('switchPage', pageId);
            
            // 更新导航高亮 - 使用 style 而不是 className
            var navItems = document.querySelectorAll('.nav-item');
            for (var i = 0; i < navItems.length; i++) {
                var item = navItems[i];
                var isActive = item.id === 'nav-' + pageId;
                item.style.background = isActive ? 'rgba(233, 69, 96, 0.15)' : 'transparent';
                item.style.color = isActive ? '#e94560' : 'rgba(255,255,255,0.7)';
                item.style.borderLeftColor = isActive ? '#e94560' : 'transparent';
            }
            
            // 切换页面显示 - 使用 style.display
            var pages = document.querySelectorAll('.page');
            for (var i = 0; i < pages.length; i++) {
                pages[i].style.display = 'none';
            }
            var activePage = document.getElementById('page-' + pageId);
            if (activePage) {
                activePage.style.display = 'block';
            }
            
            // 更新标题
            var titleEl = document.getElementById('page-title');
            if (titleEl && pageTitles[pageId]) {
                titleEl.textContent = pageTitles[pageId];
            }
        }
        
        console.log('侧边菜单应用已加载');
    </script>
</body>
</html>
        ''')
        
        print("\n✅ 侧边菜单应用已启动")
        print("📝 点击左侧菜单切换页面")
        
        app.run()


if __name__ == "__main__":
    main()
