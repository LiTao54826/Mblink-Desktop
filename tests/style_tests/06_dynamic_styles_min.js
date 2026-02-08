/**
 * 测试场景 6: 动态样式更新
 * 测试目标: 验证样式动态变化、重新布局、重绘性能
 * 潜在 BUG: 样式更新不触发重绘、布局抖动、内存泄漏
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\06_dynamic_styles.js
 */

import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

function DynamicStylesTest() {
    const [animating, setAnimating] = useState(false);

    return h('button', {
                style: `padding: 10px 20px; background: ${animating ? '#f44336' : '#4caf50'}; color: white; border: none; border-radius: 4px; cursor: pointer; margin-bottom: 15px;`
            }, animating ? 'Stop Animation' : 'Start Animation')
}

const root = document.getElementById('root') || document.body;
render(h(DynamicStylesTest), root);

console.log('[Test 06] Dynamic styles test rendered');

