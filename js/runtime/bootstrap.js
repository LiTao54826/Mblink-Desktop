/**
 * @file bootstrap.js
 * @brief JavaScript运行时引导程序
 * 
 * 功能：
 * - 初始化全局对象
 * - 设置console API
 * - 设置setTimeout/setInterval
 * - 初始化DOM
 * - 加载Preact
 * 
 * TODO:
 * - [ ] 实现console API
 * - [ ] 实现定时器API
 * - [ ] 初始化document对象
 * - [ ] 加载Preact库
 */

(function(global) {
    'use strict';
    
    // ========== Console API ==========
    // TODO: 实现console.log, console.error等
    global.console = {
        log: function(...args) {
            // TODO: 调用C++的日志函数
            // __native_log('log', args);
            print('[LOG]', ...args);
        },
        error: function(...args) {
            // TODO: 调用C++的日志函数
            // __native_log('error', args);
            print('[ERROR]', ...args);
        },
        warn: function(...args) {
            // TODO: 调用C++的日志函数
            // __native_log('warn', args);
            print('[WARN]', ...args);
        },
        info: function(...args) {
            // TODO: 调用C++的日志函数
            // __native_log('info', args);
            print('[INFO]', ...args);
        },
        debug: function(...args) {
            // TODO: 调用C++的日志函数
            // __native_log('debug', args);
            print('[DEBUG]', ...args);
        }
    };
    
    // ========== 定时器API ==========
    // TODO: 实现setTimeout, setInterval, clearTimeout, clearInterval
    let timerIdCounter = 1;
    const timers = new Map();
    
    global.setTimeout = function(callback, delay, ...args) {
        // TODO: 调用C++的定时器函数
        const id = timerIdCounter++;
        // __native_set_timeout(id, callback, delay, args);
        timers.set(id, { callback, delay, args, type: 'timeout' });
        return id;
    };
    
    global.clearTimeout = function(id) {
        // TODO: 调用C++的清除定时器函数
        // __native_clear_timeout(id);
        timers.delete(id);
    };
    
    global.setInterval = function(callback, delay, ...args) {
        // TODO: 调用C++的定时器函数
        const id = timerIdCounter++;
        // __native_set_interval(id, callback, delay, args);
        timers.set(id, { callback, delay, args, type: 'interval' });
        return id;
    };
    
    global.clearInterval = function(id) {
        // TODO: 调用C++的清除定时器函数
        // __native_clear_interval(id);
        timers.delete(id);
    };
    
    // ========== requestAnimationFrame ==========
    // TODO: 实现requestAnimationFrame
    global.requestAnimationFrame = function(callback) {
        // TODO: 调用C++的RAF函数
        // return __native_request_animation_frame(callback);
        return setTimeout(callback, 16); // 临时实现：60fps
    };
    
    global.cancelAnimationFrame = function(id) {
        // TODO: 调用C++的取消RAF函数
        // __native_cancel_animation_frame(id);
        clearTimeout(id);
    };
    
    // ========== 全局对象 ==========
    global.window = global;
    global.self = global;
    global.globalThis = global;
    
    // ========== 初始化DOM ==========
    // TODO: 从C++创建document对象
    // global.document = __native_create_document();
    
    console.log('MBink JavaScript runtime initialized');
    
})(this);

