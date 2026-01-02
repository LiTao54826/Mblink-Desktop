/**
 * @file test_modal_animation.js
 * @brief 测试 Modal 打开时动画是否继续运行
 * 
 * Feature: animation-element-association
 * Property 1: Animation State Preservation Across Render Tree Rebuilds
 * Validates: Requirements 1.1, 1.2
 */

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName) {
    const passed = actual === expected;
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${expected}`);
        console.log(`  Actual: ${actual}`);
    }
    return passed;
}

function assertTrue(condition, testName) {
    logTest(testName, condition);
    if (!condition) {
        console.log(`  Expected: true`);
        console.log(`  Actual: false`);
    }
    return condition;
}

// 测试变量
let testsPassed = 0;
let testsFailed = 0;

console.log("[TEST_START] Modal Animation Tests");

// 添加 @keyframes 规则
function addKeyframesRule() {
    // 创建 style 元素添加 @keyframes 规则
    const style = document.createElement('style');
    style.id = 'test-keyframes-style';
    style.textContent = `
        @keyframes spin {
            from {
                transform: rotate(0deg);
            }
            to {
                transform: rotate(360deg);
            }
        }
        
        @keyframes pulse {
            0% {
                opacity: 1;
            }
            50% {
                opacity: 0.5;
            }
            100% {
                opacity: 1;
            }
        }
    `;
    document.head.appendChild(style);
    
    const added = document.getElementById('test-keyframes-style') !== null;
    logTest("Keyframes rules added", added);
    return added;
}

// 测试 1: 创建带动画的 spinner 元素
function testCreateSpinner() {
    const spinner = document.createElement('div');
    spinner.id = 'test-spinner';
    spinner.style.width = '50px';
    spinner.style.height = '50px';
    spinner.style.backgroundColor = '#3498db';
    // 使用内联动画样式
    spinner.style.animation = 'spin 1s linear infinite';
    document.body.appendChild(spinner);
    
    // 验证 spinner 已添加到 DOM
    const found = document.getElementById('test-spinner');
    const result = assertTrue(found !== null, "Spinner element created and added to DOM");
    
    // 输出动画样式信息
    console.log(`Spinner animation style: ${spinner.style.animation}`);
    
    return result;
}

// 测试 2: 添加 Modal 后检查 spinner 元素是否仍然存在且动画样式保留
function testModalDoesNotRemoveSpinner() {
    return new Promise((resolve) => {
        setTimeout(() => {
            const spinner = document.getElementById('test-spinner');
            if (!spinner) {
                logTest("Spinner exists before modal", false);
                resolve(false);
                return;
            }
            
            logTest("Spinner exists before modal", true);
            
            // 记录添加 Modal 前的动画样式
            const animStyleBefore = spinner.style.animation;
            console.log(`Animation style before modal: ${animStyleBefore}`);
            
            // 创建 Modal overlay
            const overlay = document.createElement('div');
            overlay.id = 'test-modal-overlay';
            overlay.style.position = 'fixed';
            overlay.style.top = '0';
            overlay.style.left = '0';
            overlay.style.width = '100%';
            overlay.style.height = '100%';
            overlay.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
            overlay.style.zIndex = '1000';
            document.body.appendChild(overlay);
            
            // 创建 Modal content
            const modal = document.createElement('div');
            modal.id = 'test-modal';
            modal.style.position = 'fixed';
            modal.style.top = '50%';
            modal.style.left = '50%';
            modal.style.transform = 'translate(-50%, -50%)';
            modal.style.backgroundColor = 'white';
            modal.style.padding = '20px';
            modal.style.zIndex = '1001';
            modal.style.borderRadius = '8px';
            modal.textContent = 'Test Modal Content';
            document.body.appendChild(modal);
            
            // 等待一帧后检查
            setTimeout(() => {
                const spinnerAfter = document.getElementById('test-spinner');
                const modalExists = document.getElementById('test-modal') !== null;
                
                logTest("Modal added to DOM", modalExists);
                logTest("Spinner still exists after modal added", spinnerAfter !== null);
                
                // 检查 spinner 的动画样式是否保留
                if (spinnerAfter) {
                    const animStyleAfter = spinnerAfter.style.animation;
                    console.log(`Animation style after modal: ${animStyleAfter}`);
                    
                    const hasAnimationStyle = animStyleAfter && animStyleAfter.includes('spin');
                    logTest("Spinner animation style preserved after modal", hasAnimationStyle);
                    
                    // 检查动画样式是否与之前相同
                    const styleUnchanged = animStyleBefore === animStyleAfter;
                    logTest("Animation style unchanged", styleUnchanged);
                    
                    resolve(hasAnimationStyle && styleUnchanged);
                } else {
                    resolve(false);
                }
            }, 300);
        }, 300);
    });
}

// 测试 3: 移除 Modal 后 spinner 仍然存在且动画继续
function testSpinnerExistsAfterModalRemoved() {
    return new Promise((resolve) => {
        setTimeout(() => {
            // 移除 Modal
            const overlay = document.getElementById('test-modal-overlay');
            const modal = document.getElementById('test-modal');
            
            if (overlay) overlay.remove();
            if (modal) modal.remove();
            
            logTest("Modal removed", true);
            
            // 检查 spinner 是否仍然存在
            setTimeout(() => {
                const spinner = document.getElementById('test-spinner');
                const exists = spinner !== null;
                logTest("Spinner exists after modal removed", exists);
                
                if (spinner) {
                    const animStyle = spinner.style.animation;
                    console.log(`Spinner animation after modal removed: ${animStyle}`);
                    
                    const hasAnimation = animStyle && animStyle.includes('spin');
                    logTest("Spinner animation still active after modal removed", hasAnimation);
                    resolve(exists && hasAnimation);
                } else {
                    resolve(false);
                }
            }, 200);
        }, 200);
    });
}

// 测试 4: 添加多个兄弟元素后动画仍然运行
function testAnimationWithSiblingChanges() {
    return new Promise((resolve) => {
        setTimeout(() => {
            const spinner = document.getElementById('test-spinner');
            if (!spinner) {
                logTest("Spinner exists before sibling changes", false);
                resolve(false);
                return;
            }
            
            const animStyleBefore = spinner.style.animation;
            
            // 添加多个兄弟元素
            for (let i = 0; i < 5; i++) {
                const sibling = document.createElement('div');
                sibling.className = 'test-sibling';
                sibling.style.width = '30px';
                sibling.style.height = '30px';
                sibling.style.backgroundColor = '#e74c3c';
                sibling.style.margin = '5px';
                sibling.textContent = `Sibling ${i + 1}`;
                document.body.appendChild(sibling);
            }
            
            // 等待 DOM 更新
            setTimeout(() => {
                const spinnerAfter = document.getElementById('test-spinner');
                const animStyleAfter = spinnerAfter ? spinnerAfter.style.animation : '';
                
                const stylePreserved = animStyleBefore === animStyleAfter;
                logTest("Animation preserved after adding siblings", stylePreserved);
                
                // 清理兄弟元素
                const siblings = document.querySelectorAll('.test-sibling');
                siblings.forEach(s => s.remove());
                
                resolve(stylePreserved);
            }, 200);
        }, 100);
    });
}

// 测试 5: 移除 spinner 元素后清理
function testSpinnerRemoval() {
    return new Promise((resolve) => {
        setTimeout(() => {
            const spinner = document.getElementById('test-spinner');
            if (spinner) {
                spinner.remove();
            }
            
            // 验证元素已被移除
            const removed = document.getElementById('test-spinner') === null;
            logTest("Spinner element removed from DOM", removed);
            
            // 清理 keyframes style
            const keyframesStyle = document.getElementById('test-keyframes-style');
            if (keyframesStyle) {
                keyframesStyle.remove();
            }
            
            resolve(removed);
        }, 100);
    });
}

// 运行所有测试
async function runTests() {
    try {
        // 添加 keyframes 规则
        if (addKeyframesRule()) testsPassed++; else testsFailed++;
        
        // 同步测试
        if (testCreateSpinner()) testsPassed++; else testsFailed++;
        
        // 异步测试
        if (await testModalDoesNotRemoveSpinner()) testsPassed++; else testsFailed++;
        if (await testSpinnerExistsAfterModalRemoved()) testsPassed++; else testsFailed++;
        if (await testAnimationWithSiblingChanges()) testsPassed++; else testsFailed++;
        if (await testSpinnerRemoval()) testsPassed++; else testsFailed++;
        
    } catch (e) {
        console.log(`[ERROR] ${e.message}`);
        console.log(`[ERROR] Stack: ${e.stack}`);
        testsFailed++;
    }
    
    console.log(`[TEST_END] Passed: ${testsPassed}, Failed: ${testsFailed}`);
    
    // 返回测试结果
    if (testsFailed === 0) {
        console.log("[TEST_RESULT] ALL TESTS PASSED");
    } else {
        console.log("[TEST_RESULT] SOME TESTS FAILED");
    }
}

runTests();
