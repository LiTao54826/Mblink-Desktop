// 布局回归测试脚本（基于 esm_loader 输出日志）
// 用法：
// node tests/js/layout_incremental_update_check.js

const { execSync } = require('child_process');

function run() {
  const cmd = 'build\\bin\\Release\\esm_loader.exe examples\\incremental_update_demo\\app.js -q 3';
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

  // 核心回归：card1 input (card1.3) 不能横向溢出 card1
  const card1 = nodesByPath.get('card1');
  const input1 = nodesByPath.get('card1.3');
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
  const tag = nodesByPath.get('card1.1');
  const desc = nodesByPath.get('card1.2');
  const btnRow = nodesByPath.get('card1.4');
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
  const card2BtnSetText = nodesByPath.get('card2.4.0');
  const card2BtnRefill = nodesByPath.get('card2.4.1');
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

