// Regression check: when a filter button changes from active to inactive,
// its child text must inherit the new color instead of keeping the old white
// paint and disappearing on a white background.

const { execFileSync } = require('child_process');
const path = require('path');

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function runMbinkUiDev(args) {
  const exe = path.join(process.cwd(), 'build', 'bin', 'Release', 'mbink-ui-dev.exe');
  const output = execFileSync(exe, args, {
    cwd: process.cwd(),
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
  });
  return JSON.parse(output);
}

function evalState(project) {
  const code = `
    JSON.stringify(Array.from(document.querySelectorAll('div:nth-child(3) button')).map((button) => ({
      text: button.textContent,
      inlineColor: button.getAttribute('style').match(/color:\\s*([^;]+)/)?.[1] || '',
      computedColor: getComputedStyle(button).color
    })))
  `;
  const result = runMbinkUiDev(['eval', '--project', project, code]);
  assert(result.ok === true, 'eval_state failed');
  return JSON.parse(result.result);
}

function click(project, index) {
  const result = runMbinkUiDev([
    'click',
    '--project',
    project,
    `div:nth-child(3) button:nth-child(${index})`,
  ]);
  assert(result.ok === true && result.result.clicked === true, `click ${index} failed`);
}

function clickSelector(project, selector) {
  const result = runMbinkUiDev(['click', '--project', project, selector]);
  assert(result.ok === true && result.result.clicked === true, `click ${selector} failed`);
}

function inputText(project, selector, text) {
  const result = runMbinkUiDev(['input-text', '--project', project, selector, text]);
  assert(result.ok === true && result.result.value === text, `input ${selector} failed`);
}

function evalJson(project, code) {
  const result = runMbinkUiDev(['eval', '--project', project, `JSON.stringify(${code})`]);
  assert(result.ok === true, 'eval_json failed');
  return JSON.parse(result.result);
}

function assertButtons(state, activeIndex) {
  assert(state.length === 3, `expected 3 filter buttons, got ${state.length}`);
  const labels = state.map((button) => button.text).join(',');
  assert(labels === 'All,Active,Done', `filter labels changed: ${labels}`);

  state.forEach((button, index) => {
    const expectedColor = index === activeIndex ? '#fff' : '#555';
    assert(
      button.inlineColor === expectedColor,
      `${button.text} inline color expected ${expectedColor}, got ${button.inlineColor}`
    );
    assert(
      button.computedColor !== 'rgb(255, 255, 255)' || index === activeIndex,
      `${button.text} kept active white computed color while inactive`
    );
  });
}

function run() {
  const project = path.join(process.cwd(), 'examples', 'todo_app_js');

  runMbinkUiDev(['daemon', 'stop', '--project', project]);
  runMbinkUiDev(['open', project, '--force']);

  assertButtons(evalState(project), 0);
  click(project, 2);
  assertButtons(evalState(project), 1);
  click(project, 3);
  assertButtons(evalState(project), 2);
  click(project, 1);
  assertButtons(evalState(project), 0);

  inputText(project, '#todo-input', 'checkbox guard task');
  clickSelector(project, 'button[type="submit"]');
  clickSelector(project, 'input[type="checkbox"]');
  const checkedState = evalJson(project, `({
    checked: Array.from(document.querySelectorAll('input[type=checkbox]')).map((input) => input.checked),
    texts: Array.from(document.querySelectorAll('span')).map((span) => span.textContent)
  })`);
  assert(checkedState.checked.length === 1 && checkedState.checked[0] === true,
    'dev click did not toggle checkbox checked state');
  assert(checkedState.texts.includes('0 active / 1 done'),
    `footer did not reflect checked todo: ${checkedState.texts.join('|')}`);
  click(project, 3);
  const doneState = evalJson(project, `({
    texts: Array.from(document.querySelectorAll('span')).map((span) => span.textContent)
  })`);
  assert(doneState.texts.includes('checkbox guard task'),
    `done filter did not show checked todo: ${doneState.texts.join('|')}`);

  console.log('[PASS] Todo filter button inherited color guard');
}

run();
