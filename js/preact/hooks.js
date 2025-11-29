/**
 * Preact Hooks - Simplified version for MBink
 * Based on Preact Hooks API
 * 
 * Implements: useState, useEffect, useRef, useMemo, useCallback, useContext
 */

// Global state for hooks
let currentComponent = null;
let currentHookIndex = 0;

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
    const hookState = getHookState(currentHookIndex++);

    if (!('value' in hookState)) {
        hookState.value = typeof initialValue === 'function' ? initialValue() : initialValue;
    }

    // Capture the component reference when creating setState
    const component = currentComponent;

    const setState = (newValue) => {
        const nextValue = typeof newValue === 'function'
            ? newValue(hookState.value)
            : newValue;

        if (hookState.value !== nextValue) {
            hookState.value = nextValue;
            // Trigger re-render using captured component reference
            if (component && component.__rerender) {
                component.__rerender();
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
    const hookState = getHookState(currentHookIndex++);
    
    const hasChanged = !hookState.deps || 
        !deps || 
        deps.some((dep, i) => dep !== hookState.deps[i]);
    
    if (hasChanged) {
        hookState.deps = deps;
        
        // Schedule effect to run after render
        if (typeof setTimeout !== 'undefined') {
            setTimeout(() => {
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
    setCurrentComponent: setCurrentComponent
};
