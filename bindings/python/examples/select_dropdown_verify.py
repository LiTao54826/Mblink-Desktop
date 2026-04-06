import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from mbink import App

HTML = r"""
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <title>MBink Select Dropdown Verify</title>
  <style>
    * { box-sizing: border-box; }
    html, body { margin: 0; width: 100%; height: 100%; background: #0f172a; color: #e2e8f0; font: 14px/1.45 "Segoe UI", Arial, sans-serif; }
    body { overflow: auto; }
    .wrap { min-height: 180vh; padding: 20px; }
    .card { max-width: 980px; margin: 0 auto; background: #111827; border: 1px solid #334155; border-radius: 14px; padding: 18px; box-shadow: 0 18px 40px rgba(0,0,0,.25); }
    h1 { margin: 0 0 10px; font-size: 24px; }
    .muted { color: #94a3b8; }
    .toolbar { display: flex; gap: 10px; flex-wrap: wrap; margin: 14px 0 18px; }
    button { border: 0; border-radius: 8px; padding: 10px 14px; background: #2563eb; color: #fff; cursor: pointer; }
    .grid { display: grid; grid-template-columns: repeat(2, minmax(280px, 1fr)); gap: 16px; }
    .panel { background: #0b1220; border: 1px solid #223047; border-radius: 12px; padding: 14px; }
    .panel h2 { margin: 0 0 8px; font-size: 16px; }
    label { display: block; margin: 10px 0 6px; color: #cbd5e1; }
    select { width: 100%; min-height: 34px; padding: 6px 8px; border-radius: 8px; border: 1px solid #475569; background: #fff; color: #111827; }
    .note { margin-top: 8px; font-size: 12px; color: #93c5fd; }
    .spacer { height: 72vh; display: flex; align-items: center; justify-content: center; color: #64748b; }
    pre { margin: 16px 0 0; min-height: 120px; max-height: 220px; overflow: auto; background: #020617; border: 1px solid #223047; border-radius: 10px; padding: 12px; color: #cbd5e1; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>MBink Select Dropdown 验证页</h1>
      <div class="muted">检查点：位置贴合触发框、宽度接近 Chrome、长列表可滚动、底部控件可向上展开、DPI 125%/150% 下仍与控件对齐。</div>
      <div class="toolbar">
        <button onclick="scrollTo(0, 0)">滚动到顶部</button>
        <button onclick="document.getElementById('bottom-zone').scrollIntoView({behavior:'smooth', block:'center'})">滚动到底部测试区</button>
      </div>

      <div class="grid">
        <div class="panel">
          <h2>基础与宽度</h2>
          <label for="basic">基础下拉</label>
          <select id="basic">
            <option>Apple</option>
            <option selected>Banana</option>
            <option>Cherry</option>
            <option>Dragon Fruit</option>
          </select>

          <label for="wide">超长文本宽度</label>
          <select id="wide">
            <option>Short option</option>
            <option selected>This is a very long option used to verify content based dropdown width like Chrome</option>
            <option>Another option</option>
          </select>
          <div class="note">打开 wide，观察卡片宽度是否能覆盖长文本且不离触发框过远。</div>
        </div>

        <div class="panel">
          <h2>滚动条与分组</h2>
          <label for="long">长列表滚动</label>
          <select id="long"></select>

          <label for="grouped">optgroup / disabled</label>
          <select id="grouped">
            <optgroup label="Frontend">
              <option>React</option>
              <option selected>Preact</option>
              <option disabled>Vue (disabled)</option>
            </optgroup>
            <optgroup label="Backend">
              <option>Python</option>
              <option>Go</option>
              <option>Rust</option>
            </optgroup>
          </select>
          <div class="note">打开 long，观察滚轮与细滚动条；打开 grouped，观察 label/disabled 样式与命中。</div>
        </div>
      </div>

      <div class="spacer">向下滚动后，在窗口底部附近打开 select，验证是否会优先向上展开。</div>

      <div class="panel" id="bottom-zone">
        <h2>底部区域：向上展开</h2>
        <label for="bottom">底部 select</label>
        <select id="bottom"></select>
        <div class="note">将此控件放到视口下缘附近后打开，观察 dropdown 是否自动上翻，并仍然被 viewport clamp。</div>
      </div>

      <pre id="log">ready\n</pre>
    </div>
  </div>

  <script>
    const log = (msg) => {
      const box = document.getElementById('log');
      box.textContent += msg + "\n";
      box.scrollTop = box.scrollHeight;
    };

    const fill = (id, count, prefix) => {
      const sel = document.getElementById(id);
      for (let i = 1; i <= count; i++) {
        const opt = document.createElement('option');
        opt.textContent = `${prefix} ${i} — Lorem ipsum dolor sit amet`;
        opt.value = `${prefix}-${i}`;
        if (i === Math.ceil(count / 2)) opt.selected = true;
        sel.appendChild(opt);
      }
    };

    fill('long', 36, 'Long option');
    fill('bottom', 30, 'Bottom option');

    document.querySelectorAll('select').forEach((sel) => {
      sel.addEventListener('change', () => log(`[change] ${sel.id} => ${sel.value || sel.options[sel.selectedIndex].text}`));
      sel.addEventListener('click', () => log(`[open?] ${sel.id}`));
    });

    log('tips: try system DPI 125% / 150%, open wide / long / bottom selects');
  </script>
</body>
</html>
"""


def main():
    app = App("MBink Select Dropdown Verify", 980, 820, gpu=False)
    app.load_html(HTML)
    app.run()


if __name__ == "__main__":
    main()
