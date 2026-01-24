"""
Preact Todo 纯 JS 版本测试
使用 esm_loader 加载，测试 checkbox 重绘问题
"""
import sys
import os

# 添加模块路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import Window, Document
from lightui.esm_loader import ESMLoader

def main():
    # 创建窗口
    window = Window("Preact Todo - 纯JS测试", 600, 700)
    doc = window.document
    
    # 创建 ESM 加载器
    loader = ESMLoader(window)
    
    # 获取脚本目录
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 加载 Preact
    preact_path = os.path.join(script_dir, 'preact.min.js')
    loader.load_script(preact_path)
    
    # 加载 Preact Hooks
    hooks_path = os.path.join(script_dir, 'preact_hooks.min.js')
    loader.load_script(hooks_path)
    
    # 加载应用代码
    app_path = os.path.join(script_dir, 'preact_todo_pure.js')
    loader.load_script(app_path)
    
    # 注入样式并渲染
    loader.eval_script("""
        // 注入样式
        const styleEl = document.createElement('style');
        styleEl.textContent = styles;
        document.head.appendChild(styleEl);
        
        // 渲染应用
        const container = document.createElement('div');
        container.id = 'app';
        document.body.appendChild(container);
        
        preact.render(preact.h(App), container);
        
        console.log('[Main] App rendered');
    """)
    
    # 运行事件循环
    window.run()

if __name__ == '__main__':
    main()

