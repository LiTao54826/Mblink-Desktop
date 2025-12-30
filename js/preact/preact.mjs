/**
 * Preact Core - ES6 Module version for MBink
 * Based on Preact 10.x API
 * 
 * Usage: import { h, render } from 'preact';
 */

// VNode type constants
const VNODE_TYPE_ELEMENT = 1;
const VNODE_TYPE_TEXT = 2;
const VNODE_TYPE_COMPONENT = 3;

/**
 * Create a Virtual DOM node (VNode)
 */
export function h(type, props, ...children) {
    // Flatten children array
    const flatChildren = [];
    for (const child of children) {
        if (Array.isArray(child)) {
            flatChildren.push(...child);
        } else if (child != null && child !== false && child !== true) {
            flatChildren.push(child);
        }
    }

    return {
        type: type,
        props: props || {},
        children: flatChildren,
        key: props ? props.key : undefined,
        ref: props ? props.ref : undefined,
        __v: VNODE_TYPE_ELEMENT
    };
}

/**
 * Create a VNode (alias for h)
 */
export const createElement = h;

/**
 * Fragment component - renders children without wrapper
 */
export function Fragment(props) {
    return props.children;
}

/**
 * Render a VNode tree into a DOM container
 */
export function render(vnode, container) {
    if (container.__preactRoot) {
        diff(container.__preactRoot, vnode, container);
    } else {
        container.innerHTML = '';
        const dom = createDOMElement(vnode);
        if (dom) {
            container.appendChild(dom);
        }
        container.__preactRoot = vnode;
    }
}

/**
 * Create a real DOM element from a VNode
 */
function createDOMElement(vnode) {
    if (vnode == null || vnode === false || vnode === true) {
        return null;
    }
    
    if (typeof vnode === 'string' || typeof vnode === 'number') {
        return document.createTextNode(String(vnode));
    }
    
    // Handle component functions
    if (typeof vnode.type === 'function') {
        if (!vnode.__component) {
            vnode.__component = {
                __hooks: [],
                __vnode: vnode,
                __dom: null,
                __rerender: null
            };
        }

        const component = vnode.__component;

        component.__rerender = function() {
            if (typeof setCurrentComponent !== 'undefined') {
                setCurrentComponent(component);
            }
            const newVNode = vnode.type(vnode.props);
            if (typeof setCurrentComponent !== 'undefined') {
                setCurrentComponent(null);
            }

            const oldDOM = component.__dom;
            if (!oldDOM || !oldDOM.parentNode) return;

            const parent = oldDOM.parentNode;
            const newDOM = createDOMElement(newVNode);

            if (newDOM) {
                parent.replaceChild(newDOM, oldDOM);
                component.__dom = newDOM;
                component.__renderedVNode = newVNode;
                newDOM.__componentVNode = vnode;
            }
        };

        if (typeof setCurrentComponent !== 'undefined') {
            setCurrentComponent(component);
        }
        const componentVNode = vnode.type(vnode.props);
        if (typeof setCurrentComponent !== 'undefined') {
            setCurrentComponent(null);
        }

        const dom = createDOMElement(componentVNode);
        component.__dom = dom;
        component.__renderedVNode = componentVNode;
        if (dom) {
            dom.__componentVNode = vnode;
        }
        return dom;
    }
    
    // Handle Fragment
    if (vnode.type === Fragment) {
        const fragment = document.createDocumentFragment();
        for (const child of vnode.children) {
            const childDOM = createDOMElement(child);
            if (childDOM) {
                fragment.appendChild(childDOM);
            }
        }
        return fragment;
    }
    
    // Handle regular elements
    const element = document.createElement(vnode.type);
    
    // Check for dangerouslySetInnerHTML first
    let hasDangerousHTML = false;
    
    if (vnode.props) {
        for (const [key, value] of Object.entries(vnode.props)) {
            if (key === 'key' || key === 'ref' || key === 'children') continue;
            
            if (key === 'dangerouslySetInnerHTML' && value && value.__html != null) {
                // Handle dangerouslySetInnerHTML - set innerHTML directly
                element.innerHTML = value.__html;
                hasDangerousHTML = true;
            } else if (key.startsWith('on') && typeof value === 'function') {
                element.addEventListener(key.substring(2).toLowerCase(), value);
            } else if (key === 'className') {
                element.className = value;
            } else if (key === 'style' && typeof value === 'object') {
                Object.assign(element.style, value);
            } else if (key === 'contentEditable') {
                // contentEditable 需要设置为字符串 "true" 或 "false"
                element.setAttribute('contenteditable', value === true ? 'true' : String(value));
            } else if (typeof value === 'boolean') {
                if (value) element.setAttribute(key, '');
            } else if (value != null) {
                element.setAttribute(key, String(value));
            }
        }
    }
    
    // Only add children if dangerouslySetInnerHTML was not used
    if (!hasDangerousHTML && vnode.children) {
        for (const child of vnode.children) {
            const childDOM = createDOMElement(child);
            if (childDOM) {
                element.appendChild(childDOM);
            }
        }
    }
    
    element.__vnode = vnode;
    
    if (vnode.props?.ref) {
        if (typeof vnode.props.ref === 'function') {
            vnode.props.ref(element);
        } else {
            vnode.props.ref.current = element;
        }
    }
    
    return element;
}

