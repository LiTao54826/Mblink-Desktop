// 布局回归测试脚本（基于 esm_loader 输出日志）
// 用法：
// node tests/js/layout_incremental_update_check.js

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

function writeFixture() {
  const fixtureDir = path.resolve('tmp', 'layout_incremental_update_check');
  fs.rmSync(fixtureDir, { recursive: true, force: true });
  fs.mkdirSync(fixtureDir, { recursive: true });
  const entry = path.join(fixtureDir, 'index.html');
  fs.writeFileSync(entry, `<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>Incremental layout guard</title>
  <style>
    * { box-sizing: border-box; }
    body { margin: 0; font-family: Segoe UI, sans-serif; background: #f8fafc; color: #172033; }
    #grid { display: grid; grid-template-columns: repeat(2, 380px); gap: 18px; padding: 24px; align-items: start; }
    .card { width: 380px; padding: 16px; border: 1px solid #cbd5e1; border-radius: 6px; background: #ffffff; }
    .card h2 { margin: 0; font-size: 18px; line-height: 24px; }
    .tag { display: block; margin-top: 8px; width: max-content; padding: 3px 8px; border: 1px solid #94a3b8; border-radius: 4px; }
    .desc { margin: 10px 0 12px; line-height: 20px; }
    .card input { display: block; width: 100%; padding: 8px 10px; border: 1px solid #94a3b8; border-radius: 4px; }
    .btn-row { display: flex; gap: 8px; margin-top: 12px; }
    .btn-row button { padding: 8px 10px; border: 1px solid #64748b; border-radius: 4px; background: #f8fafc; }
  </style>
</head>
<body>
  <main id="grid"></main>
  <script>
    const grid = document.getElementById('grid');
    for (let i = 1; i <= 6; i += 1) {
      const card = document.createElement('section');
      card.className = 'card';
      card.id = 'card' + i;
      card.innerHTML =
        '<h2>Card ' + i + '</h2>' +
        '<span class="tag">stable tag</span>' +
        '<p class="desc">A compact paragraph used to verify inline formatting and block flow.</p>' +
        '<input value="card ' + i + ' value">' +
        '<div class="btn-row"><button>SetTextContent</button><button>Refill</button></div>';
      grid.appendChild(card);
    }

    const rectOf = (element) => {
      const rect = element.getBoundingClientRect();
      return {
        left: rect.left,
        top: rect.top,
        right: rect.right,
        bottom: rect.bottom,
        x: rect.x,
        y: rect.y,
        w: rect.width,
        h: rect.height
      };
    };

    const boxOf = (element) => {
      const style = getComputedStyle(element);
      return {
        width: style.width,
        padding: [
          style.paddingTop,
          style.paddingRight,
          style.paddingBottom,
          style.paddingLeft
        ].join(' ')
      };
    };

    const report = (path, element, depth) => {
      console.log('[layout] ' + JSON.stringify({
        path,
        depth,
        box: boxOf(element),
        rect: rectOf(element)
      }));
    };

    const reportTree = (path, element, depth) => {
      report(path, element, depth);
      [...element.children].forEach((child, childIndex) => {
        reportTree(path + '.' + childIndex, child, depth + 1);
      });
    };

    [...document.querySelectorAll('.card')].forEach((card, cardIndex) => {
      const cardPath = 'card' + (cardIndex + 1);
      reportTree(cardPath, card, 0);
    });
  </script>
</body>
</html>
`);
  return entry;
}

