use std::sync::{
    atomic::{AtomicBool, Ordering},
    Arc,
};

use mbink::App;
use serde_json::json;

const HTML: &str = r#"
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <title>MBink Rust Tray Demo</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body {
      width: 100%; height: 100%; overflow: hidden;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: #0f172a;
      color: #e5e7eb;
    }
    .window-frame {
      width: 100%; height: 100%;
      display: flex; flex-direction: column;
      background: linear-gradient(180deg, #111827 0%, #0f172a 100%);
      border: 1px solid rgba(148, 163, 184, 0.18);
      border-radius: 12px;
      overflow: hidden;
      box-shadow: 0 16px 48px rgba(0, 0, 0, 0.35);
    }
    .titlebar {
      -webkit-app-region: drag;
      height: 40px;
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 0 12px;
      background: rgba(15, 23, 42, 0.88);
      border-bottom: 1px solid rgba(148, 163, 184, 0.16);
      user-select: none;
    }
    .titlebar-icon { font-size: 15px; }
    .titlebar-text { flex: 1; font-size: 13px; font-weight: 600; }
    .menubar {
      -webkit-app-region: no-drag;
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .menu-item {
      padding: 4px 10px;
      border-radius: 8px;
      font-size: 12px;
      color: #cbd5e1;
      cursor: default;
    }
    .menu-item:hover { background: rgba(148, 163, 184, 0.14); }
    .window-controls {
      -webkit-app-region: no-drag;
      display: flex;
      gap: 8px;
      margin-left: 8px;
    }
    .btn-control {
      width: 14px; height: 14px;
      border-radius: 50%;
      border: none;
      cursor: pointer;
    }
    .btn-pin { background: #59a8f8; -webkit-window-control: pin; }
    .btn-minimize { background: #fbbf24; -webkit-window-control: minimize; }
    .btn-maximize { background: #22c55e; -webkit-window-control: maximize; }
    .btn-close { background: #ef4444; -webkit-window-control: close; }
    .content {
      flex: 1;
      display: grid;
      place-items: center;
      padding: 24px;
    }
    .panel {
      width: min(100%, 720px);
      background: rgba(15, 23, 42, 0.62);
      border: 1px solid rgba(148, 163, 184, 0.16);
      border-radius: 16px;
      padding: 28px;
      backdrop-filter: blur(10px);
    }
    .panel h1 {
      font-size: 28px;
      margin-bottom: 12px;
    }
    .panel p {
      color: #cbd5e1;
      line-height: 1.7;
      margin-bottom: 18px;
    }
    .feature-list {
      display: grid;
      gap: 10px;
      margin-top: 12px;
    }
    .feature-item {
      display: flex;
      align-items: center;
      gap: 8px;
      color: #dbeafe;
      font-size: 14px;
    }
    .feature-check { color: #22c55e; }
    .footer {
      padding: 10px 14px;
      font-size: 12px;
      color: #94a3b8;
      border-top: 1px solid rgba(148, 163, 184, 0.14);
      background: rgba(15, 23, 42, 0.72);
    }
  </style>
</head>
<body>
  <div class="window-frame">
    <div class="titlebar">
      <span class="titlebar-icon">🦀</span>
      <span class="titlebar-text">MBink Rust Tray Demo</span>
      <div class="menubar">
        <div class="menu-item">文件</div>
        <div class="menu-item">编辑</div>
        <div class="menu-item">视图</div>
        <div class="menu-item">窗口</div>
        <div class="menu-item">帮助</div>
      </div>
      <div class="window-controls">
        <div class="btn-control btn-pin"></div>
        <div class="btn-control btn-minimize"></div>
        <div class="btn-control btn-maximize"></div>
        <div class="btn-control btn-close"></div>
      </div>
    </div>
    <div class="content">
      <div class="panel">
        <h1>Rust Tray Demo</h1>
        <p>这个示例直接照着 Python 的无边框风格补了一个模拟菜单栏。窗口关闭后不会退出，而是隐藏到系统托盘。</p>
        <div class="feature-list">
          <div class="feature-item"><span class="feature-check">✓</span> 无边框窗口 + 自定义标题栏</div>
          <div class="feature-item"><span class="feature-check">✓</span> 模拟菜单栏 UI（文件 / 编辑 / 视图 / 窗口 / 帮助）</div>
          <div class="feature-item"><span class="feature-check">✓</span> 标题栏拖拽与窗口控制按钮</div>
          <div class="feature-item"><span class="feature-check">✓</span> 托盘左键恢复、右键菜单与退出</div>
        </div>
      </div>
    </div>
    <div class="footer">MBink Rust Borderless Tray Example</div>
  </div>
</body>
</html>
"#;

fn main() -> mbink::Result<()> {
    let mut app = App::builder()
        .title("MBink Rust Tray Demo")
        .size(720, 480)
        .borderless(true)
        .build()?;
    let handle = app.handle();
    let quitting = Arc::new(AtomicBool::new(false));
    let topmost = Arc::new(AtomicBool::new(false));

    app.load_html(HTML)?;
    app.create_tray("MBink Rust Tray Demo")?;
    app.set_tray_menu(&json!([
        { "id": "show", "label": "显示窗口" },
        {
            "type": "submenu",
            "label": "更多",
            "children": [
                { "id": "toggle_top", "label": "置顶窗口", "checked": false },
                { "type": "separator" },
                { "id": "about", "label": "关于" }
            ]
        },
        { "type": "separator" },
        { "id": "quit", "label": "退出" }
    ]))?;

    {
        let quitting = Arc::clone(&quitting);
        let handle = handle;
        app.on_close_request(move || {
            if quitting.load(Ordering::SeqCst) {
                true
            } else {
                println!("close requested -> hide to tray");
                let _ = handle.hide_to_tray();
                false
            }
        })?;
    }

    app.on_close(|| {
        println!("application closed");
    })?;

    app.on_tray_click(move || {
        println!("tray clicked -> restore window");
        let _ = handle.show_main_window();
    })?;

    {
        let quitting = Arc::clone(&quitting);
        let topmost = Arc::clone(&topmost);
        let handle = handle;
        app.on_tray_menu(move |payload| {
            let item_id = payload.get("id").and_then(|v| v.as_str()).unwrap_or("");
            println!("tray menu: {item_id}");

            match item_id {
                "show" => {
                    handle.show_main_window()?;
                    handle.set_title("MBink Rust Tray Demo")?;
                }
                "toggle_top" => {
                    let next = !topmost.load(Ordering::SeqCst);
                    handle.set_always_on_top(next)?;
                    topmost.store(next, Ordering::SeqCst);
                    println!("always_on_top = {next}");
                }
                "about" => {
                    handle.show_main_window()?;
                    handle.set_title("MBink Rust Tray Demo - About")?;
                    println!("MBink Rust tray example");
                }
                "quit" => {
                    quitting.store(true, Ordering::SeqCst);
                    handle.stop();
                }
                _ => {}
            }

            Ok(json!({ "ok": true, "id": item_id }))
        })?;
    }

    app.run();
    Ok(())
}
