/**
 * SysMon UI — Preact 系统监视器界面
 *
 * 运行时变量：
 *   通过 ESM import 使用官方 preact
 *   sys                   — Python 端 app.shared("sys") 创建的共享对象
 *     .cpu_total          float   总 CPU 使用率 %
 *     .cpu_cores          float[] 每核心使用率 %
 *     .mem_used           int     已用内存 MB
 *     .mem_total          int     总内存 MB
 *     .mem_percent        float   内存使用率 %
 *     .disk_read          float   磁盘读速度 KB/s
 *     .disk_write         float   磁盘写速度 KB/s
 *     .net_up             float   网络上传 KB/s
 *     .net_down           float   网络下载 KB/s
 *     .cpu_hist           float[] CPU 历史数据（60 点）
 *     .mem_hist           float[] 内存历史数据（60 点）
 *     .timestamp          string  当前时间 HH:MM:SS
 */

import { h, render } from 'preact';

// ── 主题色 ──────────────────────────────────────────────────────────────────
const C = {
  bg:       '#0f1117',
  surface:  '#1a1d27',
  border:   '#2a2d3d',
  titlebar: '#12141e',
  cpu:      '#4f8ef7',
  mem:      '#a78bfa',
  disk:     '#34d399',
  net:      '#fb923c',
  text:     '#e2e8f0',
  muted:    '#64748b',
  accent:   '#4f8ef7',
};

// ── 工具：格式化速率 ──────────────────────────────────────────────────────────
function fmtRate(kb) {
  if (kb >= 1024) return (kb / 1024).toFixed(1) + ' MB/s';
  return kb.toFixed(1) + ' KB/s';
}

function fmtMem(mb) {
  if (mb >= 1024) return (mb / 1024).toFixed(1) + ' GB';
  return mb + ' MB';
}

// ── SVG 折线图（viewBox 自适应宽度）──────────────────────────────────────────
var SPARK_VW = 300;  // 内部坐标系宽度（固定），外部由 width='100%' 缩放
function Sparkline({ data, color, h: height = 44, max = 100 }) {
  if (!data || data.length < 2) return null;
  const len   = data.length;
  const xStep = SPARK_VW / (len - 1);
  const pts = data.map(function(v, i) {
    var x = i * xStep;
    var y = height - (v / max) * (height - 4) - 2;
    return x + ',' + y;
  }).join(' ');

  var fill = '0,' + height + ' ' + pts + ' ' + ((len - 1) * xStep) + ',' + height;
  var gid  = 'g_' + color.replace('#', '');

  return h('svg', {
    viewBox: '0 0 ' + SPARK_VW + ' ' + height,
    width: '100%', height: height,
    preserveAspectRatio: 'none',
    style: { display: 'block' },
  },
    h('defs', null,
      h('linearGradient', { id: gid, x1: '0', y1: '0', x2: '0', y2: '1' },
        h('stop', { offset: '0%',   stopColor: color, stopOpacity: '0.35' }),
        h('stop', { offset: '100%', stopColor: color, stopOpacity: '0.02' }),
      )
    ),
    h('polygon', { points: fill, fill: 'url(#' + gid + ')' }),
    h('polyline', {
      points: pts,
      fill: 'none',
      stroke: color,
      strokeWidth: '1.5',
      strokeLinejoin: 'round',
      strokeLinecap: 'round',
    })
  );
}

// ── 仪表卡 ───────────────────────────────────────────────────────────────────
function MetricCard({ title, icon, value, unit, sub, color, chart, max }) {
  return h('div', {
    style: {
      background: C.surface, borderRadius: '10px', padding: '10px 12px',
      border: '1px solid ' + C.border, display: 'flex', flexDirection: 'column', gap: '6px',
      minWidth: 0, overflow: 'hidden',
    }
  },
    h('div', { style: { display: 'flex', alignItems: 'center', gap: '6px' } },
      h('span', { style: { fontSize: '14px' } }, icon),
      h('span', { style: { fontSize: '11px', color: C.muted, fontWeight: 600, letterSpacing: '0.05em', textTransform: 'uppercase' } }, title),
    ),
    h('div', { style: { display: 'flex', alignItems: 'baseline', gap: '3px' } },
      h('span', { style: { fontSize: '26px', fontWeight: 700, color: C.text, lineHeight: 1 } }, value),
      h('span', { style: { fontSize: '13px', color: C.muted } }, unit),
    ),
    chart ? h(Sparkline, { data: chart, color: color, max: max }) : null,
    sub ? h('div', { style: { fontSize: '10px', color: C.muted } }, sub) : null,
  );
}