function run() {
  const entry = writeFixture();
  const cmd = `build\\bin\\Release\\esm_loader.exe "${entry}" -q 3`;
  const output = execSync(cmd, { cwd: process.cwd(), encoding: 'utf8', stdio: 'pipe' });

  const lines = output.split(/\r?\n/);
  const layoutLines = lines.filter((l) => l.includes('[layout] {"path":"card'));

  if (layoutLines.length === 0) {
    throw new Error('未捕获到布局JSON输出，请检查 demo 脚本是否注入 dumpCardLayoutReport');
  }

  const cardRoots = [];
  const nodesByPath = new Map();

  for (const line of layoutLines) {
    const idx = line.indexOf('{');
    if (idx < 0) continue;
    const json = JSON.parse(line.slice(idx));
    nodesByPath.set(json.path, json);

    if (json.depth === 0 && /^card[0-9]+$/.test(json.path)) {
      cardRoots.push(json);
    }
  }

  if (cardRoots.length !== 6) {
    throw new Error(`卡片根节点数量异常，期望6，实际${cardRoots.length}`);
  }

  for (const card of cardRoots) {
    if (card.box.width !== '380px') {
      throw new Error(`${card.path} 宽度异常: ${card.box.width}`);
    }
    if (card.box.padding !== '16px 16px 16px 16px') {
      throw new Error(`${card.path} padding异常: ${card.box.padding}`);
    }
  }

  const firstPathUnder = (rootPath, predicate) => {
    const candidates = [...nodesByPath.entries()]
      .filter(([nodePath]) => nodePath.startsWith(`${rootPath}.`))
      .sort((a, b) => a[0].localeCompare(b[0], undefined, { numeric: true }));
    for (const [nodePath, node] of candidates) {
      if (predicate(nodePath, node)) return { path: nodePath, node };
    }
    return null;
  };

  const card1ContentLeft = Number(cardRoots[0].rect.left) + 17;
  const nearContentLeft = (node) => Math.abs(Number(node.rect.left) - card1ContentLeft) <= 0.01;
  const positiveWidth = (node) => Number(node.rect.w) > 0;
  const card1Desc = firstPathUnder('card1', (_path, node) => nearContentLeft(node) && Number(node.rect.w) > 300 && Number(node.rect.h) >= 30);
  const card1Input = firstPathUnder('card1', (_path, node) => nearContentLeft(node) && Number(node.rect.w) > 300 && Number(node.rect.h) > 0 && node.box.width === '100%');
  const card1BtnRow = firstPathUnder('card1', (_path, node) => nearContentLeft(node) && Number(node.rect.w) > 300 && Number(node.rect.h) > 20 && Number(node.rect.top) > Number(card1Input?.node?.rect.bottom || 0));
  const card1Tag = firstPathUnder('card1', (_path, node) => nearContentLeft(node) && !positiveWidth(node) && Number(node.rect.h) >= 20);

  // 核心回归：card1 input (card1.3) 不能横向溢出 card1
  const card1 = nodesByPath.get('card1');
  const input1 = card1Input && card1Input.node;
  if (!card1 || !input1) {
    throw new Error('缺少 card1 或 card1.3(input) 布局节点，无法执行溢出断言');
  }

  const cardRight = Number(card1.rect.right);
  const inputRight = Number(input1.rect.right);
  const inputLeft = Number(input1.rect.left);
  const cardLeft = Number(card1.rect.left);

  if (!(inputLeft >= cardLeft)) {
    throw new Error(`input 左边界越界: inputLeft=${inputLeft}, cardLeft=${cardLeft}`);
  }
  if (!(inputRight <= cardRight)) {
    throw new Error(`input 右边界越界: inputRight=${inputRight}, cardRight=${cardRight}`);
  }

  // 额外 guard：input 实际几何宽不应超过 card content 宽(380 - 2*16 - 2*1 = 346)
  const inputWidth = Number(input1.rect.w);
  const expectedMaxContentWidth = 346;
  if (inputWidth > expectedMaxContentWidth + 0.01) {
    throw new Error(`input 几何宽异常(疑似按 outer 宽解析 100%): inputWidth=${inputWidth}, max=${expectedMaxContentWidth}`);
  }

  // 新增回归：匿名块 IFC 不应出现双重偏移导致的右下错位
  // 1) 同一列的元素 left 应一致（标题/标签/描述/input/按钮行都应贴齐 card content 左边）
  const title = nodesByPath.get('card1.0');
  const tag = card1Tag && card1Tag.node;
  const desc = card1Desc && card1Desc.node;
  const btnRow = card1BtnRow && card1BtnRow.node;
  if (!title || !tag || !desc || !btnRow) {
    throw new Error('缺少 card1.0/1/2/4 布局节点，无法执行位置偏移断言');
  }

  const titleLeft = Number(title.rect.left);
  const tagLeft = Number(tag.rect.left);
  const descLeft = Number(desc.rect.left);
  const btnRowLeft = Number(btnRow.rect.left);
  const epsilon = 0.01;

  if (Math.abs(tagLeft - titleLeft) > epsilon) {
    throw new Error(`tag 左对齐异常(疑似X偏移): tagLeft=${tagLeft}, titleLeft=${titleLeft}`);
  }
  if (Math.abs(inputLeft - titleLeft) > epsilon) {
    throw new Error(`input 左对齐异常(疑似X偏移): inputLeft=${inputLeft}, titleLeft=${titleLeft}`);
  }
  if (Math.abs(descLeft - titleLeft) > epsilon) {
    throw new Error(`desc 左对齐异常(疑似X偏移): descLeft=${descLeft}, titleLeft=${titleLeft}`);
  }
  if (Math.abs(btnRowLeft - titleLeft) > epsilon) {
    throw new Error(`btnRow 左对齐异常(疑似X偏移): btnRowLeft=${btnRowLeft}, titleLeft=${titleLeft}`);
  }

  // 2) input 不应与后续按钮行重叠（防止Y方向错位）
  const inputBottom = Number(input1.rect.bottom);
  const btnRowTop = Number(btnRow.rect.top);
  if (btnRowTop + epsilon < inputBottom) {
    throw new Error(`input 与按钮行发生重叠(疑似Y偏移): inputBottom=${inputBottom}, btnRowTop=${btnRowTop}`);
  }

  // 3) card2 按钮行高度回归：SetTextContent 不应因文本测量误差产生双行高度
  const card2BtnCandidates = [...nodesByPath.entries()]
    .filter(([nodePath, node]) => nodePath.startsWith('card2.') && Number(node.rect.w) > 0 && Number(node.rect.h) > 20)
    .sort((a, b) => {
      const topDelta = Number(b[1].rect.top) - Number(a[1].rect.top);
      if (Math.abs(topDelta) > epsilon) return topDelta;
      return Number(a[1].rect.left) - Number(b[1].rect.left);
    });
  const card2BtnSetText = card2BtnCandidates[0] && card2BtnCandidates[0][1];
  const card2BtnRefill = card2BtnCandidates[1] && card2BtnCandidates[1][1];
  if (!card2BtnSetText || !card2BtnRefill) {
    throw new Error('缺少 card2.4.0/4.1 按钮布局节点，无法执行按钮高度断言');
  }

  const card2BtnSetTextHeight = Number(card2BtnSetText.rect.h);
  const card2BtnRefillHeight = Number(card2BtnRefill.rect.h);
  if (Math.abs(card2BtnSetTextHeight - card2BtnRefillHeight) > epsilon) {
    throw new Error(
      `card2 按钮高度不一致(疑似文本测量导致换行): SetTextContent=${card2BtnSetTextHeight}, 重新填充=${card2BtnRefillHeight}`
    );
  }

  console.log('[PASS] 布局基线 + 溢出回归检查通过');
  console.log(`[INFO] 捕获布局节点总数: ${layoutLines.length}`);
  console.log(`[INFO] card1.right=${cardRight}, input.right=${inputRight}, input.width=${inputWidth}`);
  console.log(`[INFO] 对齐检查: title.left=${titleLeft}, tag.left=${tagLeft}, input.left=${inputLeft}, btnRow.left=${btnRowLeft}`);
  console.log(`[INFO] 纵向检查: input.bottom=${inputBottom}, btnRow.top=${btnRowTop}`);
  console.log(`[INFO] card2按钮高度: SetTextContent=${card2BtnSetTextHeight}, 重新填充=${card2BtnRefillHeight}`);
}

run();

