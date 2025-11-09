/**
 * @file dom.js
 * @brief DOM API Polyfills
 * 
 * 功能：
 * - 为C++实现的DOM API提供JavaScript包装
 * - 添加缺失的DOM API
 * - 兼容性处理
 * 
 * TODO:
 * - [ ] 实现Element.prototype扩展
 * - [ ] 实现Node.prototype扩展
 * - [ ] 实现Document.prototype扩展
 * - [ ] 添加缺失的API
 */

(function(global) {
    'use strict';
    
    // ========== Element扩展 ==========
    
    // classList API
    if (!Element.prototype.classList) {
        Object.defineProperty(Element.prototype, 'classList', {
            get: function() {
                const element = this;
                return {
                    add: function(className) {
                        element.className = (element.className + ' ' + className).trim();
                    },
                    remove: function(className) {
                        element.className = element.className
                            .split(' ')
                            .filter(c => c !== className)
                            .join(' ');
                    },
                    toggle: function(className) {
                        if (this.contains(className)) {
                            this.remove(className);
                            return false;
                        } else {
                            this.add(className);
                            return true;
                        }
                    },
                    contains: function(className) {
                        return element.className.split(' ').includes(className);
                    }
                };
            }
        });
    }
    
    // matches API
    if (!Element.prototype.matches) {
        Element.prototype.matches = function(selector) {
            // TODO: 实现CSS选择器匹配
            // 临时实现：只支持简单选择器
            if (selector.startsWith('#')) {
                return this.id === selector.slice(1);
            } else if (selector.startsWith('.')) {
                return this.classList.contains(selector.slice(1));
            } else {
                return this.tagName.toLowerCase() === selector.toLowerCase();
            }
        };
    }
    
    // closest API
    if (!Element.prototype.closest) {
        Element.prototype.closest = function(selector) {
            let element = this;
            while (element && element.nodeType === 1) {
                if (element.matches(selector)) {
                    return element;
                }
                element = element.parentNode;
            }
            return null;
        };
    }
    
    // ========== Node扩展 ==========
    
    // append API
    if (!Element.prototype.append) {
        Element.prototype.append = function(...nodes) {
            nodes.forEach(node => {
                if (typeof node === 'string') {
                    this.appendChild(document.createTextNode(node));
                } else {
                    this.appendChild(node);
                }
            });
        };
    }
    
    // prepend API
    if (!Element.prototype.prepend) {
        Element.prototype.prepend = function(...nodes) {
            const firstChild = this.firstChild;
            nodes.forEach(node => {
                if (typeof node === 'string') {
                    this.insertBefore(document.createTextNode(node), firstChild);
                } else {
                    this.insertBefore(node, firstChild);
                }
            });
        };
    }
    
    // remove API
    if (!Element.prototype.remove) {
        Element.prototype.remove = function() {
            if (this.parentNode) {
                this.parentNode.removeChild(this);
            }
        };
    }
    
    // ========== Document扩展 ==========
    
    // getElementById (如果C++没有实现)
    if (!Document.prototype.getElementById) {
        Document.prototype.getElementById = function(id) {
            // TODO: 调用C++实现或遍历DOM树
            return this.querySelector('#' + id);
        };
    }
    
    // getElementsByClassName (如果C++没有实现)
    if (!Document.prototype.getElementsByClassName) {
        Document.prototype.getElementsByClassName = function(className) {
            // TODO: 调用C++实现或遍历DOM树
            return this.querySelectorAll('.' + className);
        };
    }
    
    // getElementsByTagName (如果C++没有实现)
    if (!Document.prototype.getElementsByTagName) {
        Document.prototype.getElementsByTagName = function(tagName) {
            // TODO: 调用C++实现或遍历DOM树
            return this.querySelectorAll(tagName);
        };
    }
    
    // ========== Event扩展 ==========
    
    // Event构造函数
    if (typeof Event === 'undefined') {
        global.Event = function(type, options) {
            this.type = type;
            this.bubbles = options && options.bubbles || false;
            this.cancelable = options && options.cancelable || false;
            this.target = null;
            this.currentTarget = null;
            this.defaultPrevented = false;
        };
        
        Event.prototype.preventDefault = function() {
            if (this.cancelable) {
                this.defaultPrevented = true;
            }
        };
        
        Event.prototype.stopPropagation = function() {
            // TODO: 实现事件传播停止
        };
    }
    
    // CustomEvent构造函数
    if (typeof CustomEvent === 'undefined') {
        global.CustomEvent = function(type, options) {
            const event = new Event(type, options);
            event.detail = options && options.detail;
            return event;
        };
    }
    
    console.log('DOM polyfills loaded');
    
})(this);

