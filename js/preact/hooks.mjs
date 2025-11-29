/**
 * Preact Hooks - ES6 Module version for MBink
 * 
 * Usage: import { useState, useEffect } from 'preact/hooks';
 */

// Current component being rendered (set by render)
let currentComponent = null;
let currentHookIndex = 0;

/**
 * Set the current component (called by render)
 */
export function setCurrentComponent(component) {
    currentComponent = component;
    currentHookIndex = 0;
}

/**
 * Get current hook state
 */
function getHookState(index) {
    if (!currentComponent) {
        throw new Error('Hooks can only be called inside a component');
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
 * useState hook
 */
export function useState(initialValue) {
    const hookState = getHookState(currentHookIndex++);
    
    if (!hookState.initialized) {
        hookState.initialized = true;
        hookState.value = typeof initialValue === 'function' ? initialValue() : initialValue;
    }
    
    const component = currentComponent;
    
    const setState = (newValue) => {
        const nextValue = typeof newValue === 'function' 
            ? newValue(hookState.value) 
            : newValue;
        
        if (nextValue !== hookState.value) {
            hookState.value = nextValue;
            if (component && component.__rerender) {
                component.__rerender();
            }
        }
    };
    
    return [hookState.value, setState];
}

/**
 * useEffect hook (async)
 */
export function useEffect(effect, deps) {
    const hookState = getHookState(currentHookIndex++);
    const hasChanged = !hookState.deps || !deps || deps.some((dep, i) => dep !== hookState.deps[i]);
    
    if (hasChanged) {
        hookState.deps = deps;
        if (typeof setTimeout !== 'undefined') {
            setTimeout(() => {
                if (hookState.cleanup) hookState.cleanup();
                hookState.cleanup = effect();
            }, 0);
        } else {
            if (hookState.cleanup) hookState.cleanup();
            hookState.cleanup = effect();
        }
    }
}

/**
 * useLayoutEffect hook (sync)
 */
export function useLayoutEffect(effect, deps) {
    const hookState = getHookState(currentHookIndex++);
    const hasChanged = !hookState.deps || !deps || deps.some((dep, i) => dep !== hookState.deps[i]);
    
    if (hasChanged) {
        hookState.deps = deps;
        if (hookState.cleanup) hookState.cleanup();
        hookState.cleanup = effect();
    }
}

/**
 * useRef hook
 */
export function useRef(initialValue) {
    const hookState = getHookState(currentHookIndex++);
    
    if (!hookState.initialized) {
        hookState.initialized = true;
        hookState.ref = { current: initialValue };
    }
    
    return hookState.ref;
}

/**
 * useMemo hook
 */
export function useMemo(factory, deps) {
    const hookState = getHookState(currentHookIndex++);
    const hasChanged = !hookState.deps || !deps || deps.some((dep, i) => dep !== hookState.deps[i]);
    
    if (hasChanged) {
        hookState.deps = deps;
        hookState.value = factory();
    }
    
    return hookState.value;
}

/**
 * useCallback hook
 */
export function useCallback(callback, deps) {
    return useMemo(() => callback, deps);
}

/**
 * Context implementation
 */
export function createContext(defaultValue) {
    const context = {
        _defaultValue: defaultValue,
        _value: defaultValue,
        Provider: function({ value, children }) {
            context._value = value;
            return children;
        },
        Consumer: function({ children }) {
            return children(context._value);
        }
    };
    return context;
}

/**
 * useContext hook
 */
export function useContext(context) {
    return context._value !== undefined ? context._value : context._defaultValue;
}

/**
 * useReducer hook
 */
export function useReducer(reducer, initialState, init) {
    const hookState = getHookState(currentHookIndex++);
    
    if (!hookState.initialized) {
        hookState.initialized = true;
        hookState.state = init ? init(initialState) : initialState;
    }
    
    const component = currentComponent;
    
    const dispatch = (action) => {
        const nextState = reducer(hookState.state, action);
        if (nextState !== hookState.state) {
            hookState.state = nextState;
            if (component && component.__rerender) {
                component.__rerender();
            }
        }
    };
    
    return [hookState.state, dispatch];
}

// Default export
export default {
    useState,
    useEffect,
    useLayoutEffect,
    useRef,
    useMemo,
    useCallback,
    useContext,
    useReducer,
    createContext,
    setCurrentComponent
};

