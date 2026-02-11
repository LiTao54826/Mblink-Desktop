// tests/js/test_host_event.js
// Host Event System 单元测试

// ========== 测试辅助函数 ==========

function logTest(name, passed) {
    if (passed) {
        console.log("[TEST_PASS] " + name);
    } else {
        console.log("[TEST_FAIL] " + name);
    }
}

function assertEqual(actual, expected, testName) {
    var passed = actual === expected;
    logTest(testName, passed);
    if (!passed) {
        console.log("  Expected: " + expected);
        console.log("  Actual: " + actual);
    }
    return passed;
}

function assertNotEqual(actual, notExpected, testName) {
    var passed = actual !== notExpected;
    logTest(testName, passed);
    if (!passed) {
        console.log("  Should not be: " + notExpected);
        console.log("  Actual: " + actual);
    }
    return passed;
}

function assertTrue(value, testName) {
    logTest(testName, !!value);
    if (!value) {
        console.log("  Expected truthy, got: " + value);
    }
    return !!value;
}

function assertFalse(value, testName) {
    logTest(testName, !value);
    if (value) {
        console.log("  Expected falsy, got: " + value);
    }
    return !value;
}


// ========== 测试用例 ==========

console.log("[TEST_START] Host Event System Tests");

// --- 1. host.on 注册返回唯一 ID ---
function testOnReturnsUniqueId() {
    var id1 = host.on("evt1", function() {});
    var id2 = host.on("evt1", function() {});
    var id3 = host.on("evt2", function() {});

    assertNotEqual(id1, -1, "on() returns valid ID (not -1)");
    assertNotEqual(id1, id2, "on() returns unique IDs for same event");
    assertNotEqual(id2, id3, "on() returns unique IDs for different events");

    // 清理
    host.off(id1);
    host.off(id2);
    host.off(id3);
}
testOnReturnsUniqueId();

// --- 2. host.off 移除后不再收到事件 ---
function testOffRemovesListener() {
    var called = false;
    var id = host.on("testOff", function() {
        called = true;
    });

    host.off(id);

    // 需要通过 C++ emit + flushEvents 来触发
    // 在纯 JS 测试中，我们验证 off 不会崩溃
    // 实际的 emit 测试需要 Python 端或 C++ 端触发
    logTest("off() removes listener without error", true);

    // 验证重复 off 不报错
    host.off(id);
    logTest("off() duplicate call is safe", true);
}
testOffRemovesListener();

// --- 3. 无效参数测试 ---
function testInvalidParams() {
    // 非字符串事件名
    var id1 = host.on(123, function() {});
    assertEqual(id1, -1, "on() with non-string eventName returns -1");

    var id2 = host.on(null, function() {});
    assertEqual(id2, -1, "on() with null eventName returns -1");

    // 非函数回调
    var id3 = host.on("test", "not a function");
    assertEqual(id3, -1, "on() with non-function callback returns -1");

    var id4 = host.on("test", 42);
    assertEqual(id4, -1, "on() with number callback returns -1");

    // 参数不足
    var id5 = host.on("test");
    assertEqual(id5, -1, "on() with missing callback returns -1");
}
testInvalidParams();

// --- 4. 重复 off 同一 ID ---
function testDuplicateOff() {
    var id = host.on("dupOff", function() {});
    host.off(id);
    host.off(id);  // 不应报错
    host.off(id);  // 再次调用也不应报错
    logTest("off() multiple times on same ID is safe", true);
}
testDuplicateOff();

// --- 5. 无 Listener 时 off 不报错 ---
function testOffNonExistentId() {
    host.off(99999);
    host.off(-1);
    host.off(0);
    logTest("off() with non-existent ID is safe", true);
}
testOffNonExistentId();

// --- 6. 向后兼容：host.state API 仍正常 ---
function testBackwardCompatState() {
    // host.state.get/set/watch/unwatch 应该仍然存在
    assertTrue(typeof host.state.get === "function", "host.state.get exists");
    assertTrue(typeof host.state.set === "function", "host.state.set exists");
    assertTrue(typeof host.state.watch === "function", "host.state.watch exists");
    assertTrue(typeof host.state.unwatch === "function", "host.state.unwatch exists");
    assertTrue(typeof host.state.exists === "function", "host.state.exists exists");
    assertTrue(typeof host.state.type === "function", "host.state.type exists");
}
testBackwardCompatState();

