/**
 * 测试圆形元素的 box-shadow 渲染
 * 问题：position: fixed 元素的阴影应该是圆形的，但实际渲染成矩形
 */

function h(tag, props, ...children) {
    const element = document.createElement(tag);

    if (props) {
        for (const [key, value] of Object.entries(props)) {
            if (key === 'style') {
                element.setAttribute('style', value);
            } else {
                element.setAttribute(key, value);
            }
        }
    }

    for (const child of children) {
        if (typeof child === 'string') {
            element.appendChild(document.createTextNode(child));
        } else if (child) {
            element.appendChild(child);
        }
    }

    return element;
}

// 测试 1：非 fixed 圆形按钮（用于对比）
const normalButton = h('div', {
    style: 'position: absolute; top: 100px; left: 100px; width: 60px; height: 60px; background: #4caf50; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 12px 24px 12px rgba(255,0,0,0.8); cursor: pointer;'
}, '✓');

// 测试 2：fixed 圆形按钮（右下角）
const fixedButton = h('div', {
    style: 'position: fixed; bottom: 20px; right: 20px; width: 60px; height: 60px; background: #9c27b0; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 12px 24px 12px rgba(255,0,0,0.8); cursor: pointer; z-index: 1000;'
}, '+');

// 测试 3：fixed 圆形按钮（左下角）
const fixedButton2 = h('div', {
    style: 'position: fixed; bottom: 20px; left: 20px; width: 60px; height: 60px; background: #e91e63; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 12px 24px 12px rgba(255,0,0,0.8); cursor: pointer; z-index: 1000;'
}, '-');

// document.body.appendChild(normalButton);
// document.body.appendChild(fixedButton);
document.body.appendChild(fixedButton2);


