/**
 * Preact Hooks - Simplified version for MBink
 * Based on Preact Hooks API
 *
 * Implements: useState, useEffect, useRef, useMemo, useCallback, useContext
 *
 * 使用 IIFE 封装避免全局变量污染，只暴露 preactHooks/PreactHooks 对象
 */

(function (global) {
    'use strict';

    // Global state for hooks
    var currentComponent = null;
    var currentHookIndex = 0;
    // Track components that ever used hooks so shutdown can run all effect cleanups
    var mountedComponents = new Set();
    var pendingEffects = [];
    var effectsScheduled = false;

    var hookDebug = global.__mbinkHookDebug || (global.__mbinkHookDebug = {
        nextComponentId: 1,
        nextHookId: 1,
        cleanupCount: 0
    });

    function ensureComponentId(component) {
        if (!component) return null;
        if (!component.__debugId) {
            component.__debugId = 'c' + (hookDebug.nextComponentId++);
        }
        return component.__debugId;
    }

    function ensureHookId(hookState) {
        if (!hookState) return null;
        if (!hookState.__debugId) {
            hookState.__debugId = 'h' + (hookDebug.nextHookId++);
        }
        return hookState.__debugId;
    }

    var pendingUpdates = new Set();
    var updateScheduled = false;

function cleanupComponent(component) {
    if (!component) {
        return;
    }

    var componentId = ensureComponentId(component);

    if (currentComponent === component) {
        currentComponent = null;
        currentHookIndex = 0;
    }

    mountedComponents.delete(component);
    pendingUpdates.delete(component);

    var hooks = component.__hooks;
    if (hooks && hooks.length) {
        if (pendingEffects.length) {
            var nextPendingEffects = [];
            for (var j = 0; j < pendingEffects.length; j++) {
                var pendingHook = pendingEffects[j];
                if (!pendingHook) {
                    continue;
                }
                if (hooks.indexOf(pendingHook) !== -1) {
                    pendingHook.pendingEffect = null;
                    pendingHook.effectQueued = false;
                    continue;
                }
                nextPendingEffects.push(pendingHook);
            }
            pendingEffects = nextPendingEffects;
        }

        for (var i = 0; i < hooks.length; i++) {
            var hookState = hooks[i];
            if (!hookState) {
                continue;
            }

            ensureHookId(hookState);

            hookState.pendingEffect = null;
            hookState.effectQueued = false;

            if (typeof hookState.cleanup === 'function') {
                try { hookState.cleanup(); } catch (_) {}
                hookState.cleanup = null;
            }

            hookState.deps = null;
            hookState.value = null;
            hookState.reducer = null;
            hookState.dispatch = null;
            if (hookState.ref && typeof hookState.ref === 'object') {
                hookState.ref.current = null;
            }
            hookState.ref = null;
        }
        hooks.length = 0;
    }

    hookDebug.cleanupCount++;
}

function flushPendingEffects() {
    effectsScheduled = false;
    var toRun = pendingEffects.slice();
    pendingEffects = [];

    for (var i = 0; i < toRun.length; i++) {
        var hookState = toRun[i];
        if (!hookState) {
            continue;
        }

        hookState.effectQueued = false;

        if (typeof hookState.cleanup === 'function') {
            try { hookState.cleanup(); } catch (_) {}
        }
        if (typeof hookState.pendingEffect === 'function') {
            hookState.cleanup = hookState.pendingEffect();
            hookState.pendingEffect = null;
        }
    }

}

function schedulePendingEffects() {
    if (effectsScheduled) {
        return;
    }
    effectsScheduled = true;

    if (typeof setTimeout !== 'undefined') {
        setTimeout(flushPendingEffects, 0);
    } else {
        flushPendingEffects();
    }
}

// 获取 requestAnimationFrame，支持回退
function getRAF() {
    if (typeof requestAnimationFrame === 'function') {
        return requestAnimationFrame;
    }
    // 回退到 setTimeout
    return function (callback) {
        return setTimeout(callback, 16);
    };
}

function scheduleUpdate(component) {
    if (!component || pendingUpdates.has(component)) {
        return;
    }

    pendingUpdates.add(component);

    if (!updateScheduled) {
        updateScheduled = true;
        // 使用 requestAnimationFrame 批量更新（带回退）
        var raf = getRAF();
        raf(flushUpdates);
    }
}

function flushUpdates() {
    // 通知 C++ 开始批量操作
    if (typeof document !== 'undefined' && typeof document.__beginBatch === 'function') {
        document.__beginBatch();
    }

    // 执行所有待更新组件
    var updates = Array.from(pendingUpdates);
    pendingUpdates.clear();
    updateScheduled = false;

    for (var i = 0; i < updates.length; i++) {
        var component = updates[i];
        if (component && component.__rerender) {
            try {
                component.__rerender();
            } catch (e) {
                console.error('[flushUpdates] Component rerender error:', e.message);
            }
        }
    }

    // 通知 C++ 结束批量操作
    if (typeof document !== 'undefined' && typeof document.__endBatch === 'function') {
        document.__endBatch();
    }
}

/**
 * Set the current component context (called by renderer)
 * @internal
 */
function setCurrentComponent(component) {
    currentComponent = component;
    currentHookIndex = 0;
    if (component) {
        ensureComponentId(component);
        mountedComponents.add(component);
    }
}

/**
 * Get the current hook state
 * @internal
 */
function getHookState(index) {
    if (!currentComponent) {
        throw new Error('Hooks can only be called inside function components');
    }

    if (!currentComponent.__hooks) {
        currentComponent.__hooks = [];
    }

    if (!currentComponent.__hooks[index]) {
        currentComponent.__hooks[index] = {};
    }

    return currentComponent.__hooks[index];
}

    /**
     * useState Hook - Manage component state
     * @param {any} initialValue - Initial state value
     * @returns {[any, Function]} [state, setState]
     */
    function useState(initialValue) {
        var hookState = getHookState(currentHookIndex++);

        if (!('value' in hookState)) {
            hookState.value = typeof initialValue === 'function' ? initialValue() : initialValue;
        }

        if (!hookState.setState) {
            hookState.component = currentComponent;
            hookState.setState = function(newValue) {
                var nextValue = typeof newValue === 'function'
                    ? newValue(hookState.value)
                    : newValue;

                if (hookState.value !== nextValue) {
                    hookState.value = nextValue;
                    if (hookState.component) {
                        scheduleUpdate(hookState.component);
                    }
                }
            };
            hookState.result = [hookState.value, hookState.setState];
        }

        hookState.component = currentComponent;
        hookState.result[0] = hookState.value;
        return hookState.result;
    }

    /**
     * useEffect Hook - Side effects
     * @param {Function} effect - Effect function
     * @param {Array} deps - Dependency array
     */
    function useEffect(effect, deps) {
        var hookState = getHookState(currentHookIndex++);

        var hasChanged = !hookState.deps || !deps;
        if (!hasChanged && deps) {
            for (var i = 0; i < deps.length; i++) {
                if (deps[i] !== hookState.deps[i]) {
                    hasChanged = true;
                    break;
                }
            }
        }

        if (hasChanged) {
            hookState.deps = deps;
            hookState.pendingEffect = effect;

            if (!hookState.effectQueued) {
                hookState.effectQueued = true;
                pendingEffects.push(hookState);
            }

            schedulePendingEffects();
        }
    }

/**
 * useLayoutEffect Hook - Synchronous effects
 * @param {Function} effect - Effect function
 * @param {Array} deps - Dependency array
 */
function useLayoutEffect(effect, deps) {
    var hookState = getHookState(currentHookIndex++);

    var hasChanged = !hookState.deps || !deps;
    if (!hasChanged && deps) {
        for (var i = 0; i < deps.length; i++) {
            if (deps[i] !== hookState.deps[i]) {
                hasChanged = true;
                break;
            }
        }
    }

    if (hasChanged) {
        hookState.deps = deps;

        // Run immediately (synchronous)
        if (hookState.cleanup) {
            hookState.cleanup();
        }
        hookState.cleanup = effect();
    }
}

/**
 * useRef Hook - Mutable ref object
 * @param {any} initialValue - Initial value
 * @returns {object} Ref object with .current property
 */
function useRef(initialValue) {
    var hookState = getHookState(currentHookIndex++);

    if (!('ref' in hookState)) {
        hookState.ref = { current: initialValue };
    }

    return hookState.ref;
}

/**
 * useMemo Hook - Memoized value
 * @param {Function} factory - Factory function
 * @param {Array} deps - Dependency array
 * @returns {any} Memoized value
 */
function useMemo(factory, deps) {
    var hookState = getHookState(currentHookIndex++);

    var hasChanged = !hookState.deps || !deps;
    if (!hasChanged && deps) {
        for (var i = 0; i < deps.length; i++) {
            if (deps[i] !== hookState.deps[i]) {
                hasChanged = true;
                break;
            }
        }
    }

    if (hasChanged) {
        hookState.deps = deps;
        hookState.value = factory();
    }

    return hookState.value;
}

/**
 * useCallback Hook - Memoized callback
 * @param {Function} callback - Callback function
 * @param {Array} deps - Dependency array
 * @returns {Function} Memoized callback
 */
function useCallback(callback, deps) {
    return useMemo(function() { return callback; }, deps);
}

/**
 * useContext Hook - Access context value
 * @param {object} context - Context object
 * @returns {any} Context value
 */
function useContext(context) {
    if (!currentComponent) {
        throw new Error('useContext can only be called inside function components');
    }

    // Simple context implementation
    return context._currentValue || context.__currentValue;
}

/**
 * useReducer Hook - State management with reducer
 * @param {Function} reducer - Reducer function
 * @param {any} initialState - Initial state
 * @param {Function} init - Lazy initializer
 * @returns {[any, Function]} [state, dispatch]
 */
function useReducer(reducer, initialState, init) {
    var hookState = getHookState(currentHookIndex++);

    if (!('value' in hookState)) {
        hookState.value = init ? init(initialState) : initialState;
    }

    if (!hookState.dispatch) {
        hookState.component = currentComponent;
        hookState.dispatch = function(action) {
            var nextState = hookState.reducer(hookState.value, action);

            if (hookState.value !== nextState) {
                hookState.value = nextState;
                if (hookState.component) {
                    scheduleUpdate(hookState.component);
                }
            }
        };
        hookState.result = [hookState.value, hookState.dispatch];
    }

    hookState.reducer = reducer;
    hookState.component = currentComponent;
    hookState.result[0] = hookState.value;
    return hookState.result;
}

/**
 * useImperativeHandle Hook - Customize ref exposure
 * @param {object|Function} ref - Ref object or callback ref
 * @param {Function} createHandle - Function that returns the handle
 * @param {Array} deps - Dependency array
 */
function useImperativeHandle(ref, createHandle, deps) {
    useLayoutEffect(function() {
        if (typeof ref === 'function') {
            var result = ref(createHandle());
            return function() {
                ref(null);
                if (result && typeof result === 'function') {
                    result();
                }
            };
        } else if (ref) {
            ref.current = createHandle();
            return function() {
                ref.current = null;
            };
        }
    }, deps == null ? deps : deps.concat(ref));
}

/**
 * useDebugValue Hook - Display custom label in devtools
 * @param {any} value - Value to display
 * @param {Function} formatter - Optional formatter function
 */
function useDebugValue(value, formatter) {
    // Access Preact options if available
    var opts = typeof Preact !== 'undefined' ? Preact.options : null;
    if (opts && opts.useDebugValue) {
        opts.useDebugValue(formatter ? formatter(value) : value);
    }
}

/**
 * useErrorBoundary Hook - Error boundary hook
 * @param {Function} cb - Error callback
 * @returns {[any, Function]} [error, resetError]
 */
function useErrorBoundary(cb) {
    var hookState = getHookState(currentHookIndex++);
    var errState = useState();

    hookState.value = cb;

    if (!currentComponent.componentDidCatch) {
        currentComponent.componentDidCatch = function(err, errorInfo) {
            if (hookState.value) {
                hookState.value(err, errorInfo);
            }
            errState[1](err);
        };
    }

    return [
        errState[0],
        function() {
            errState[1](undefined);
        }
    ];
}

/**
 * useId Hook - Generate unique IDs for accessibility
 * @returns {string} Unique ID
 */
function useId() {
    var hookState = getHookState(currentHookIndex++);

    if (!hookState.value) {
        // Generate unique ID based on component and hook index
        var componentId = ensureComponentId(currentComponent);
        var hookId = ensureHookId(hookState);
        hookState.value = 'id-' + componentId + '-' + hookId;
    }

    return hookState.value;
}

/**
 * Create a context object
 * @param {any} defaultValue - Default context value
 * @returns {object} Context object
 */
function createContext(defaultValue) {
    var context = {
        _currentValue: defaultValue,
        Provider: function Provider(props) {
            context._currentValue = props.value;
            return props.children;
        },
        Consumer: function Consumer(props) {
            return props.children(context._currentValue);
        }
    };

    return context;
}

    // Export all hooks as global object (QuickJS compatible)
    var PreactHooks = {
        useState: useState,
        useEffect: useEffect,
        useLayoutEffect: useLayoutEffect,
        useRef: useRef,
        useMemo: useMemo,
        useCallback: useCallback,
        useContext: useContext,
        useReducer: useReducer,
        useImperativeHandle: useImperativeHandle,
        useDebugValue: useDebugValue,
        useErrorBoundary: useErrorBoundary,
        useId: useId,
        createContext: createContext,
        setCurrentComponent: setCurrentComponent,
        cleanupComponent: cleanupComponent,
        // Phase 4: 调度器 API
        scheduleUpdate: scheduleUpdate,
        flushUpdates: flushUpdates
    };

    // 添加小写别名以提高兼容性
    var preactHooks = PreactHooks;

    // 暴露到全局作用域
    global.PreactHooks = PreactHooks;
    global.preactHooks = preactHooks;

    // 暴露清理函数，供 C++ 关闭时调用以释放 IIFE 内部的闭包引用
    global.__preactHooksCleanup = function() {
        flushPendingEffects();

        // 1) 先执行所有组件 hooks 的 cleanup，解除 document/window 级监听器等副作用
        mountedComponents.forEach(function(component) {
            if (!component || !component.__hooks) return;
            for (var i = 0; i < component.__hooks.length; i++) {
                var hookState = component.__hooks[i];
                if (!hookState) continue;
                hookState.pendingEffect = null;
                hookState.effectQueued = false;
                if (typeof hookState.cleanup === 'function') {
                    try { hookState.cleanup(); } catch (_) {}
                    hookState.cleanup = null;
                }
            }
            component.__hooks = [];
        });
        mountedComponents.clear();

        // 2) 清空调度器状态，避免残留闭包引用
        pendingEffects = [];
        effectsScheduled = false;
        pendingUpdates.clear();
        updateScheduled = false;
        currentComponent = null;
    };

})(typeof globalThis !== 'undefined' ? globalThis : typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : this);
