/**
 * 测试 Modal 弹窗中按钮的点击
 * 
 * 问题：Modal 弹窗中的按钮不能正常被点击
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

// 简化的 Modal 结构
function TestModal() {
    return h('div', {
        id: 'modal-mask',
        style: {
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            backgroundColor: 'rgba(0, 0, 0, 0.5)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 1000,
        }
    }, [
        h('div', {
            id: 'modal-content',
            style: {
                width: '400px',
                height: '200px',
                backgroundColor: 'white',
                borderRadius: '8px',
                display: 'flex',
                flexDirection: 'column',
            }
        }, [
            h('div', {
                id: 'modal-header',
                style: {
                    padding: '16px',
                    borderBottom: '1px solid #eee',
                }
            }, 'Modal Title'),
            h('div', {
                id: 'modal-body',
                style: {
                    padding: '16px',
                    flex: 1,
                }
            }, 'Modal content here'),
            h('div', {
                id: 'modal-footer',
                style: {
                    padding: '16px',
                    borderTop: '1px solid #eee',
                    display: 'flex',
                    justifyContent: 'flex-end',
                    gap: '12px',
                }
            }, [
                h('button', {
                    id: 'cancel-btn',
                    style: {
                        padding: '8px 16px',
                        backgroundColor: '#f0f0f0',
                        border: 'none',
                        borderRadius: '4px',
                    }
                }, 'Cancel'),
                h('button', {
                    id: 'ok-btn',
                    style: {
                        padding: '8px 16px',
                        backgroundColor: '#1890ff',
                        color: 'white',
                        border: 'none',
                        borderRadius: '4px',
                    }
                }, 'OK'),
            ]),
        ]),
    ]);
}

// 运行测试
console.log("[TEST_START] Modal Button Click Tests");

render(h(TestModal), document.body);

// 强制同步布局
document.body.offsetHeight;

// 获取视口尺寸
const viewportWidth = window.innerWidth || 800;
const viewportHeight = window.innerHeight || 600;
console.log(`[INFO] Viewport: ${viewportWidth}x${viewportHeight}`);

// 检查 modal-mask
const mask = document.getElementById('modal-mask');
if (mask) {
    const maskRect = mask.getBoundingClientRect();
    console.log(`[INFO] modal-mask rect: x=${maskRect.x}, y=${maskRect.y}, w=${maskRect.width}, h=${maskRect.height}`);
    
    assertEqual(maskRect.x, 0, "Modal mask x should be 0");
    assertEqual(maskRect.y, 0, "Modal mask y should be 0");
    assertEqual(maskRect.width, viewportWidth, "Modal mask width should match viewport", 1);
    assertEqual(maskRect.height, viewportHeight, "Modal mask height should match viewport", 1);
} else {
    console.log("[TEST_FAIL] modal-mask element not found");
}

// 检查 modal-content
const content = document.getElementById('modal-content');
if (content) {
    const contentRect = content.getBoundingClientRect();
    console.log(`[INFO] modal-content rect: x=${contentRect.x}, y=${contentRect.y}, w=${contentRect.width}, h=${contentRect.height}`);
    
    // 内容应该居中
    const expectedX = (viewportWidth - 400) / 2;
    const expectedY = (viewportHeight - 200) / 2;
    console.log(`[INFO] Expected modal position: x=${expectedX}, y=${expectedY}`);
    
    assertEqual(contentRect.width, 400, "Modal content width should be 400");
    assertEqual(contentRect.height, 200, "Modal content height should be 200");
    assertEqual(contentRect.x, expectedX, "Modal content should be horizontally centered", 5);
    assertEqual(contentRect.y, expectedY, "Modal content should be vertically centered", 5);
} else {
    console.log("[TEST_FAIL] modal-content element not found");
}

// 检查 OK 按钮
const okBtn = document.getElementById('ok-btn');
if (okBtn) {
    const btnRect = okBtn.getBoundingClientRect();
    console.log(`[INFO] ok-btn rect: x=${btnRect.x}, y=${btnRect.y}, w=${btnRect.width}, h=${btnRect.height}`);
    
    // 按钮应该在 modal 内部
    const contentRect = content.getBoundingClientRect();
    const btnInModal = btnRect.x >= contentRect.x && 
                       btnRect.y >= contentRect.y &&
                       btnRect.x + btnRect.width <= contentRect.x + contentRect.width &&
                       btnRect.y + btnRect.height <= contentRect.y + contentRect.height;
    logTest("OK button should be inside modal content", btnInModal);
    
    // 按钮应该有合理的尺寸
    logTest("OK button should have positive width", btnRect.width > 0);
    logTest("OK button should have positive height", btnRect.height > 0);
} else {
    console.log("[TEST_FAIL] ok-btn element not found");
}

// 检查 Cancel 按钮
const cancelBtn = document.getElementById('cancel-btn');
if (cancelBtn) {
    const btnRect = cancelBtn.getBoundingClientRect();
    console.log(`[INFO] cancel-btn rect: x=${btnRect.x}, y=${btnRect.y}, w=${btnRect.width}, h=${btnRect.height}`);
    
    logTest("Cancel button should have positive width", btnRect.width > 0);
    logTest("Cancel button should have positive height", btnRect.height > 0);
} else {
    console.log("[TEST_FAIL] cancel-btn element not found");
}

console.log("[TEST_END]");
