/**
 * @file bootstrap.js
 * @brief JavaScript运行时引导程序
 */

(function(global) {
    'use strict';

    global.console = global.console || {
        log: function(...args) { print('[LOG]', ...args); },
        error: function(...args) { print('[ERROR]', ...args); },
        warn: function(...args) { print('[WARN]', ...args); },
        info: function(...args) { print('[INFO]', ...args); },
        debug: function(...args) { print('[DEBUG]', ...args); }
    };

    if (typeof global.setTimeout !== 'function') {
        let timerIdCounter = 1;
        const timers = new Map();
        global.setTimeout = function(callback, delay, ...args) {
            const id = timerIdCounter++;
            timers.set(id, { callback, delay, args, type: 'timeout' });
            return id;
        };
        global.clearTimeout = function(id) { timers.delete(id); };
        global.setInterval = function(callback, delay, ...args) {
            const id = timerIdCounter++;
            timers.set(id, { callback, delay, args, type: 'interval' });
            return id;
        };
        global.clearInterval = function(id) { timers.delete(id); };
    }

    if (typeof global.requestAnimationFrame !== 'function') {
        global.requestAnimationFrame = function(callback) {
            return global.setTimeout(callback, 16);
        };
    }
    if (typeof global.cancelAnimationFrame !== 'function') {
        global.cancelAnimationFrame = function(id) {
            global.clearTimeout(id);
        };
    }

    global.window = global;
    global.self = global;
    global.globalThis = global;

    var runtime = global.__mbinkSharedRuntime || (global.__mbinkSharedRuntime = {
        roots: [],
        pending: false,
        currentDispatcher: null,
        userHook: null
    });

    runtime.registerRoot = function(vnode, container, renderImpl) {
        if (!container || typeof renderImpl !== 'function') return;
        var roots = runtime.roots;
        var found = null;
        for (var i = 0; i < roots.length; i++) {
            if (roots[i].container === container) {
                found = roots[i];
                break;
            }
        }
        if (!found) {
            found = { container: container, vnode: vnode, renderImpl: renderImpl };
            roots.push(found);
        }
        found.vnode = vnode;
        found.renderImpl = renderImpl;
    };

    runtime.cleanup = function() {
        runtime.pending = false;
        for (var i = 0; i < runtime.roots.length; i++) {
            var item = runtime.roots[i];
            if (!item) continue;
            if (item.container) {
                try { item.container.__preactRoot = null; } catch (_) {}
            }
            item.vnode = null;
            item.renderImpl = null;
            item.container = null;
        }
        runtime.roots = [];
        runtime.userHook = null;
        runtime.currentDispatcher = null;
    };

    runtime.flush = function() {
        if (!runtime.currentDispatcher) return;
        runtime.pending = false;
        var roots = runtime.roots.slice();
        for (var i = 0; i < roots.length; i++) {
            var item = roots[i];
            if (item && item.container && item.vnode && typeof item.renderImpl === 'function') {
                item.renderImpl(item.vnode, item.container);
            }
        }
        if (typeof runtime.userHook === 'function') {
            runtime.userHook();
        }
    };

    runtime.schedule = function() {
        if (!runtime.currentDispatcher || runtime.pending) return;
        runtime.pending = true;
        var defer = typeof global.setTimeout === 'function'
            ? global.setTimeout
            : function(fn) { fn(); return 0; };
        defer(function() { runtime.flush(); }, 0);
    };

    runtime.currentDispatcher = function() {
        runtime.schedule();
    };

    Object.defineProperty(global, '__onSharedUpdate', {
        configurable: true,
        enumerable: false,
        get: function() {
            return runtime.currentDispatcher;
        },
        set: function(fn) {
            runtime.userHook = typeof fn === 'function' ? fn : null;
        }
    });

    global.__mbinkRegisterPreactRoot = function(vnode, container, renderImpl) {
        runtime.registerRoot(vnode, container, renderImpl);
    };

    global.__mbinkRuntimeCleanup = function() {
        try {
            runtime.cleanup();
        } catch (_) {}
    };

    console.log('MBink JavaScript runtime initialized');
})(this);

