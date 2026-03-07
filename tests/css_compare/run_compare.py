#!/usr/bin/env python3
"""CSS布局引擎对比测试 - 对比MBink vs Chrome 布局差异

用法:
  cd d:\\code\\C\\MBink
  python tests/css_compare/run_compare.py
  python tests/css_compare/run_compare.py --tolerance 2 --timeout 8
  python tests/css_compare/run_compare.py --no-chrome
"""
import subprocess, sys, json, argparse, datetime
from pathlib import Path

ROOT_DIR   = Path(__file__).resolve().parent.parent.parent
HTML_FILE  = Path(__file__).resolve().parent / "css_test_cases.html"
ESM_EXE    = ROOT_DIR / "build" / "bin" / "Release" / "esm_loader.exe"
REPORT_DIR = Path(__file__).resolve().parent / "reports"
VW, VH = 1200, 800

R="\033[0m"; RED="\033[91m"; YEL="\033[93m"; GRN="\033[92m"
CYN="\033[96m"; BLD="\033[1m"; GRY="\033[90m"
def cp(c, *a): print(c + " ".join(str(x) for x in a) + R)

# ─── 1. MBink 采集 ───────────────────────────────────────────────────────────
def collect_mbink(timeout: int) -> dict:
    if not ESM_EXE.exists():
        cp(RED, f"[错误] esm_loader 不存在: {ESM_EXE}"); sys.exit(1)
    # --borderless 去掉标题栏，使窗口尺寸 == 内容区尺寸，与 Chrome viewport 一致
    # --no-gpu 使用软件渲染，避免 GPU 缩放影响像素精度
    cmd = [str(ESM_EXE), str(HTML_FILE),
           "--width", str(VW), "--height", str(VH),
           "--borderless", "--no-gpu",
           "-q", str(timeout)]
    cp(CYN, f"\n[MBink] 运行: {' '.join(cmd)}")
    try:
        r = subprocess.run(cmd, capture_output=True, text=True,
                           timeout=timeout + 5, cwd=str(ROOT_DIR))
    except subprocess.TimeoutExpired:
        cp(RED, "[MBink] 进程超时！"); return {}, {}
    return _parse(r.stdout + r.stderr, "MBink")

def _parse(text: str, src: str) -> tuple:
    for line in text.splitlines():
        if "LAYOUT_DATA:" in line:
            raw = line[line.index("LAYOUT_DATA:") + 12:]
            try:
                data = json.loads(raw)
                items = {e["testid"]: e for e in data.get("items", []) if "testid" in e}
                vp = data.get("viewport", {})
                cp(GRN, f"[{src}] 采集到 {len(items)} 个元素  "
                        f"viewport={vp.get('width',0)}x{vp.get('height',0)}")
                return items, vp
            except Exception as e:
                cp(RED, f"[{src}] JSON 解析失败: {e}")
        if "LAYOUT_ERROR:" in line:
            cp(YEL, f"[{src}] JS错误: {line}")
    cp(YEL, f"[{src}] 未找到 LAYOUT_DATA，末尾输出:")
    for ln in text.splitlines()[-15:]:
        print(GRY + "  " + ln + R)
    return {}, {}

