/*
 * CSS 动画最小测试（transform/opacity）
 * 使用方法: esm_loader.exe test_css_animation_min.js
 */

(function () {
    'use strict';

    console.log('=== CSS 动画最小测试 ===');

    var style = document.createElement('style');
    style.textContent = `
        @keyframes moveFade {
            from { transform: translateX(0px); opacity: 0.2; }
            to { transform: translateX(160px); opacity: 1; }
        }
        @keyframes colorShift {
            0% { background: #e74c3c; }
            50% { background: #f1c40f; }
            100% { background: #2ecc71; }
        }
        body {
            margin: 0;
            background: #1e1e1e;
            color: #fff;
            font-family: Arial, sans-serif;
        }
        .panel {
            padding: 24px;
        }
        .row {
            display: flex;
            gap: 16px;
            align-items: center;
            margin-bottom: 16px;
        }
        .box {
            width: 80px;
            height: 80px;
            border-radius: 12px;
            background: #3498db;
            animation: moveFade 1.8s linear infinite;
        }
        .color-box {
            width: 80px;
            height: 80px;
            border-radius: 12px;
            animation: colorShift 1.8s linear infinite;
        }
        .label {
            font-size: 14px;
            opacity: 0.9;
        }
    `;
    document.head.appendChild(style);

    var panel = document.createElement('div');
    panel.className = 'panel';

    var row1 = document.createElement('div');
    row1.className = 'row';
    var box1 = document.createElement('div');
    box1.className = 'box';
    var label1 = document.createElement('div');
    label1.className = 'label';
    label1.textContent = 'transform + opacity';
    row1.appendChild(box1);
    row1.appendChild(label1);

    var row2 = document.createElement('div');
    row2.className = 'row';
    var box2 = document.createElement('div');
    box2.className = 'color-box';
    var label2 = document.createElement('div');
    label2.className = 'label';
    label2.textContent = 'background-color';
    row2.appendChild(box2);
    row2.appendChild(label2);

    panel.appendChild(row1);
    panel.appendChild(row2);
    document.body.appendChild(panel);

    console.log('渲染完成');
})();

