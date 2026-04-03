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
        currentTrackingRoot: null,
        renderCount: 0,
        flushCount: 0,
        scheduleCount: 0,
        trackedDependencyCount: 0,
        lastScheduledKeys: '',
        lastFlushedKeys: '',
        lastFlushMatchedRoots: 0,
        lastFlushSkippedRoots: 0,
        lastFlushInvalidRoots: 0,
        proxyCache: {},
        proxyTargets: {}
    });

    runtime.trackDependency = function(depId) {
        var root = runtime.currentTrackingRoot;
        if (!root || !depId) return;
        if (!root.deps) root.deps = {};
        if (!root.deps[depId]) {
            root.deps[depId] = true;
            runtime.trackedDependencyCount++;
        }
    };

    runtime.beginTracking = function(root) {
        if (!root) return;
        root.deps = {};
        runtime.currentTrackingRoot = root;
    };

    runtime.endTracking = function(root) {
        if (runtime.currentTrackingRoot === root) {
            runtime.currentTrackingRoot = null;
        }
    };

    runtime.shouldFlushRoot = function(root, changedKeys) {
        if (!root || !changedKeys || !changedKeys.length) return false;
        if (!root.deps) return true;
        for (var i = 0; i < changedKeys.length; i++) {
            if (root.deps[changedKeys[i]]) return true;
        }
        return false;
    };

    runtime.wrapSharedObject = function(name, target) {
        if (!name || !target || typeof Proxy !== 'function') return target;
        var cached = runtime.proxyCache[name];
        if (cached && runtime.proxyTargets[name] === target) {
            return cached;
        }

        var proxy = new Proxy(target, {
            get: function(obj, prop, receiver) {
                if (typeof prop === 'string') {
                    runtime.trackDependency(name + ':' + prop);
                }
                return Reflect.get(obj, prop, receiver);
            },
            set: function(obj, prop, value, receiver) {
                return Reflect.set(obj, prop, value, receiver);
            },
            deleteProperty: function(obj, prop) {
                return Reflect.deleteProperty(obj, prop);
            },
            ownKeys: function(obj) {
                var keys = Reflect.ownKeys(obj);
                for (var i = 0; i < keys.length; i++) {
                    if (typeof keys[i] === 'string') {
                        runtime.trackDependency(name + ':' + keys[i]);
                    }
                }
                return keys;
            },
            getOwnPropertyDescriptor: function(obj, prop) {
                return Object.getOwnPropertyDescriptor(obj, prop);
            }
        });

        runtime.proxyTargets[name] = target;
        runtime.proxyCache[name] = proxy;
        return proxy;
    };

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
            found = { container: container, vnode: vnode, renderImpl: renderImpl, deps: null };
            roots.push(found);
        }
        found.vnode = vnode;
        found.renderImpl = renderImpl;
        try { container.__preactRoot = found; } catch (_) {}
    };

    global.__mbinkRegisterPreactRoot = runtime.registerRoot;

    runtime.cleanup = function() {
        runtime.pending = false;
        runtime.currentTrackingRoot = null;
        runtime.renderCount = 0;
        runtime.flushCount = 0;
        runtime.scheduleCount = 0;
        runtime.trackedDependencyCount = 0;
        runtime.proxyCache = {};
        runtime.proxyTargets = {};
        for (var i = 0; i < runtime.roots.length; i++) {
            var item = runtime.roots[i];
            if (!item) continue;
            if (item.container) {
                try { item.container.__preactRoot = null; } catch (_) {}
            }
            item.vnode = null;
            item.renderImpl = null;
            item.container = null;
            item.deps = null;
        }
        runtime.roots = [];
        runtime.currentDispatcher = null;
    };

    runtime.flush = function(changedKeys) {
        if (!runtime.currentDispatcher) return;
        runtime.pending = false;
        runtime.flushCount++;
        runtime.lastFlushedKeys = changedKeys && changedKeys.length ? changedKeys.join(',') : '';

        var matchedRoots = 0;
        var skippedRoots = 0;
        var invalidRoots = 0;

        var roots = runtime.roots.slice();
        for (var i = 0; i < roots.length; i++) {
            var item = roots[i];
            if (!item || !item.container || !item.vnode || typeof item.renderImpl !== 'function') {
                invalidRoots++;
                continue;
            }
            if (changedKeys && changedKeys.length && !runtime.shouldFlushRoot(item, changedKeys)) {
                skippedRoots++;
                continue;
            }
            matchedRoots++;
            runtime.beginTracking(item);
            try {
                runtime.renderCount++;
                item.renderImpl(item.vnode, item.container);
            } finally {
                runtime.endTracking(item);
            }
        }

        runtime.lastFlushMatchedRoots = matchedRoots;
        runtime.lastFlushSkippedRoots = skippedRoots;
        runtime.lastFlushInvalidRoots = invalidRoots;
    };

    runtime.schedule = function(changedKeys) {
        if (!runtime.currentDispatcher) return;
        runtime.scheduleCount++;
        runtime.lastScheduledKeys = changedKeys && changedKeys.length ? changedKeys.join(',') : '';

        if (!runtime.pendingKeys) runtime.pendingKeys = {};
        if (changedKeys && changedKeys.length) {
            for (var i = 0; i < changedKeys.length; i++) {
                runtime.pendingKeys[changedKeys[i]] = true;
            }
        }

        if (runtime.pending) return;

        runtime.pending = true;
        var defer = typeof global.setTimeout === 'function'
            ? global.setTimeout
            : function(fn) { fn(); return 0; };
        defer(function() {
            var merged = [];
            var map = runtime.pendingKeys || {};
            runtime.pendingKeys = {};
            for (var key in map) {
                if (Object.prototype.hasOwnProperty.call(map, key)) {
                    merged.push(key);
                }
            }
            runtime.flush(merged);
        }, 0);
    };

    runtime.currentDispatcher = function(changedKeys) {
        runtime.schedule(changedKeys);
    };

    global.__mbinkSharedUpdateDispatcher = runtime.currentDispatcher;
    global.__mbinkWrapSharedObject = runtime.wrapSharedObject;
    global.__mbinkBeginRootTracking = runtime.beginTracking;
    global.__mbinkEndRootTracking = runtime.endTracking;

    global.__mbinkRuntimeCleanup = function() {
        try {
            runtime.cleanup();
        } catch (_) {}
    };

    global.__mbinkShutdown = function() {
        var safe = function(fn) {
            if (typeof fn === 'function') {
                try { fn(); } catch (_) {}
            }
        };

        safe(globalThis.__fetchCleanup);
        safe(globalThis.__preactHooksCleanup);
        safe(globalThis.__preactCleanup);
        safe(globalThis.__mbinkRuntimeCleanup);

        var keys = ['__fetchCleanup', '__preactHooksCleanup', '__preactCleanup',
                    '__mbinkRuntimeCleanup', '__mbinkShutdown',
                    'Preact', 'PreactHooks', 'preact', 'preactHooks',
                    '__mbinkRegisterPreactRoot', '__preactSetCurrentComponent',
                    '__mbinkSharedUpdateDispatcher', '__mbinkWrapSharedObject',
                    '__mbinkBeginRootTracking', '__mbinkEndRootTracking', '__mbinkSharedRuntime',
                    'data', 'backend', 'py'];
        for (var i = 0; i < keys.length; i++) {
            try { delete globalThis[keys[i]]; } catch (_) {
                try { globalThis[keys[i]] = undefined; } catch (_2) {}
            }
        }
    };

    console.log('MBink JavaScript runtime initialized');
})(this);

