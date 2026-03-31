"""
MBink 无边框窗口示例

演示功能：
- borderless=True 无边框窗口
- -webkit-app-region: drag 标题栏拖拽
- -webkit-window-control 窗口控制按钮（最小化/最大化/关闭/置顶）
- 窗口边缘调整大小
- DWM 阴影效果

运行方式：
    python borderless_demo.py
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import mbink as ui

app = ui.App("无边框窗口", 800, 600, borderless=True, resizable=True, gpu=False)

app.load_html("""
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body {
      width: 100%; height: 100%;
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
    .btn-pin      { background: #59a8f8; -webkit-window-control: pin; }
    .btn-minimize { background: #febc2e; -webkit-window-control: minimize; }
    .btn-maximize { background: #28c840; -webkit-window-control: maximize; }
    .btn-close    { background: #ff5f57; -webkit-window-control: close; }
    .content {
      flex: 1;
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
      <span class="titlebar-text">Borderless Window Demo (Python)</span>
      <div class="titlebar-buttons">
        <div class="btn-control btn-pin"></div>
        <div class="btn-control btn-minimize"></div>
        <div class="btn-control btn-maximize"></div>
        <div class="btn-control btn-close"></div>
      </div>
    </div>
    <div class="content">
      <h1>无边框窗口 🎉</h1>
      <p>Python 端通过 borderless=True 创建无边框窗口</p>
      <div class="feature-list">
        <div class="feature-item"><span class="feature-check">✓</span> 标题栏拖拽移动窗口</div>
        <div class="feature-item"><span class="feature-check">✓</span> 窗口控制按钮（置顶/最小化/最大化/关闭）</div>
        <div class="feature-item"><span class="feature-check">✓</span> 窗口边缘调整大小</div>
        <div class="feature-item"><span class="feature-check">✓</span> DWM 阴影效果</div>
        <div class="feature-item"><span class="feature-check">✓</span> 圆角窗口样式</div>
      </div>
    </div>
    <div class="footer">MBink Python Borderless Window</div>
  </div>
</body>
</html>
""")

app.run()

# cd D:\code\C\MBink\bindings\python
# pip install -e .
# pyinstaller -F examples\borderless_demo.py --add-binary="mbink\bin\mbink.dll;mbink\bin"