# ─── 2. Chrome 采集 ──────────────────────────────────────────────────────────
_COLLECT_JS = """() => {
  const rectMap = {};
  function firstPass(el) {
    const tid = el.getAttribute && el.getAttribute('data-testid');
    if (tid) {
      const r = el.getBoundingClientRect();
      rectMap[tid] = { el, left: r.left, top: r.top, w: r.width, h: r.height };
    }
    for (const ch of (el.children || [])) firstPass(ch);
  }
  function nearestParentTid(el) {
    let p = el.parentElement;
    while (p) {
      const t = p.getAttribute && p.getAttribute('data-testid');
      if (t) return t;
      p = p.parentElement;
    }
    return null;
  }
  const items = [];
  function secondPass(el) {
    const tid = el.getAttribute && el.getAttribute('data-testid');
    if (tid) {
      const r    = el.getBoundingClientRect();
      const ptid = nearestParentTid(el);
      const pr   = ptid ? rectMap[ptid] : null;
      const rnd  = v => Math.round(v * 100) / 100;
      const relX = pr ? rnd(r.left - pr.left) : null;
      const relY = pr ? rnd(r.top  - pr.top)  : null;
      const selfCX = r.left + r.width  / 2;
      const selfCY = r.top  + r.height / 2;
      const centerOffX = pr ? rnd(selfCX - (pr.left + pr.w / 2)) : null;
      const centerOffY = pr ? rnd(selfCY - (pr.top  + pr.h / 2)) : null;
      items.push({
        testid: tid, parentTestid: ptid,
        x: rnd(r.x), y: rnd(r.y),
        width: rnd(r.width), height: rnd(r.height),
        top: rnd(r.top), left: rnd(r.left), right: rnd(r.right), bottom: rnd(r.bottom),
        relX, relY, centerOffX, centerOffY,
        overflowX:    el.scrollWidth  > el.clientWidth  + 1,
        overflowY:    el.scrollHeight > el.clientHeight + 1,
        scrollWidth:  el.scrollWidth,  scrollHeight: el.scrollHeight,
        clientWidth:  el.clientWidth,  clientHeight: el.clientHeight,
      });
    }
    for (const ch of (el.children || [])) secondPass(ch);
  }
  const root = document.getElementById('test-root') || document.body;
  firstPass(root);
  secondPass(root);
  return {
    viewport: { width: window.innerWidth, height: window.innerHeight },
    count: items.length, items
  };
}"""

# 本机 Chrome 候选路径（优先使用，无需 playwright install）
_CHROME_CANDIDATES = [
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    r"C:\Users\{}\AppData\Local\Google\Chrome\Application\chrome.exe".format(
        __import__("os").environ.get("USERNAME", "User")),
]

def _find_chrome() -> str | None:
    for p in _CHROME_CANDIDATES:
        if Path(p).exists():
            return p
    return None

def collect_chrome() -> tuple:
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        cp(RED, "[Chrome] 未安装 playwright，运行: pip install playwright")
        return {}, {}

    chrome_path = _find_chrome()
    url = HTML_FILE.as_uri()
    cp(CYN, f"\n[Chrome] 打开: {url}")
    if chrome_path:
        cp(GRY, f"   使用本机 Chrome: {chrome_path}")
    else:
        cp(YEL, "   未找到本机 Chrome，尝试 Playwright 内置浏览器")

    with sync_playwright() as pw:
        launch_args = dict(headless=True)
        if chrome_path:
            launch_args["executable_path"] = chrome_path
        try:
            br = pw.chromium.launch(**launch_args)
        except Exception as e:
            cp(RED, f"[Chrome] 启动失败: {e}")
            return {}, {}
        page = br.new_page(viewport={"width": VW, "height": VH})
        page.goto(url, wait_until="networkidle")
        page.wait_for_timeout(950)
        data = page.evaluate(_COLLECT_JS)
        br.close()

    items = {e["testid"]: e for e in data.get("items", []) if "testid" in e}
    vp = data.get("viewport", {"width": VW, "height": VH})
    cp(GRN, f"[Chrome] 采集到 {len(items)} 个元素  viewport={vp.get('width',0)}x{vp.get('height',0)}")
    return items, vp


# ─── 3. 差异对比（纯相对坐标策略）──────────────────────────────────────────────
# 策略说明：
#  ① 不对比 width/height 绝对值（字体/平台差异影响内容尺寸）
#  ② 核心：对比 relX/relY（元素相对最近父 testid 容器的偏移）
#  ③ 对齐检测：对比 centerOffX/Y（元素中心相对父容器中心的偏移差）
#  ④ 溢出检测：对比 overflowX/Y 布尔值是否一致
#  ⑤ 兄弟间距：同父容器下相邻子项之间的相对距离差

SCOLOR = {"PASS": GRN, "WARN": YEL, "FAIL": RED,
          "MISSING_MBINK": RED, "MISSING_CHROME": YEL, "MBINK_ERROR": RED}
SICON  = {"PASS": "✅", "WARN": "⚠️ ", "FAIL": "❌",
          "MISSING_MBINK": "❓", "MISSING_CHROME": "❓", "MBINK_ERROR": "💥"}

