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

// Phase 4: Preact 调度器 - 批量更新支持
var pendingUpdates = new Set();
var updateScheduled = false;

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
                // Silently handle rerender errors
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

        // Capture the component reference when creating setState
        var component = currentComponent;

        var setState = function(newValue) {
            var nextValue = typeof newValue === 'function'
                ? newValue(hookState.value)
                : newValue;

            if (hookState.value !== nextValue) {
                hookState.value = nextValue;
                // Phase 4: 使用调度器批量更新，而非立即渲染
                if (component) {
                    scheduleUpdate(component);
                }
            }
        };

        return [hookState.value, setState];
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

            // Schedule effect to run after render
            if (typeof setTimeout !== 'undefined') {
                setTimeout(function() {
                    if (hookState.cleanup) {
                        hookState.cleanup();
                    }
                    hookState.cleanup = effect();
                }, 0);
            } else {
                // Fallback: run immediately
                if (hookState.cleanup) {
                    hookState.cleanup();
                }
                hookState.cleanup = effect();
            }
        }
    }

/**
 * useLayoutEffect Hook - Synchronous effects
 * @param {Function} effect - Effect function
 * @param {Array} deps - Dependency array
 */
function useLayoutEffect(effect, deps) {
    const hookState = getHookState(currentHookIndex++);

    const hasChanged = !hookState.deps ||
        !deps ||
        deps.some((dep, i) => dep !== hookState.deps[i]);

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
    const hookState = getHookState(currentHookIndex++);

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
    const hookState = getHookState(currentHookIndex++);

    const hasChanged = !hookState.deps ||
        !deps ||
        deps.some((dep, i) => dep !== hookState.deps[i]);

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
    return useMemo(() => callback, deps);
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
    return context._currentValue;
}

/**
 * useReducer Hook - State management with reducer
 * @param {Function} reducer - Reducer function
 * @param {any} initialState - Initial state
 * @param {Function} init - Lazy initializer
 * @returns {[any, Function]} [state, dispatch]
 */
function useReducer(reducer, initialState, init) {
    const hookState = getHookState(currentHookIndex++);

    if (!('value' in hookState)) {
        hookState.value = init ? init(initialState) : initialState;
    }

    // Capture the component reference when creating dispatch
    const component = currentComponent;

    const dispatch = (action) => {
        const nextState = reducer(hookState.value, action);

        if (hookState.value !== nextState) {
            hookState.value = nextState;
            // Trigger re-render using captured component reference
            if (component && component.__rerender) {
                component.__rerender();
            }
        }
    };

    return [hookState.value, dispatch];
}

/**
 * Create a context object
 * @param {any} defaultValue - Default context value
 * @returns {object} Context object
 */
function createContext(defaultValue) {
    const context = {
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
        createContext: createContext,
        setCurrentComponent: setCurrentComponent,
        // Phase 4: 调度器 API
        scheduleUpdate: scheduleUpdate,
        flushUpdates: flushUpdates
    };

    // 添加小写别名以提高兼容性
    var preactHooks = PreactHooks;

    // 暴露到全局作用域
    global.PreactHooks = PreactHooks;
    global.preactHooks = preactHooks;

})(typeof globalThis !== 'undefined' ? globalThis : typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : this);