// ── CPU 核心条 ────────────────────────────────────────────────────────────────
function CoreBars({ cores }) {
  if (!cores || !cores.length) return null;
  return h('div', { style: { background: C.surface, borderRadius: '10px', padding: '10px 12px', border: '1px solid ' + C.border, flex: 1, minHeight: 0, overflow: 'hidden' } },
    h('div', { style: { fontSize: '10px', color: C.muted, fontWeight: 600, letterSpacing: '0.05em', marginBottom: '8px' } }, '🧩 CPU CORES'),
    h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(72px, 1fr))', gap: '6px' } },
      ...cores.map((v, i) =>
        h('div', { key: i, style: { display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '4px' } },
          h('div', { style: { width: '100%', height: '4px', background: C.border, borderRadius: '2px', overflow: 'hidden' } },
            h('div', { style: { height: '100%', width: v + '%', background: C.cpu, borderRadius: '2px', transition: 'width 0.5s ease' } })
          ),
          h('span', { style: { fontSize: '10px', color: C.muted } }, `C${i} ${v}%`)
        )
      )
    )
  );
}

// ── 标题栏 ────────────────────────────────────────────────────────────────────
function TitleBar({ timestamp }) {
  return h('div', {
    class: 'drag',
    style: {
      height: '38px', background: C.titlebar,
      display: 'flex', alignItems: 'center', padding: '0 14px',
      borderBottom: '1px solid ' + C.border, flexShrink: 0,
      userSelect: 'none',
    }
  },
    h('span', { style: { fontSize: '15px', marginRight: '8px' } }, '🖥️'),
    h('span', { style: { fontSize: '13px', fontWeight: 600, color: C.text } }, 'System Monitor'),
    h('span', { style: { marginLeft: '10px', fontSize: '11px', color: C.muted } }, timestamp || ''),
    h('div', { class: 'no-drag', style: { marginLeft: 'auto', display: 'flex', gap: '7px', alignItems: 'center' } },
      h('button', { class: 'wc-btn wc-pin',      title: '置顶' }),
      h('button', { class: 'wc-btn wc-minimize', title: '最小化' }),
      h('button', { class: 'wc-btn wc-maximize', title: '最大化' }),
      h('button', { class: 'wc-btn wc-close',    title: '关闭' }),
    )
  );
}

// ── 主应用 ────────────────────────────────────────────────────────────────────
function App() {
  const s = sys;   // globalThis.sys — SharedState

  const cpuColor = s.cpu_total > 80 ? '#f87171' : s.cpu_total > 50 ? '#fbbf24' : C.cpu;

  return h('div', {
    style: {
      width: '100%', height: '100%', display: 'flex', flexDirection: 'column',
      background: C.bg, color: C.text,
      fontFamily: '"Segoe UI", -apple-system, BlinkMacSystemFont, sans-serif',
      overflow: 'hidden',
    }
  },
    h(TitleBar, { timestamp: s.timestamp }),

    // 主内容区
    h('div', { style: { flex: 1, padding: '10px 12px', display: 'flex', flexDirection: 'column', gap: '10px', overflow: 'hidden', minHeight: 0 } },

      // 第一行：4 个主仪表卡（高度固定，不伸缩）
      h('div', { style: { display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '10px', flex: '0 0 auto' } },
        h(MetricCard, {
          title: 'CPU', icon: '⚡', color: cpuColor,
          value: s.cpu_total, unit: '%',
          chart: s.cpu_hist, max: 100,
          sub: `${(s.cpu_cores || []).length} 核心`,
        }),
        h(MetricCard, {
          title: 'Memory', icon: '🧠', color: C.mem,
          value: s.mem_percent, unit: '%',
          chart: s.mem_hist, max: 100,
          sub: `${fmtMem(s.mem_used)} / ${fmtMem(s.mem_total)}`,
        }),
        h(MetricCard, {
          title: 'Disk I/O', icon: '💾', color: C.disk,
          value: fmtRate(s.disk_read), unit: '',
          sub: `↓ ${fmtRate(s.disk_read)}  ↑ ${fmtRate(s.disk_write)}`,
        }),
        h(MetricCard, {
          title: 'Network', icon: '🌐', color: C.net,
          value: fmtRate(s.net_down), unit: '',
          sub: `↓ ${fmtRate(s.net_down)}  ↑ ${fmtRate(s.net_up)}`,
        }),
      ),

      // 第二行：核心占用条
      h(CoreBars, { cores: s.cpu_cores }),
    )
  );
}


render(h(App), document.getElementById('root') || document.body);

// console.log("加载完成")