/**
 * Diff and update DOM
 */
function diff(oldVNode, newVNode, container) {
    container.innerHTML = '';
    const dom = createDOMElement(newVNode);
    if (dom) {
        container.appendChild(dom);
    }
    container.__preactRoot = newVNode;
}

/**
 * Create a ref object
 */
export function createRef(initialValue) {
    return { current: initialValue };
}

/**
 * Clone a VNode with new props
 */
export function cloneElement(vnode, props, ...children) {
    return {
        ...vnode,
        props: { ...vnode.props, ...props },
        children: children.length > 0 ? children : vnode.children
    };
}

/**
 * Check if a value is a valid element
 */
export function isValidElement(value) {
    return value != null && typeof value === 'object' && value.__v === VNODE_TYPE_ELEMENT;
}

/**
 * Component base class
 */
export class Component {
    constructor(props) {
        this.props = props;
        this.state = {};
    }
    
    setState(update) {
        if (typeof update === 'function') {
            this.state = { ...this.state, ...update(this.state, this.props) };
        } else {
            this.state = { ...this.state, ...update };
        }
        
        if (this.__container && this.__vnode) {
            const newVNode = this.render();
            diff(this.__vnode, newVNode, this.__container);
            this.__vnode = newVNode;
        }
    }
    
    render() {
        return null;
    }
}

/**
 * Create a Context object for passing data through the component tree
 * @param {any} defaultValue - Default value when no Provider is found
 * @returns {object} Context object with Provider and Consumer
 */
export function createContext(defaultValue) {
    const context = {
        __defaultValue: defaultValue,
        __currentValue: defaultValue,
        __listeners: []
    };

    /**
     * Provider component - provides value to descendants
     */
    function Provider(props) {
        const value = props.value !== undefined ? props.value : defaultValue;
        context.__currentValue = value;

        // Notify listeners
        for (const listener of context.__listeners) {
            listener(value);
        }

        return props.children;
    }

    /**
     * Consumer component - consumes context value via render prop
     */
    function Consumer(props) {
        const children = props.children;
        if (typeof children === 'function') {
            return children(context.__currentValue);
        }
        return children;
    }

    context.Provider = Provider;
    context.Consumer = Consumer;

    // For useContext hook support
    context.__getValue = function() {
        return context.__currentValue;
    };

    context.__subscribe = function(listener) {
        context.__listeners.push(listener);
        return function() {
            const idx = context.__listeners.indexOf(listener);
            if (idx > -1) {
                context.__listeners.splice(idx, 1);
            }
        };
    };

    return context;
}

// Default export
export default { h, createElement, render, Fragment, Component, createRef, cloneElement, isValidElement, createContext };

