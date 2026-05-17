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

    if (typeof global.globalThis === 'undefined') {
        global.globalThis = global;
    }
    if (typeof global.window === 'undefined') {
        global.window = global.globalThis;
    }
    if (typeof global.self === 'undefined') {
        global.self = global.window;
    }

    var createMap = Object.create
        ? function() { return Object.create(null); }
        : function() { return {}; };

    var runtime = global.__mbinkSharedRuntime || (global.__mbinkSharedRuntime = {
        roots: [],
        pending: false,
        currentDispatcher: null,
        currentTrackingRoot: null,
        proxyCache: createMap(),
        proxyTargets: createMap(),
        depKeyCache: createMap(),
        pendingKeys: createMap(),
        deepProxyCache: typeof WeakMap === 'function' ? new WeakMap() : null
    });

    runtime.proxyCache = runtime.proxyCache || createMap();
    runtime.proxyTargets = runtime.proxyTargets || createMap();
    runtime.depKeyCache = runtime.depKeyCache || createMap();
    runtime.pendingKeys = runtime.pendingKeys || createMap();
    runtime.deepProxyCache = runtime.deepProxyCache || (typeof WeakMap === 'function' ? new WeakMap() : null);

    runtime.getDependencyKey = runtime.getDependencyKey || function(name, prop) {
        var cache = runtime.depKeyCache[name];
        if (!cache) {
            cache = runtime.depKeyCache[name] = createMap();
        }
        var key = cache[prop];
        if (!key) {
            key = name + ':' + prop;
            cache[prop] = key;
        }
        return key;
    };

    runtime.trackDependency = runtime.trackDependency || function(key) {
        var root = runtime.currentTrackingRoot;
        if (!root || !key) return;
        if (!root.deps) {
            root.deps = createMap();
        }
        root.deps[key] = true;
    };

    runtime.trackObjectDependency = runtime.trackObjectDependency || function(name) {
        var root = runtime.currentTrackingRoot;
        if (!root || !name) return;
        if (!root.objectDeps) {
            root.objectDeps = createMap();
        }
        root.objectDeps[name] = true;
    };

    runtime.beginTracking = runtime.beginTracking || function(root) {
        runtime.currentTrackingRoot = root || null;
        if (!root) return;
        root.deps = createMap();
        root.objectDeps = createMap();
    };

    runtime.endTracking = runtime.endTracking || function(root) {
        if (runtime.currentTrackingRoot === root || !root) {
            runtime.currentTrackingRoot = null;
        }
    };

    runtime.shouldFlushRoot = runtime.shouldFlushRoot || function(root, changedKeys) {
        if (!root || !changedKeys || !changedKeys.length) return true;
        var deps = root.deps;
        var objectDeps = root.objectDeps;
        if (!deps && !objectDeps) return true;
        for (var i = 0; i < changedKeys.length; i++) {
            var key = changedKeys[i];
            if (deps && deps[key]) {
                return true;
            }
            if (objectDeps) {
                var sep = key.indexOf(':');
                var objectName = sep >= 0 ? key.slice(0, sep) : key;
                if (objectDeps[objectName]) {
                    return true;
                }
            }
        }
        return false;
    };

    runtime.wrapSharedObject = function(name, target) {
        if (!name || !target || typeof Proxy !== 'function') return target;
        var cached = runtime.proxyCache[name];
        if (cached && runtime.proxyTargets[name] === target) {
            return cached;
        }

        var syncSet = function(prop, value) {
            var changed = true;
            if (typeof global.__mbinkSharedNativeSet === 'function') {
                try { changed = global.__mbinkSharedNativeSet(name, prop, value) !== false; } catch (_) {}
            }
            if (changed) {
                runtime.schedule([runtime.getDependencyKey(name, prop)]);
            }
            return changed;
        };

        var syncDelete = function(prop) {
            var changed = true;
            if (typeof global.__mbinkSharedNativeDelete === 'function') {
                try { changed = global.__mbinkSharedNativeDelete(name, prop) !== false; } catch (_) {}
            }
            if (changed) {
                runtime.schedule([runtime.getDependencyKey(name, prop)]);
            }
            return changed;
        };

        var wrapNested = function(rootKey, value, getRootValue) {
            if (!value || typeof value !== 'object' || typeof Proxy !== 'function') {
                return value;
            }

            var depKey = runtime.getDependencyKey(name, rootKey);

            var bucket = null;
            if (runtime.deepProxyCache && typeof runtime.deepProxyCache.get === 'function') {
                try {
                    bucket = runtime.deepProxyCache.get(value);
                    if (!bucket) {
                        bucket = createMap();
                        runtime.deepProxyCache.set(value, bucket);
                    }
                    if (bucket[depKey]) {
                        return bucket[depKey];
                    }
                } catch (_) {
                    bucket = null;
                }
            }

            var proxy = new Proxy(value, {
                get: function(obj, prop, receiver) {
                    if (typeof prop === 'string') {
                        runtime.trackDependency(depKey);
                    }
                    var v = Reflect.get(obj, prop, receiver);
                    if (typeof v === 'function') {
                        return v;
                    }
                    return wrapNested(rootKey, v, getRootValue);
                },
                set: function(obj, prop, v, receiver) {
                    var ok = Reflect.set(obj, prop, v, receiver);
                    if (ok) {
                        syncSet(rootKey, getRootValue());
                    }
                    return ok;
                },
                deleteProperty: function(obj, prop) {
                    var ok = Reflect.deleteProperty(obj, prop);
                    if (ok) {
                        syncSet(rootKey, getRootValue());
                    }
                    return ok;
                },
                ownKeys: function(obj) {
                    runtime.trackDependency(depKey);
                    return Reflect.ownKeys(obj);
                },
                getOwnPropertyDescriptor: function(obj, prop) {
                    if (typeof prop === 'string') {
                        runtime.trackDependency(depKey);
                    }
                    return Object.getOwnPropertyDescriptor(obj, prop);
                }
            });

            if (bucket) {
                bucket[depKey] = proxy;
            }
            return proxy;
        };

        var proxy = new Proxy(target, {
            get: function(obj, prop, receiver) {
                if (typeof prop === 'string') {
                    runtime.trackDependency(runtime.getDependencyKey(name, prop));
                }
                var v = Reflect.get(obj, prop, receiver);
                if (typeof prop === 'string') {
                    return wrapNested(prop, v, function() { return obj[prop]; });
                }
                return v;
            },
            set: function(obj, prop, value, receiver) {
                var ok = Reflect.set(obj, prop, value, receiver);
                if (ok && typeof prop === 'string') {
                    syncSet(prop, obj[prop]);
                }
                return ok;
            },
            deleteProperty: function(obj, prop) {
                var ok = Reflect.deleteProperty(obj, prop);
                if (ok && typeof prop === 'string') {
                    syncDelete(prop);
                }
                return ok;
            },
            ownKeys: function(obj) {
                runtime.trackObjectDependency(name);
                return Reflect.ownKeys(obj);
            },
            getOwnPropertyDescriptor: function(obj, prop) {
                if (typeof prop === 'string') {
                    runtime.trackDependency(runtime.getDependencyKey(name, prop));
                }
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
            found = { container: container, vnode: vnode, renderImpl: renderImpl, deps: null, objectDeps: null };
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
        runtime.proxyCache = createMap();
        runtime.proxyTargets = createMap();
        runtime.depKeyCache = createMap();
        runtime.pendingKeys = createMap();
        runtime.deepProxyCache = typeof WeakMap === 'function' ? new WeakMap() : null;
        for (var i = 0; i < runtime.roots.length; i++) {
            var item = runtime.roots[i];
            if (!item) continue;
            if (item.container && typeof item.renderImpl === 'function') {
                try { item.renderImpl(null, item.container); } catch (_) {}
            }
            if (item.container) {
                try { item.container.__preactRoot = null; } catch (_) {}
            }
            item.vnode = null;
            item.renderImpl = null;
            item.container = null;
            item.deps = null;
            item.objectDeps = null;
        }
        runtime.roots = [];
        runtime.currentDispatcher = null;
    };

    runtime.flush = function(changedKeys) {
        if (!runtime.currentDispatcher) return;
        runtime.pending = false;

        var roots = runtime.roots.slice();
        for (var i = 0; i < roots.length; i++) {
            var item = roots[i];
            if (!item || !item.container || !item.vnode || typeof item.renderImpl !== 'function') {
                continue;
            }
            if (changedKeys && changedKeys.length && !runtime.shouldFlushRoot(item, changedKeys)) {
                continue;
            }
            runtime.beginTracking(item);
            try {
                item.renderImpl(item.vnode, item.container);
            } finally {
                runtime.endTracking(item);
            }
        }
    };

    runtime.schedule = function(changedKeys) {
        if (!runtime.currentDispatcher) return;
        if (!runtime.pendingKeys) runtime.pendingKeys = createMap();
        if (changedKeys && changedKeys.length) {
            for (var i = 0; i < changedKeys.length; i++) {
                runtime.pendingKeys[changedKeys[i]] = true;
            }
        }

        if (runtime.pending) return;

        runtime.pending = true;

        var defer = typeof global.queueMicrotask === 'function'
            ? function(fn) { global.queueMicrotask(fn); return 0; }
            : (typeof global.setTimeout === 'function'
                ? global.setTimeout
                : function(fn) { fn(); return 0; });
        defer(function() {
            var merged = [];
            var map = runtime.pendingKeys || createMap();
            runtime.pendingKeys = createMap();
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
                    '__mbinkRuntimeCleanup', '__mbinkShutdown', '__mbinkDumpNativeLeakStats',
                    'Preact', 'PreactHooks', 'preact', 'preactHooks',
                    '__mbinkRegisterPreactRoot', '__preactSetCurrentComponent',
                    '__mbinkSharedUpdateDispatcher', '__mbinkWrapSharedObject',
                    '__mbinkBeginRootTracking', '__mbinkEndRootTracking', '__mbinkSharedRuntime',
                    '__mbinkSharedNativeSet', '__mbinkSharedNativeDelete',
                    'data', 'backend', 'py'];
        for (var i = 0; i < keys.length; i++) {
            try { delete globalThis[keys[i]]; } catch (_) {
                try { globalThis[keys[i]] = undefined; } catch (_2) {}
            }
        }
    };

    console.log('MBink JavaScript runtime initialized');
})(this);

