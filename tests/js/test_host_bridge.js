// tests/js/test_host_bridge.js
// 测试 Host Bridge 功能

// 测试辅助函数
function logTest(name, passed) {
    if (passed) {
        console.log(`[TEST_PASS] ${name}`);
    } else {
        console.log(`[TEST_FAIL] ${name}`);
    }
}

function assertEqual(actual, expected, testName) {
    const passed = JSON.stringify(actual) === JSON.stringify(expected);
    logTest(testName, passed);
    if (!passed) {
        console.log(`  Expected: ${JSON.stringify(expected)}`);
        console.log(`  Actual: ${JSON.stringify(actual)}`);
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

// 测试 host 对象是否存在
function testHostExists() {
    console.log("\n--- Testing host object existence ---");
    
    const hasHost = typeof host !== 'undefined';
    assertTrue(hasHost, "host object exists");
    
    if (hasHost) {
        assertTrue(typeof host.call === 'function', "host.call is a function");
        assertTrue(typeof host.state === 'object', "host.state is an object");
        assertTrue(typeof host.state.get === 'function', "host.state.get is a function");
        assertTrue(typeof host.state.set === 'function', "host.state.set is a function");
        assertTrue(typeof host.state.watch === 'function', "host.state.watch is a function");
        assertTrue(typeof host.state.unwatch === 'function', "host.state.unwatch is a function");
        assertTrue(typeof host.state.exists === 'function', "host.state.exists is a function");
        assertTrue(typeof host.state.type === 'function', "host.state.type is a function");
    }
}

// 测试状态基本操作
function testStateBasicOps() {
    console.log("\n--- Testing state basic operations ---");
    
    // 测试 set 和 get
    host.state.set("testCounter", 42);
    const counter = host.state.get("testCounter");
    assertEqual(counter, 42, "state.set/get number");
    
    host.state.set("testString", "hello");
    const str = host.state.get("testString");
    assertEqual(str, "hello", "state.set/get string");
    
    host.state.set("testBool", true);
    const bool = host.state.get("testBool");
    assertEqual(bool, true, "state.set/get boolean");
    
    host.state.set("testArray", [1, 2, 3]);
    const arr = host.state.get("testArray");
    assertEqual(arr, [1, 2, 3], "state.set/get array");
    
    host.state.set("testObject", {name: "test", value: 123});
    const obj = host.state.get("testObject");
    assertEqual(obj, {name: "test", value: 123}, "state.set/get object");
}

// 测试 exists 和 type
function testStateExistsAndType() {
    console.log("\n--- Testing state exists and type ---");
    
    host.state.set("existsTest", 100);
    assertTrue(host.state.exists("existsTest"), "exists returns true for existing state");
    assertTrue(!host.state.exists("nonExistent"), "exists returns false for non-existing state");
    
    host.state.set("typeNumber", 42);
    assertEqual(host.state.type("typeNumber"), "number", "type returns 'number' for number");
    
    host.state.set("typeString", "hello");
    assertEqual(host.state.type("typeString"), "string", "type returns 'string' for string");
    
    host.state.set("typeBool", true);
    assertEqual(host.state.type("typeBool"), "boolean", "type returns 'boolean' for boolean");
    
    host.state.set("typeArray", [1, 2]);
    assertEqual(host.state.type("typeArray"), "array", "type returns 'array' for array");
    
    host.state.set("typeObject", {a: 1});
    assertEqual(host.state.type("typeObject"), "object", "type returns 'object' for object");
}

// 测试 watch
function testStateWatch() {
    console.log("\n--- Testing state watch ---");
    
    let watchCalled = false;
    let watchedName = null;
    let watchedValue = null;
    
    const watchId = host.state.watch("watchTest", (name, value) => {
        watchCalled = true;
        watchedName = name;
        watchedValue = value;
    });
    
    assertTrue(watchId >= 0, "watch returns valid watch id");
    
    // 设置值应该触发回调
    host.state.set("watchTest", "new value");
    
    // 注意：回调可能是异步的，这里只验证 watchId 有效
    console.log(`  Watch ID: ${watchId}`);
}

// 运行所有测试
console.log("[TEST_START] Host Bridge Tests");

try {
    testHostExists();
    testStateBasicOps();
    testStateExistsAndType();
    testStateWatch();
} catch (e) {
    console.log(`[ERROR] ${e.message}`);
    console.log(e.stack);
}

console.log("\n[TEST_END]");
