"""
LightUI 待办事项示例

演示功能：
- host.on(stateName, callback) 自动响应列表状态变化重新渲染
- ListState 列表状态管理
- @app.bindable 多参数绑定
- py.funcName() 调用 Python 函数（无需返回值）

运行方式：
    python todo_app.py
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import lightui as ui

app = ui.App("LightUI 待办事项", 500, 450)
todos = app.state("todos", [])


@app.bindable
def addTodo(text):
    text = str(text) if text else ""
    if text.strip():
        todos.append(text.strip())


@app.bindable
def removeTodo(index):
    index = int(index) if index is not None else -1
    if 0 <= index < len(todos):
        todos.remove(index)


app.load_html("""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <title>Borderless Window Demo</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }

    html, body {
      width: 100%; height: 100%;
      background: transparent;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      overflow: hidden;
    }

    .window-frame {
      width: 100%; height: 100%;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      border-radius: 12px;
      display: flex;
      flex-direction: column;
      overflow: hidden;
      box-shadow: 0 8px 32px rgba(0,0,0,0.3);
    }

    /* 标题栏 - 可拖拽区域 */
    .titlebar {
      -webkit-app-region: drag;
      height: 40px;
      display: flex;
      align-items: center;
      padding: 0 12px;
      background: rgba(0,0,0,0.15);
      color: white;
      font-size: 13px;
      font-weight: 600;
      user-select: none;
    }

    .titlebar-icon { margin-right: 8px; font-size: 16px; }
    .titlebar-text { flex: 1; }

    /* 窗口控制按钮 - 不可拖拽 */
    .titlebar-buttons {
      -webkit-app-region: no-drag;
      display: flex;
      gap: 8px;
    }

    .btn-control {
      width: 14px; height: 14px;
      border-radius: 50%;
      border: none;
      cursor: pointer;
    }
    .btn-minimize { background: #febc2e; -webkit-window-control: minimize; }
    .btn-maximize { background: #28c840; -webkit-window-control: maximize; }
    .btn-close    { background: #ff5f57; -webkit-window-control: close; }

    /* 内容区域 */
    .content {
      flex: 1;
      min-height: 0;
      overflow: hidden;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      color: white;
      padding: 24px;
    }

    .content h1 {
      font-size: 28px;
      margin-bottom: 12px;
      text-shadow: 0 2px 8px rgba(0,0,0,0.2);
    }

    .content p {
      font-size: 14px;
      opacity: 0.85;
      text-align: center;
      line-height: 1.6;
      max-width: 400px;
    }

    .feature-list {
      margin-top: 20px;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .feature-item {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 13px;
      opacity: 0.9;
    }

    .feature-check { color: #28c840; }

    .footer {
      padding: 10px;
      text-align: center;
      font-size: 11px;
      opacity: 0.5;
      color: white;
    }
  </style>
</head>
<body>
  <div class="window-frame">
    <div class="titlebar">
      <span class="titlebar-icon">🪟</span>
      <span class="titlebar-text">Borderless Window Demo</span>
      <div class="titlebar-buttons">
        <div class="btn-control btn-minimize"></div>
        <div class="btn-control btn-maximize"></div>
        <div class="btn-control btn-close"></div>
      </div>
    </div>
    <div class="content">
      <h1>无边框窗口 🎉</h1>
      <p>这是一个使用 CSS <code>-webkit-app-region</code> 实现的无边框窗口演示。</p>
      <div class="feature-list">
        <div class="feature-item"><span class="feature-check">✓</span> 标题栏拖拽移动窗口</div>
        <div class="feature-item"><span class="feature-check">✓</span> 按钮区域不触发拖拽</div>
        <div class="feature-item"><span class="feature-check">✓</span> 窗口边缘调整大小</div>
        <div class="feature-item"><span class="feature-check">✓</span> DWM 阴影效果</div>
        <div class="feature-item"><span class="feature-check">✓</span> 圆角窗口样式</div>
      </div>
    </div>
    <div class="footer">LightUI Borderless Window System</div>
  </div>
  <script>
  </script>
</body>
</html>


""")

app.run()