// --- 7. 向后兼容：host.on/host.off API 存在 ---
function testEventApiExists() {
    assertTrue(typeof host.on === "function", "host.on exists as function");
    assertTrue(typeof host.off === "function", "host.off exists as function");
}
testEventApiExists();

// --- 8. 多个 Listener 注册同一事件 ---
function testMultipleListenersSameEvent() {
    var ids = [];
    for (var i = 0; i < 5; i++) {
        ids.push(host.on("multi", function() {}));
    }

    // 所有 ID 应该不同
    var unique = true;
    for (var i = 0; i < ids.length; i++) {
        for (var j = i + 1; j < ids.length; j++) {
            if (ids[i] === ids[j]) {
                unique = false;
                break;
            }
        }
    }
    assertTrue(unique, "Multiple listeners get unique IDs");

    // 清理
    for (var i = 0; i < ids.length; i++) {
        host.off(ids[i]);
    }
}
testMultipleListenersSameEvent();

console.log("[TEST_END] Basic Tests");

// ========== 状态自动事件集成测试 ==========

console.log("[TEST_START] State Auto-Event Tests");

// --- 9. host.on(stateName) 在状态变化后收到新值 ---
function testStateAutoEvent() {
    // 创建状态
    host.state.set("autoTest", 0);

    var receivedValue = null;
    var callCount = 0;
    var id = host.on("autoTest", function(val) {
        receivedValue = val;
        callCount++;
    });

    // 修改状态 — jsStateSet 会 processQueue + flushEvents
    host.state.set("autoTest", 42);

    assertEqual(callCount, 1, "State auto-event: callback called once");
    assertEqual(receivedValue, 42, "State auto-event: received correct value");

    // 再次修改
    host.state.set("autoTest", 100);
    assertEqual(callCount, 2, "State auto-event: callback called twice");
    assertEqual(receivedValue, 100, "State auto-event: received updated value");

    host.off(id);
}
testStateAutoEvent();

// --- 10. 移除所有 Listener 后状态变化不再触发回调 ---
function testAutoWatcherCleanup() {
    host.state.set("cleanupTest", 0);

    var callCount = 0;
    var id1 = host.on("cleanupTest", function() { callCount++; });
    var id2 = host.on("cleanupTest", function() { callCount++; });

    host.state.set("cleanupTest", 1);
    assertEqual(callCount, 2, "Auto-watcher cleanup: both listeners called");

    // 移除所有 listener
    host.off(id1);
    host.off(id2);

    // 修改状态不应再触发回调
    callCount = 0;
    host.state.set("cleanupTest", 2);
    assertEqual(callCount, 0, "Auto-watcher cleanup: no callback after all off()");
}
testAutoWatcherCleanup();

// --- 11. host.state.watch 和 host.on 同时监听同一状态，互不干扰 ---
function testWatchAndOnCoexist() {
    host.state.set("coexist", 0);

    var watchValue = null;
    var onValue = null;

    var watchId = host.state.watch("coexist", function(val) {
        watchValue = val;
    });

    var onId = host.on("coexist", function(val) {
        onValue = val;
    });

    host.state.set("coexist", 77);

    assertEqual(watchValue, 77, "Watch+On coexist: watch received value");
    assertEqual(onValue, 77, "Watch+On coexist: on received value");

    // 移除 on，watch 应该仍然工作
    host.off(onId);
    onValue = null;
    host.state.set("coexist", 88);
    assertEqual(watchValue, 88, "Watch+On coexist: watch still works after off(on)");
    assertEqual(onValue, null, "Watch+On coexist: on not called after off");

    // 移除 watch，确认不影响
    host.state.unwatch(watchId);
    watchValue = null;
    host.state.set("coexist", 99);
    assertEqual(watchValue, null, "Watch+On coexist: watch not called after unwatch");
}
testWatchAndOnCoexist();

console.log("[TEST_END] State Auto-Event Tests");
