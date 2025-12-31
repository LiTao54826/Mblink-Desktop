/**
 * 测试 position: fixed 元素的布局
 * 
 * 问题：Modal 弹窗使用 position: fixed + flexbox 居中，
 * 但渲染位置和实际位置不一致
 */

import { h, render } from 'preact';

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName, tolerance = 0.1) {
    const passed = Math.abs(actual - expected) <= tolerance;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function assertInRange(actual, min, max, testName) {
    const passed = actual >= min && actual <= max;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${min} - ${max}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

// 测试组件
function TestApp() {
    return h('div', { id: 'app', style: { width: '800px', height: '600px' } }, [
        // 测试1: position: fixed 全屏覆盖
        h('div', {
            id: 'fixed-fullscreen',
            style: {
                position: 'fixed',
                top: 0,
                left: 0,
                right: 0,
                bottom: 0,
                backgroundColor: 'rgba(0, 0, 0, 0.5)',
            }
        }),
        
        // 测试2: position: fixed + flexbox 居中
        h('div', {
            id: 'fixed-flex-center',
            style: {
                position: 'fixed',
                top: 0,
                left: 0,
                right: 0,
                bottom: 0,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
            }
        }, [
            h('div', {
                id: 'modal-content',
                style: {
                    width: '200px',
                    height: '100px',
                    backgroundColor: 'white',
                }
            })
        ]),
        
        // 测试3: position: fixed 指定位置
        h('div', {
            id: 'fixed-positioned',
            style: {
                position: 'fixed',
                top: '50px',
                left: '100px',
                width: '150px',
                height: '80px',
                backgroundColor: 'blue',
            }
        }),
    ]);
}

// 运行测试
console.log("[TEST_START] Fixed Modal Layout Tests");

render(h(TestApp), document.body);

// 强制同步布局
document.body.offsetHeight;

// 获取视口尺寸
const viewportWidth = window.innerWidth || 800;
const viewportHeight = window.innerHeight || 600;
console.log(`[INFO] Viewport (window.innerWidth/Height): ${viewportWidth}x${viewportHeight}`);

// 检查 document.documentElement 尺寸
const htmlRect = document.documentElement.getBoundingClientRect();
console.log(`[INFO] html rect: x=${htmlRect.x}, y=${htmlRect.y}, w=${htmlRect.width}, h=${htmlRect.height}`);

// 检查 body 元素的尺寸
const bodyRect = document.body.getBoundingClientRect();
console.log(`[INFO] body rect: x=${bodyRect.x}, y=${bodyRect.y}, w=${bodyRect.width}, h=${bodyRect.height}`);

// 检查 #app 元素的尺寸
const app = document.getElementById('app');
if (app) {
    const appRect = app.getBoundingClientRect();
    console.log(`[INFO] #app rect: x=${appRect.x}, y=${appRect.y}, w=${appRect.width}, h=${appRect.height}`);
}

// 测试1: fixed 全屏覆盖
const fullscreen = document.getElementById('fixed-fullscreen');
if (fullscreen) {
    const rect = fullscreen.getBoundingClientRect();
    console.log(`[INFO] fixed-fullscreen rect: x=${rect.x}, y=${rect.y}, w=${rect.width}, h=${rect.height}`);
    
    assertEqual(rect.x, 0, "Fixed fullscreen x should be 0");
    assertEqual(rect.y, 0, "Fixed fullscreen y should be 0");
    assertEqual(rect.width, viewportWidth, "Fixed fullscreen width should match viewport", 1);
    assertEqual(rect.height, viewportHeight, "Fixed fullscreen height should match viewport", 1);
} else {
    console.log("[TEST_FAIL] fixed-fullscreen element not found");
}

// 测试2: fixed + flexbox 居中
const flexCenter = document.getElementById('fixed-flex-center');
const modalContent = document.getElementById('modal-content');
if (flexCenter && modalContent) {
    const containerRect = flexCenter.getBoundingClientRect();
    const contentRect = modalContent.getBoundingClientRect();
    
    console.log(`[INFO] fixed-flex-center rect: x=${containerRect.x}, y=${containerRect.y}, w=${containerRect.width}, h=${containerRect.height}`);
    console.log(`[INFO] modal-content rect: x=${contentRect.x}, y=${contentRect.y}, w=${contentRect.width}, h=${contentRect.height}`);
    
    // 容器应该全屏
    assertEqual(containerRect.x, 0, "Flex container x should be 0");
    assertEqual(containerRect.y, 0, "Flex container y should be 0");
    assertEqual(containerRect.width, viewportWidth, "Flex container width should match viewport", 1);
    assertEqual(containerRect.height, viewportHeight, "Flex container height should match viewport", 1);
    
    // 内容应该居中
    const expectedX = (viewportWidth - 200) / 2;
    const expectedY = (viewportHeight - 100) / 2;
    console.log(`[INFO] Expected modal position: x=${expectedX}, y=${expectedY}`);
    
    assertEqual(contentRect.width, 200, "Modal content width should be 200");
    assertEqual(contentRect.height, 100, "Modal content height should be 100");
    assertInRange(contentRect.x, expectedX - 5, expectedX + 5, "Modal content should be horizontally centered");
    assertInRange(contentRect.y, expectedY - 5, expectedY + 5, "Modal content should be vertically centered");
} else {
    console.log("[TEST_FAIL] fixed-flex-center or modal-content element not found");
}

// 测试3: fixed 指定位置
const positioned = document.getElementById('fixed-positioned');
if (positioned) {
    const rect = positioned.getBoundingClientRect();
    console.log(`[INFO] fixed-positioned rect: x=${rect.x}, y=${rect.y}, w=${rect.width}, h=${rect.height}`);
    
    assertEqual(rect.x, 100, "Fixed positioned x should be 100");
    assertEqual(rect.y, 50, "Fixed positioned y should be 50");
    assertEqual(rect.width, 150, "Fixed positioned width should be 150");
    assertEqual(rect.height, 80, "Fixed positioned height should be 80");
} else {
    console.log("[TEST_FAIL] fixed-positioned element not found");
}

console.log("[TEST_END]");