def _chk(checks, field, mv, cv, tol, desc):
    """像素差异检查（跳过 None）"""
    if mv is None or cv is None:
        return
    d = abs(mv - cv)
    st = "PASS" if d <= 1.0 else ("WARN" if d <= tol else "FAIL")
    checks.append({"field": field, "desc": desc,
                   "mbink": round(mv, 2), "chrome": round(cv, 2),
                   "diff": round(d, 2), "status": st})

def _chk_bool(checks, field, mv, cv, desc):
    """布尔一致性检查"""
    if mv is None or cv is None:
        return
    st = "PASS" if bool(mv) == bool(cv) else "FAIL"
    checks.append({"field": field, "desc": desc,
                   "mbink": mv, "chrome": cv, "diff": 0, "status": st})

def compare(mb_data: dict, ch_data: dict, tol: float) -> list:
    results = []
    for tid in sorted(set(mb_data) | set(ch_data)):
        mb, ch = mb_data.get(tid), ch_data.get(tid)
        if mb is None:
            results.append({"testid": tid, "status": "MISSING_MBINK",  "detail": "仅Chrome有"}); continue
        if ch is None:
            results.append({"testid": tid, "status": "MISSING_CHROME", "detail": "仅MBink有"}); continue
        if "error" in mb:
            results.append({"testid": tid, "status": "MBINK_ERROR",    "detail": mb["error"]}); continue

        checks = []

        # ① 相对父容器偏移（核心，消除字体/平台累积误差）
        has_parent = mb.get("parentTestid") and ch.get("parentTestid")
        if has_parent:
            _chk(checks, "relX", mb.get("relX"), ch.get("relX"), tol, "相对父容器X偏移")
            _chk(checks, "relY", mb.get("relY"), ch.get("relY"), tol, "相对父容器Y偏移")
        else:
            # 无父testid的根级元素：用绝对x/y（viewport已对齐）
            _chk(checks, "x", mb.get("x"), ch.get("x"), tol, "绝对X位置")
            _chk(checks, "y", mb.get("y"), ch.get("y"), tol, "绝对Y位置")

        # ② 对齐检测：元素中心相对父容器中心的偏移差
        #    用于检测 align-items:center / justify-content:center 是否生效
        if has_parent:
            _chk(checks, "centerOffX", mb.get("centerOffX"), ch.get("centerOffX"),
                 tol, "水平居中偏差(相对父中心)")
            _chk(checks, "centerOffY", mb.get("centerOffY"), ch.get("centerOffY"),
                 tol, "垂直居中偏差(相对父中心)")

        # ③ 内容溢出检测（overflow 状态必须一致）
        _chk_bool(checks, "overflowX", mb.get("overflowX"), ch.get("overflowX"), "横向内容溢出")
        _chk_bool(checks, "overflowY", mb.get("overflowY"), ch.get("overflowY"), "纵向内容溢出")

        worst = max((c["diff"] for c in checks if isinstance(c["diff"], (int, float))), default=0)
        ov = "PASS"
        for c in checks:
            if c["status"] == "FAIL": ov = "FAIL"; break
            if c["status"] == "WARN": ov = "WARN"

        results.append({"testid": tid, "status": ov, "worst_diff": round(worst, 2),
                         "checks": checks, "mb": mb, "ch": ch})
    return results

