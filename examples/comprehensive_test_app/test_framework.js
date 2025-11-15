/**
 * @file test_framework.js
 * @brief 简单的测试框架
 */

// 全局测试状态
const globalTestState = {
    tests: [],
    results: {
        passed: 0,
        failed: 0,
        total: 0
    },
    currentSuite: null
};

/**
 * 断言函数
 */
function assert(condition, message) {
    if (!condition) {
        throw new Error('断言失败: ' + message);
    }
}

function assertEqual(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(`断言失败: ${message}\n  期望: ${expected}\n  实际: ${actual}`);
    }
}

function assertNotEqual(actual, expected, message) {
    if (actual === expected) {
        throw new Error(`断言失败: ${message}\n  不应等于: ${expected}`);
    }
}

function assertNull(value, message) {
    if (value !== null) {
        throw new Error(`断言失败: ${message}\n  期望: null\n  实际: ${value}`);
    }
}

function assertNotNull(value, message) {
    if (value === null) {
        throw new Error(`断言失败: ${message}\n  值不应为 null`);
    }
}

function assertTrue(value, message) {
    if (value !== true) {
        throw new Error(`断言失败: ${message}\n  期望: true\n  实际: ${value}`);
    }
}

function assertFalse(value, message) {
    if (value !== false) {
        throw new Error(`断言失败: ${message}\n  期望: false\n  实际: ${value}`);
    }
}

/**
 * 测试套件
 */
function describe(suiteName, fn) {
    globalTestState.currentSuite = suiteName;
    console.log('\n📦 测试套件: ' + suiteName);
    console.log('─'.repeat(60));
    fn();
    globalTestState.currentSuite = null;
}

/**
 * 测试用例
 */
function test(testName, fn) {
    const fullName = globalTestState.currentSuite 
        ? `${globalTestState.currentSuite} > ${testName}`
        : testName;
    
    globalTestState.tests.push({
        name: fullName,
        fn: fn
    });
}

/**
 * 异步测试用例
 */
function testAsync(testName, fn) {
    const fullName = globalTestState.currentSuite 
        ? `${globalTestState.currentSuite} > ${testName}`
        : testName;
    
    globalTestState.tests.push({
        name: fullName,
        fn: fn,
        async: true
    });
}

/**
 * 运行所有测试
 */
function runAllTests() {
    console.log('\n');
    console.log('═'.repeat(60));
    console.log('🚀 开始运行测试');
    console.log('═'.repeat(60));
    
    for (const test of globalTestState.tests) {
        try {
            // 运行测试
            test.fn();
            
            // 测试通过
            globalTestState.results.passed++;
            console.log(`✅ ${test.name}`);
            
        } catch (error) {
            // 测试失败
            globalTestState.results.failed++;
            console.log(`❌ ${test.name}`);
            console.log(`   错误: ${error.message}`);
        }
        
        globalTestState.results.total++;
    }
    
    // 显示结果
    displayResults();
}

/**
 * 显示测试结果
 */
function displayResults() {
    const { passed, failed, total } = globalTestState.results;
    const passRate = total > 0 ? ((passed / total) * 100).toFixed(2) : 0;
    
    console.log('\n');
    console.log('═'.repeat(60));
    console.log('📊 测试结果');
    console.log('═'.repeat(60));
    console.log(`✅ 通过: ${passed}`);
    console.log(`❌ 失败: ${failed}`);
    console.log(`📝 总计: ${total}`);
    console.log(`📈 通过率: ${passRate}%`);
    console.log('═'.repeat(60));
    
    if (passRate >= 90) {
        console.log('🎉 优秀！所有测试基本通过！');
    } else if (passRate >= 80) {
        console.log('👍 良好！大部分测试通过！');
    } else {
        console.log('⚠️  警告：测试通过率较低，需要修复！');
    }
    
    console.log('═'.repeat(60));
}

/**
 * 辅助函数：创建测试元素
 */
function createTestElement(tagName, options = {}) {
    const element = document.createElement(tagName);
    
    if (options.id) element.id = options.id;
    if (options.className) element.className = options.className;
    if (options.textContent) element.textContent = options.textContent;
    if (options.innerHTML) element.innerHTML = options.innerHTML;
    
    if (options.attributes) {
        for (const [key, value] of Object.entries(options.attributes)) {
            element.setAttribute(key, value);
        }
    }
    
    if (options.styles) {
        for (const [key, value] of Object.entries(options.styles)) {
            element.style.setProperty(key, value);
        }
    }
    
    return element;
}

/**
 * 辅助函数：清理测试
 */
function cleanup() {
    // 清空 body
    if (document.body) {
        document.body.innerHTML = '';
    }
}

// 导出到全局
globalThis.assert = assert;
globalThis.assertEqual = assertEqual;
globalThis.assertNotEqual = assertNotEqual;
globalThis.assertNull = assertNull;
globalThis.assertNotNull = assertNotNull;
globalThis.assertTrue = assertTrue;
globalThis.assertFalse = assertFalse;
globalThis.describe = describe;
globalThis.test = test;
globalThis.testAsync = testAsync;
globalThis.runAllTests = runAllTests;
globalThis.createTestElement = createTestElement;
globalThis.cleanup = cleanup;

console.log('✅ 测试框架加载完成');

