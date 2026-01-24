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
        app.load_js_file("./sidebar_app.js")
        
        print("\n✅ 侧边菜单应用已启动")
        print("📝 点击左侧菜单切换页面")
        
        app.run()


if __name__ == "__main__":
    main()