def compare_sibling_gaps(mb_data: dict, ch_data: dict, tol: float) -> list:
    """
    兄弟元素间距对比：同一父testid容器下，相邻子项的相对间距差异。

    同时检测两个方向：
      行方向 (水平) gap = next.relX - (cur.relX + cur.width)
      列方向 (垂直) gap = next.relY - (cur.relY + cur.height)

    通过判断相邻子项的主轴方向自动选择：
      若 next.relX > cur.relX（X 有明显增量） → 水平排列 → 检测水平间距
      若 next.relY > cur.relY（Y 有明显增量） → 垂直排列 → 检测垂直间距
    不依赖绝对尺寸，仅比较两端的 gap 差值。
    """
    from collections import defaultdict
    results = []

    def group(data):
        g = defaultdict(list)
        for el in data.values():
            ptid = el.get("parentTestid")
            if ptid and el.get("relX") is not None:
                g[ptid].append(el)
        for lst in g.values():
            lst.sort(key=lambda e: (round(e.get("relY", 0)), round(e.get("relX", 0))))
        return g

    mb_g, ch_g = group(mb_data), group(ch_data)

    for ptid in sorted(set(mb_g) | set(ch_g)):
        ml, cl = mb_g.get(ptid, []), ch_g.get(ptid, [])
        n = min(len(ml), len(cl))
        if n < 2:
            continue
        for i in range(n - 1):
            mA, mB = ml[i], ml[i+1]
            cA, cB = cl[i], cl[i+1]

            # 判断排列方向（以 Chrome 的数据为准）
            dx = abs(cB.get("relX", 0) - cA.get("relX", 0))
            dy = abs(cB.get("relY", 0) - cA.get("relY", 0))

            checks = []

            if dx >= dy:
                # 水平排列（row 方向） → 检测水平 gap
                mb_gap = round(mB.get("relX", 0) - mA.get("relX", 0) - mA.get("width", 0), 2)
                ch_gap = round(cB.get("relX", 0) - cA.get("relX", 0) - cA.get("width", 0), 2)
                d = abs(mb_gap - ch_gap)
                st = "PASS" if d <= 1.0 else ("WARN" if d <= tol else "FAIL")
                checks.append({"field": "hgap", "desc": f"水平间距 子{i}↔子{i+1}",
                                "mbink": mb_gap, "chrome": ch_gap,
                                "diff": round(d, 2), "status": st})
            else:
                # 垂直排列（col 方向） → 检测垂直 gap
                mb_gap = round(mB.get("relY", 0) - mA.get("relY", 0) - mA.get("height", 0), 2)
                ch_gap = round(cB.get("relY", 0) - cA.get("relY", 0) - cA.get("height", 0), 2)
                d = abs(mb_gap - ch_gap)
                st = "PASS" if d <= 1.0 else ("WARN" if d <= tol else "FAIL")
                checks.append({"field": "vgap", "desc": f"垂直间距 子{i}↔子{i+1}",
                                "mbink": mb_gap, "chrome": ch_gap,
                                "diff": round(d, 2), "status": st})

            # 只报告非 PASS 的
            worst = max(c["diff"] for c in checks)
            ov = "FAIL" if any(c["status"] == "FAIL" for c in checks) else (
                 "WARN" if any(c["status"] == "WARN" for c in checks) else "PASS")
            if ov != "PASS":
                results.append({
                    "testid": f"[gap] {ptid} → 子{i}↔子{i+1}",
                    "status": ov, "worst_diff": round(worst, 2),
                    "checks": checks, "mb": {}, "ch": {}
                })
    return results



# ─── 4. 控制台报告 + 文本报告 ────────────────────────────────────────────────
def print_report(results: list, tol: float):
    passes = sum(1 for r in results if r["status"] == "PASS")
    warns  = sum(1 for r in results if r["status"] == "WARN")
    fails  = sum(1 for r in results if r["status"] in ("FAIL","MISSING_MBINK","MBINK_ERROR"))

    # 生成纯文本内容（无 ANSI 颜色）
    ts  = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    txt_path = REPORT_DIR / f"report_{ts}.txt"
    REPORT_DIR.mkdir(parents=True, exist_ok=True)

    lines = []  # 纯文本行缓冲

    def emit(text=""):
        """同时输出到控制台和文本缓冲"""
        print(text)
        # 去掉 ANSI 转义序列后写入文本
        import re
        clean = re.sub(r'\033\[[0-9;]*m', '', text)
        lines.append(clean)

    SEP = "─" * 72
    emit(f"\n{SEP}")
    emit(f"  CSS 布局对比结果  (容差: ±{tol}px，相对坐标策略)")
    emit(f"  生成时间: {ts}")
    emit(SEP)

    for r in results:
        st   = r["status"]
        col  = SCOLOR.get(st, R)
        icon = SICON.get(st, "?")
        label = f"{icon} [{st:<14}]  {r['testid']}"

        if st in ("MISSING_MBINK", "MISSING_CHROME", "MBINK_ERROR"):
            emit(col + label + f"  ({r.get('detail','')})" + R)
            continue

        emit(col + label + f"  最大差异={r.get('worst_diff',0)}px" + R)

        if st != "PASS":
            for ck in r.get("checks", r.get("fields", [])):
                if ck["status"] == "PASS": continue
                fc  = SCOLOR.get(ck["status"], R)
                mv  = ck["mbink"]; cv = ck["chrome"]
                mvs = f"{mv:.1f}" if isinstance(mv, float) else str(mv)
                cvs = f"{cv:.1f}" if isinstance(cv, float) else str(cv)
                emit(fc + f"    {ck.get('desc', ck['field']):26s}"
                     f"  MBink={mvs:>8}  Chrome={cvs:>8}"
                     f"  Δ={ck['diff']}" + R)

    emit(SEP)
    emit(f"  ✅ PASS : {passes}")
    emit(f"  ⚠️  WARN : {warns}")
    emit(f"  ❌ FAIL : {fails}")
    emit(f"  共计   : {len(results)}")
    emit(SEP + "\n")

    # 写入文本文件
    txt_path.write_text("\n".join(lines), encoding="utf-8")
    cp(CYN, f"📝 文本报告: {txt_path}")

    return passes, warns, fails

