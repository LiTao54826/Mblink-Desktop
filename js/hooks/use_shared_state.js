/**
 * @file use_shared_state.js
 * @brief useSharedState Hook - 跨语言共享状态的 Preact Hook
 * 
 * 提供与宿主语言共享状态的 React-like Hook：
 * - 自动订阅状态变化
 * - 组件卸载时自动取消订阅
 * - 状态变化触发重渲染
 */

import { useState, useEffect, useCallback } from 'preact/hooks';

/**
 * useSharedState Hook
 * 
 * 用于在 Preact 组件中访问和修改跨语言共享状态。
 * 
 * @param {string} name - 状态名称
 * @param {*} [defaultValue] - 默认值（如果状态不存在）
 * @returns {[*, function]} - [当前值, 设置函数]
 * 
 * @example
 * function Counter() {
 *   const [count, setCount] = useSharedState('counter', 0);
 *   return h('button', { onClick: () => setCount(count + 1) }, `Count: ${count}`);
 * }
 */
export function useSharedState(name, defaultValue = null) {
    // 检查 host.state 是否可用
    if (typeof host === 'undefined' || !host.state) {
        console.warn('[useSharedState] host.state not available, using local state');
        return useState(defaultValue);
    }
    
    // 初始化状态：如果不存在则使用默认值
    const getInitialValue = () => {
        if (host.state.exists(name)) {
            return host.state.get(name);
        }
        // 设置默认值
        host.state.set(name, defaultValue);
        return defaultValue;
    };
    
    // 本地状态用于触发重渲染
    const [value, setValue] = useState(getInitialValue);
    
    // 设置函数
    const setSharedValue = useCallback((newValue) => {
        // 支持函数式更新
        const actualValue = typeof newValue === 'function' 
            ? newValue(host.state.get(name)) 
            : newValue;
        
        host.state.set(name, actualValue);
        setValue(actualValue);
    }, [name]);
    
    // 订阅状态变化
    useEffect(() => {
        // 监听状态变化
        const watchId = host.state.watch(name, (stateName, newValue) => {
            setValue(newValue);
        });
        
        // 清理函数：取消订阅
        return () => {
            if (typeof host.state.unwatch === 'function') {
                host.state.unwatch(watchId);
            }
        };
    }, [name]);
    
    return [value, setSharedValue];
}

/**
 * useSharedStateValue Hook
 * 
 * 只读版本的 useSharedState，用于只需要读取状态的组件。
 * 
 * @param {string} name - 状态名称
 * @param {*} [defaultValue] - 默认值
 * @returns {*} - 当前值
 */
export function useSharedStateValue(name, defaultValue = null) {
    const [value] = useSharedState(name, defaultValue);
    return value;
}

/**
 * useSharedStateSetter Hook
 * 
 * 只返回设置函数，用于只需要修改状态的组件。
 * 不会订阅状态变化，避免不必要的重渲染。
 * 
 * @param {string} name - 状态名称
 * @returns {function} - 设置函数
 */
export function useSharedStateSetter(name) {
    return useCallback((newValue) => {
        if (typeof host === 'undefined' || !host.state) {
            console.warn('[useSharedStateSetter] host.state not available');
            return;
        }
        
        const actualValue = typeof newValue === 'function'
            ? newValue(host.state.get(name))
            : newValue;
        
        host.state.set(name, actualValue);
    }, [name]);
}

// 默认导出
export default useSharedState;