# ─── 5. HTML报告 ──────────────────────────────────────────────────────────────
def save_html_report(results: list, tol: float) -> Path:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    ts  = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    out = REPORT_DIR / f"report_{ts}.html"

    _CL = {"FAIL": "red", "WARN": "orange", "PASS": "green"}

    rows = []
    for r in results:
        st   = r["status"]
        bg   = {"PASS": "#c8e6c9", "WARN": "#fff9c4", "FAIL": "#ffcdd2"}.get(st, "#ffe0b2")
        icon = SICON.get(st, "?")
        tid  = r["testid"]

        if st in ("MISSING_MBINK", "MISSING_CHROME", "MBINK_ERROR"):
            rows.append(f"<tr style='background:{bg}'>"
                        f"<td>{icon} {st}</td><td>{tid}</td>"
                        f"<td colspan='4'>{r.get('detail','')}</td></tr>")
            continue

        mb, ch = r.get("mb", {}), r.get("ch", {})

        # 使用新的 checks 字段（兼容旧 fields）
        ck_list  = r.get("checks", r.get("fields", []))
        ck_parts = []
        for ck in ck_list:
            color = _CL.get(ck["status"], "black")
            mv = ck["mbink"]; cv = ck["chrome"]
            mvs = f"{mv:.1f}" if isinstance(mv, float) else str(mv)
            cvs = f"{cv:.1f}" if isinstance(cv, float) else str(cv)
            ck_parts.append(
                f"<b style='color:{color}'>{ck.get('desc', ck['field'])}"
                f" [MB:{mvs} CH:{cvs} Δ{ck['diff']}]</b>"
            )
        ck_html = "<br>".join(ck_parts) if ck_parts else "—"

        # 参考坐标：优先 relX/relY，否则 x/y
        mb_pos = (f"relX={mb.get('relX','—')} relY={mb.get('relY','—')}"
                  if mb.get("relX") is not None
                  else f"x={mb.get('x',0):.1f} y={mb.get('y',0):.1f}")
        ch_pos = (f"relX={ch.get('relX','—')} relY={ch.get('relY','—')}"
                  if ch.get("relX") is not None
                  else f"x={ch.get('x',0):.1f} y={ch.get('y',0):.1f}")

        rows.append(
            f"<tr style='background:{bg}'>"
            f"<td>{icon} {st}</td>"
            f"<td><b>{tid}</b>"
            + (f"<br><small>父:{mb.get('parentTestid','—')}</small>" if mb.get("parentTestid") else "")
            + f"</td>"
            f"<td style='font-size:11px'>{mb_pos}</td>"
            f"<td style='font-size:11px'>{ch_pos}</td>"
            f"<td style='font-size:11px'>{ck_html}</td></tr>"
        )

    passes = sum(1 for r in results if r["status"] == "PASS")
    warns  = sum(1 for r in results if r["status"] == "WARN")
    fails  = sum(1 for r in results if r["status"] in ("FAIL", "MISSING_MBINK", "MBINK_ERROR"))

    html = (
        f"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        f"<title>CSS布局报告 {ts}</title>"
        f"<style>"
        f"body{{font-family:monospace;padding:20px;background:#fafafa;color:#222}}"
        f"h1{{color:#333}}"
        f".summary{{margin:10px 0;padding:10px;background:#fff;border-radius:6px;border:1px solid #ddd}}"
        f"table{{border-collapse:collapse;width:100%}}"
        f"th{{background:#37474f;color:white;padding:8px 12px;text-align:left;font-size:12px}}"
        f"td{{padding:6px 10px;border-bottom:1px solid #ddd;font-size:12px;vertical-align:top}}"
        f"tr:hover{{filter:brightness(0.96)}}"
        f"</style></head><body>"
        f"<h1>🔬 CSS 布局对比报告</h1>"
        f"<div class='summary'>"
        f"⏱ {ts} &nbsp;|&nbsp; 容差 ±{tol}px &nbsp;|&nbsp; 共 {len(results)} 项 &nbsp;|&nbsp; "
        f"<span style='color:green'>✅ {passes}</span> &nbsp;"
        f"<span style='color:orange'>⚠️ {warns}</span> &nbsp;"
        f"<span style='color:red'>❌ {fails}</span>"
        f"</div>"
        f"<p style='font-size:11px;color:#666'>策略：相对坐标对比（relX/relY）+ 对齐偏差 + 溢出检测，不对比绝对 width/height</p>"
        f"<table>"
        f"<tr><th>状态</th><th>TestID</th><th>MBink 坐标</th><th>Chrome 坐标</th><th>检查项详情</th></tr>"
        + "".join(rows)
        + "</table></body></html>"
    )
    out.write_text(html, encoding="utf-8")
    return out

# ─── 6. 主入口 ────────────────────────────────────────────────────────────────
def main():
    pa = argparse.ArgumentParser(description="CSS布局引擎对比测试")
    pa.add_argument("--tolerance", type=float, default=2.0, help="像素容差，默认2px")
    pa.add_argument("--timeout",   type=int,   default=8,   help="esm_loader等待秒数，默认8s")
    pa.add_argument("--no-chrome", action="store_true",     help="跳过Chrome，仅展示MBink数据")
    args = pa.parse_args()

    cp(BLD+CYN, "\n🔬 CSS 布局引擎对比测试")
    cp(GRY, f"   HTML : {HTML_FILE}")
    cp(GRY, f"   EXE  : {ESM_EXE}")
    cp(GRY, f"   容差 : ±{args.tolerance}px")

    mb_data, mb_vp = collect_mbink(args.timeout)
    if args.no_chrome:
        ch_data, ch_vp = {}, {}
    else:
        ch_data, ch_vp = collect_chrome()

    if not mb_data and not ch_data:
        cp(RED, "\n❌ 两端均无数据，请检查配置。"); sys.exit(1)

    # viewport 一致性检查
    if mb_vp and ch_vp:
        mw, mh = mb_vp.get("width", 0), mb_vp.get("height", 0)
        cw, ch_ = ch_vp.get("width", 0), ch_vp.get("height", 0)
        if abs(mw - cw) > 2 or abs(mh - ch_) > 2:
            cp(YEL, f"\n⚠️  viewport 不一致！MBink={mw}x{mh}  Chrome={cw}x{ch_}")
            cp(YEL,  "   位置坐标差异可能因此偏大，建议使用 --borderless 模式")
        else:
            cp(GRN, f"   ✅ viewport 一致: {mw}x{mh}")

    if not ch_data:
        cp(YEL, "\n⚠️  Chrome数据为空，仅展示MBink采集结果：")
        for tid, el in sorted(mb_data.items()):
            print(f"  {tid:48s}  x={el.get('x',0):7.1f}  y={el.get('y',0):7.1f}"
                  f"  w={el.get('width',0):7.1f}  h={el.get('height',0):7.1f}")
        sys.exit(0)

    results     = compare(mb_data, ch_data, args.tolerance)
    gap_results = compare_sibling_gaps(mb_data, ch_data, args.tolerance)
    all_results = results + gap_results

    _, _, fails = print_report(all_results, args.tolerance)
    report = save_html_report(all_results, args.tolerance)
    cp(CYN, f"📄 HTML报告: {report}")
    sys.exit(0 if fails == 0 else 1)

if __name__ == "__main__":
    main()

