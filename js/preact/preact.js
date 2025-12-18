/**
 * Preact Core - Simplified version for MBink
 * Based on Preact 10.x API
 *
 * This is a lightweight implementation optimized for MBink's QuickJS runtime.
 * Implements real Virtual DOM diffing for efficient updates.
 *
 * 使用 IIFE 封装避免全局变量污染，只暴露 preact/Preact 对象
 */

(function (global) {
    'use strict';

    // VNode type constants
    var VNODE_TYPE_ELEMENT = 1;
    var VNODE_TYPE_TEXT = 2;
    var VNODE_TYPE_COMPONENT = 3;

/**
 * Create a Virtual DOM node (VNode)
 * @param {string|Function} type - Element tag name or component function
 * @param {object} props - Element properties/attributes
 * @param {...any} children - Child nodes
 * @returns {object} VNode
 */
function h(type, props) {
    // Get children from arguments (ES5 compatible)
    var children = [];
    for (var i = 2; i < arguments.length; i++) {
        children.push(arguments[i]);
    }

    // Flatten children array
    var flatChildren = [];
    for (var j = 0; j < children.length; j++) {
        var child = children[j];
        if (Array.isArray(child)) {
            for (var k = 0; k < child.length; k++) {
                flatChildren.push(child[k]);
            }
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
 * Get the key for a VNode (for keyed diffing)
 */
function getKey(vnode, index) {
    if (vnode == null) return null;
    if (typeof vnode === 'string' || typeof vnode === 'number') return null;
    return vnode.key != null ? vnode.key : index;
}

/**
 * Check if two VNodes are the same type (can be diffed)
 */
function isSameVNodeType(oldVNode, newVNode) {
    if (oldVNode == null || newVNode == null) return false;

    // Both are text
    if ((typeof oldVNode === 'string' || typeof oldVNode === 'number') &&
        (typeof newVNode === 'string' || typeof newVNode === 'number')) {
        return true;
    }

    // Both are vnodes with same type
    if (typeof oldVNode === 'object' && typeof newVNode === 'object') {
        return oldVNode.type === newVNode.type;
    }

    return false;
}

    /**
     * Create a VNode (alias for h)
     */
    var createElement = h;

/**
 * Fragment component - renders children without wrapper
 */
function Fragment(props) {
    return props.children;
}

/**
 * Render a VNode tree into a DOM container
 * @param {object} vnode - Virtual DOM node
 * @param {Element} container - DOM container element
 */
function render(vnode, container) {
    // Use C++ implementation if available
    if (typeof __preact_internal !== 'undefined' && __preact_internal.render) {
        return __preact_internal.render(vnode, container);
    }

    // Get old vnode from container
    var oldVNode = container.__preactVNode;
    var oldDOM = container.__preactDOM;

    // Diff and patch
    var newDOM = diffNode(oldVNode, vnode, container, oldDOM);

    // Store references
    container.__preactVNode = vnode;
    container.__preactDOM = newDOM;
}

/**
 * Create a real DOM element from a VNode (initial render only)
 * @param {object} vnode - Virtual DOM node
 * @returns {Element|Text} DOM element
 */
function createDOMElement(vnode) {
    // Handle null/undefined
    if (vnode == null || vnode === false || vnode === true) {
        return null;
    }

    // Handle text nodes
    if (typeof vnode === 'string' || typeof vnode === 'number') {
        console.log('[createDOMElement] Creating text node: "' + vnode + '"');
        var textNode = document.createTextNode(String(vnode));
        console.log('[createDOMElement] Text node created, textContent="' + textNode.textContent + '"');
        return textNode;
    }

    // Handle component functions
    if (typeof vnode.type === 'function') {
        return createComponentDOM(vnode);
    }

    // Handle Fragment
    if (vnode.type === Fragment) {
        var fragment = document.createDocumentFragment();
        for (var i = 0; i < vnode.children.length; i++) {
            var childDOM = createDOMElement(vnode.children[i]);
            if (childDOM) {
                fragment.appendChild(childDOM);
            }
        }
        return fragment;
    }

    // Handle regular elements
    // Check if this is an SVG element or if we're inside an SVG context
    var isSVG = vnode.type === 'svg' || vnode.__isSVG;
    var SVG_TAGS = ['svg', 'circle', 'ellipse', 'line', 'path', 'polygon', 'polyline', 'rect', 'g', 'text', 'tspan', 'defs', 'use', 'symbol', 'clipPath', 'mask', 'pattern', 'image', 'foreignObject', 'linearGradient', 'radialGradient', 'stop'];
    if (SVG_TAGS.indexOf(vnode.type) !== -1) {
        isSVG = true;
    }

    var element;
    if (isSVG) {
        element = document.createElementNS('http://www.w3.org/2000/svg', vnode.type);
    } else {
        element = document.createElement(vnode.type);
    }

    // Set properties
    setDOMProps(element, {}, vnode.props || {}, isSVG);

    // Append children
    if (vnode.children) {
        for (var j = 0; j < vnode.children.length; j++) {
            var child = vnode.children[j];
            // Pass SVG context to children
            if (isSVG && child && typeof child === 'object') {
                child.__isSVG = true;
            }
            var childEl = createDOMElement(child);
            if (childEl) {
                element.appendChild(childEl);
            }
        }
    }

    // Store VNode reference (using global storage instead of direct property)
    setElementVNode(element, vnode);
    vnode.__dom = element;

    // Handle ref
    if (vnode.props && vnode.props.ref) {
        if (typeof vnode.props.ref === 'function') {
            vnode.props.ref(element);
        } else {
            vnode.props.ref.current = element;
        }
    }

    return element;
}

/**
 * Create DOM for a component VNode
 */
function createComponentDOM(vnode) {
    // Create component instance for hooks
    if (!vnode.__component) {
        vnode.__component = {
            __hooks: [],
            __vnode: vnode,
            __dom: null,
            __renderedVNode: null
        };
    }

    var component = vnode.__component;

    // Set up rerender function using Virtual DOM diffing
    component.__rerender = function () {
        try {
            // Set current component for hooks
            if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                PreactHooks.setCurrentComponent(component);
            }

            // IMPORTANT: Use component.__vnode.props to get the latest props
            var currentVNode = component.__vnode;
            var newRenderedVNode = currentVNode.type(currentVNode.props);

            // Clear current component
            if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                PreactHooks.setCurrentComponent(null);
            }

            // Get old DOM and parent
            var oldDOM = component.__dom;
            if (!oldDOM || !oldDOM.parentNode) {
                return;
            }

            var parent = oldDOM.parentNode;
            var oldRenderedVNode = component.__renderedVNode;

            // Use Virtual DOM diffing to update in place
            var newDOM = diffNode(oldRenderedVNode, newRenderedVNode, parent, oldDOM);

            // Update component state
            component.__dom = newDOM;
            component.__renderedVNode = newRenderedVNode;
            if (newDOM) {
                newDOM.__componentVNode = vnode;
            }
        } catch (e) {
            // Silently handle errors
        }
    };

    // Set current component for hooks
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(component);
    }

    // Call component function
    var renderedVNode = vnode.type(vnode.props);

    // Clear current component
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(null);
    }

    // Create DOM element
    var dom = createDOMElement(renderedVNode);

    // Store DOM reference and rendered VNode
    component.__dom = dom;
    component.__renderedVNode = renderedVNode;

    // Store reference to the component VNode on the DOM
    if (dom) {
        dom.__componentVNode = vnode;
    }

    return dom;
}

/**
 * Global storage for DOM element data - avoids setting custom properties on DOM elements
 * which can cause issues with QuickJS
 */
var __elementDataStore = {};
var __elementIdCounter = 1;

function getElementId(element) {
    // Use a data attribute to store element ID (safer than custom JS property)
    var id = element.getAttribute('data-preact-id');
    if (!id) {
        id = String(__elementIdCounter++);
        element.setAttribute('data-preact-id', id);
        __elementDataStore[id] = { listeners: {}, handlers: {} };
    }
    return id;
}

function getElementData(element) {
    var id = getElementId(element);
    if (!__elementDataStore[id]) {
        __elementDataStore[id] = { listeners: {}, handlers: {} };
    }
    return __elementDataStore[id];
}

function setElementVNode(element, vnode) {
    var data = getElementData(element);
    data.vnode = vnode;
}

function getElementVNode(element) {
    var id = element.getAttribute('data-preact-id');
    if (id && __elementDataStore[id]) {
        return __elementDataStore[id].vnode;
    }
    return null;
}

/**
 * Create a stable event handler wrapper that delegates to the latest handler
 * This allows us to update the handler without removing/adding listeners
 */
function createStableHandler(elementId, eventKey) {
    return function (event) {
        var data = __elementDataStore[elementId];
        if (data && data.handlers && data.handlers[eventKey]) {
            data.handlers[eventKey](event);
        }
    };
}

/**
 * Set DOM properties, handling events, styles, etc.
 * Uses stable event handler wrappers to avoid add/remove listener on every render
 */
function setDOMProps(element, oldProps, newProps, isSVG) {
    var elementId = getElementId(element);
    var data = __elementDataStore[elementId];
    var listeners = data.listeners;
    var handlers = data.handlers;

    // Remove old event listeners and attributes
    for (var key in oldProps) {
        if (key === 'key' || key === 'ref' || key === 'children') continue;

        if (!(key in newProps)) {
            if (key.substring(0, 2) === 'on' && typeof oldProps[key] === 'function') {
                var eventName = key.substring(2).toLowerCase();
                // Remove using stored listener ID
                if (listeners[key]) {
                    element.removeEventListener(eventName, listeners[key]);
                    if (eventName === 'change' && listeners[key + '_input']) {
                        element.removeEventListener('input', listeners[key + '_input']);
                    }
                    delete listeners[key];
                    delete listeners[key + '_input'];
                    delete handlers[key];
                    delete handlers[key + '_input'];
                }
            } else if (key === 'className') {
                element.className = '';
            } else if (key === 'style') {
                element.style.cssText = '';
            } else {
                element.removeAttribute(key);
            }
        }
    }

    // Set new properties
    for (var prop in newProps) {
        if (prop === 'key' || prop === 'ref' || prop === 'children') continue;

        var newValue = newProps[prop];
        var oldValue = oldProps[prop];

        if (prop.substring(0, 2) === 'on' && typeof newValue === 'function') {
            var evtName = prop.substring(2).toLowerCase();

            // Always update the handler reference (so latest closure is called)
            handlers[prop] = newValue;
            if (evtName === 'change') {
                handlers[prop + '_input'] = newValue;
            }

            // Only add listener if not already added
            if (!listeners[prop]) {
                // Create stable wrapper and add listener
                var stableHandler = createStableHandler(elementId, prop);
                element.addEventListener(evtName, stableHandler);
                listeners[prop] = stableHandler;  // Store the wrapper function itself

                if (evtName === 'change') {
                    var stableInputHandler = createStableHandler(elementId, prop + '_input');
                    element.addEventListener('input', stableInputHandler);
                    listeners[prop + '_input'] = stableInputHandler;  // Store the wrapper function itself
                }
            }
        } else if (prop === 'value' && (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA')) {
            // For controlled inputs, always check against current DOM value
            // Don't skip based on oldValue because DOM value can be changed by user input
            if (element.value !== String(newValue)) {
                element.value = newValue;
            }
        } else if (newValue === oldValue) {
            // Skip unchanged non-event props
            continue;
        } else if (prop === 'className') {
            element.className = newValue || '';
        } else if (prop === 'style') {
            if (typeof newValue === 'string') {
                element.style.cssText = newValue;
            } else if (typeof newValue === 'object') {
                // Clear old styles first if old value was also object
                if (typeof oldValue === 'object' && oldValue) {
                    for (var oldStyle in oldValue) {
                        if (!(oldStyle in newValue)) {
                            element.style[oldStyle] = '';
                        }
                    }
                }
                for (var styleProp in newValue) {
                    element.style[styleProp] = newValue[styleProp];
                }
            }
        } else if (typeof newValue === 'boolean') {
            if (newValue) {
                element.setAttribute(prop, '');
            } else {
                element.removeAttribute(prop);
            }
        } else if (prop === 'innerHTML') {
            // innerHTML 是 DOM 属性，不是 HTML 属性
            element.innerHTML = newValue || '';
        } else if (prop === 'textContent') {
            // textContent 也是 DOM 属性
            element.textContent = newValue || '';
        } else if (newValue != null) {
            element.setAttribute(prop, String(newValue));
        } else {
            element.removeAttribute(prop);
        }
    }
}

/**
 * Diff a single node and return the (potentially new) DOM node
 * This is the core of Virtual DOM diffing
 */
function diffNode(oldVNode, newVNode, parentDOM, oldDOM) {
    var oldType = oldVNode ? (typeof oldVNode === 'object' ? oldVNode.type : typeof oldVNode) : 'null';
    var newType = newVNode ? (typeof newVNode === 'object' ? newVNode.type : typeof newVNode) : 'null';
    // console.log('[diffNode] old=' + oldType + ', new=' + newType);

    try {
        // New node is null - remove old
        if (newVNode == null || newVNode === false || newVNode === true) {
            // console.log('[diffNode] newVNode is null/false/true, removing old');
            if (oldDOM && parentDOM) {
                parentDOM.removeChild(oldDOM);
            }
            return null;
        }

        // Old node is null - create new
        if (oldVNode == null || oldVNode === false || oldVNode === true || !oldDOM) {
            // console.log('[diffNode] oldVNode is null, creating new');
            var newDOM = createDOMElement(newVNode);
            if (newDOM && parentDOM) {
                parentDOM.appendChild(newDOM);
            }
            return newDOM;
        }

        // Both are text nodes
        if ((typeof oldVNode === 'string' || typeof oldVNode === 'number') &&
            (typeof newVNode === 'string' || typeof newVNode === 'number')) {
            if (String(oldVNode) !== String(newVNode)) {
                console.log('[diffNode] Updating text: "' + oldVNode + '" -> "' + newVNode + '"');
                console.log('[diffNode] oldDOM.nodeType=' + oldDOM.nodeType + ', oldDOM.textContent="' + oldDOM.textContent + '"');
                oldDOM.textContent = String(newVNode);
                console.log('[diffNode] After update: oldDOM.textContent="' + oldDOM.textContent + '"');
            }
            return oldDOM;
        }

        // Type changed - replace entirely
        if (!isSameVNodeType(oldVNode, newVNode)) {
            // console.log('[diffNode] Type changed, replacing');
            var replacementDOM = createDOMElement(newVNode);
            if (parentDOM && oldDOM) {
                parentDOM.replaceChild(replacementDOM, oldDOM);
            }
            return replacementDOM;
        }

        // Both are components of the same type
        if (typeof newVNode.type === 'function') {
            // console.log('[diffNode] Both components, calling diffComponent');
            // IMPORTANT: Transfer component reference from old to new VNode
            // This is necessary because newVNode is freshly created and doesn't have __component
            if (oldVNode.__component && !newVNode.__component) {
                newVNode.__component = oldVNode.__component;
            }
            return diffComponent(oldVNode, newVNode, parentDOM, oldDOM);
        }

        // Both are elements of the same type - update in place
        // console.log('[diffNode] Both elements of type ' + newVNode.type + ', calling diffElement');
        return diffElement(oldVNode, newVNode, oldDOM);
    } catch (e) {
        // console.log('[diffNode] Error: ' + e.message);
        throw e;
    }
}

/**
 * Diff component VNodes
 */
function diffComponent(oldVNode, newVNode, parentDOM, oldDOM) {
    var componentName = newVNode.type.name || 'Anonymous';
    // console.log('[diffComponent] Component: ' + componentName);

    // Reuse the component instance
    var component = oldVNode.__component;
    if (!component) {
        // No old component, create new
        // console.log('[diffComponent] No old component, creating new');
        return createDOMElement(newVNode);
    }

    // Transfer component to new vnode
    newVNode.__component = component;
    component.__vnode = newVNode;

    // Set current component for hooks
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(component);
    }

    // Log props comparison
    // console.log('[diffComponent] ' + componentName + ' old props keys: ' + Object.keys(oldVNode.props || {}).join(','));
    // console.log('[diffComponent] ' + componentName + ' new props keys: ' + Object.keys(newVNode.props || {}).join(','));

    // Get new rendered VNode
    var newRenderedVNode = newVNode.type(newVNode.props);

    // Clear current component
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(null);
    }

    var oldRenderedVNode = component.__renderedVNode;
    // console.log('[diffComponent] ' + componentName + ' calling diffNode on rendered output');

    // Diff the rendered output
    var newDOM = diffNode(oldRenderedVNode, newRenderedVNode, parentDOM, oldDOM);

    // Update component
    component.__dom = newDOM;
    component.__renderedVNode = newRenderedVNode;

    // Update rerender function - use component.__vnode to get latest props
    component.__rerender = function () {
        // console.log('[Preact __rerender] Starting rerender');
        if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
            PreactHooks.setCurrentComponent(component);
        }

        // IMPORTANT: Use component.__vnode.props instead of captured newVNode.props
        // This ensures we use the latest props from parent component
        var currentVNode = component.__vnode;
        var updatedVNode = currentVNode.type(currentVNode.props);

        if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
            PreactHooks.setCurrentComponent(null);
        }

        var currentDOM = component.__dom;
        if (!currentDOM || !currentDOM.parentNode) {
            return;
        }

        var resultDOM = diffNode(component.__renderedVNode, updatedVNode, currentDOM.parentNode, currentDOM);
        component.__dom = resultDOM;
        component.__renderedVNode = updatedVNode;
    };

    return newDOM;
}

/**
 * Diff element VNodes (same tag type)
 */
function diffElement(oldVNode, newVNode, dom) {
    var tagName = dom && dom.tagName ? dom.tagName.toLowerCase() : 'unknown';
    // console.log('[diffElement] Updating ' + tagName + ' element');

    // Check if this is an SVG element
    var SVG_TAGS = ['svg', 'circle', 'ellipse', 'line', 'path', 'polygon', 'polyline', 'rect', 'g', 'text', 'tspan', 'defs', 'use', 'symbol', 'clipPath', 'mask', 'pattern', 'image', 'foreignObject', 'linearGradient', 'radialGradient', 'stop'];
    var isSVG = SVG_TAGS.indexOf(tagName) !== -1;

    // Update props - event listeners use stable wrappers so we can update handlers safely
    setDOMProps(dom, oldVNode.props || {}, newVNode.props || {}, isSVG);

    // Diff children
    var oldChildCount = oldVNode.children ? oldVNode.children.length : 0;
    var newChildCount = newVNode.children ? newVNode.children.length : 0;
    // console.log('[diffElement] ' + tagName + ' has ' + oldChildCount + ' old children, ' + newChildCount + ' new children');
    diffChildren(oldVNode.children || [], newVNode.children || [], dom);

    // Update vnode reference (using global storage)
    setElementVNode(dom, newVNode);
    newVNode.__dom = dom;

    // Handle ref
    if (newVNode.props && newVNode.props.ref) {
        if (typeof newVNode.props.ref === 'function') {
            newVNode.props.ref(dom);
        } else {
            newVNode.props.ref.current = dom;
        }
    }

    // console.log('[diffElement] Done updating ' + tagName);
    return dom;
}

/**
 * Diff children arrays using key-based algorithm
 */
function diffChildren(oldChildren, newChildren, parentDOM) {
    if (!parentDOM) {
        // console.log('[diffChildren] No parentDOM, returning');
        return;
    }

    oldChildren = oldChildren || [];
    newChildren = newChildren || [];

    var oldLen = oldChildren.length;
    var newLen = newChildren.length;

    // console.log('[diffChildren] oldLen=' + oldLen + ', newLen=' + newLen);

    // Build a map of old children by key
    var oldKeyedMap = {};  // key -> { vnode, dom, index }
    var oldUnkeyed = [];   // [{ vnode, dom, index }]

    // Get current DOM children as static array
    var childNodesArray = [];
    var childNodes = parentDOM.childNodes;
    if (childNodes) {
        for (var k = 0; k < childNodes.length; k++) {
            childNodesArray.push(childNodes[k]);
        }
    } else {
        // parentDOM.childNodes is undefined, use empty array
    }

    // Map old children
    for (var i = 0; i < oldLen; i++) {
        var oldChild = oldChildren[i];
        var oldDOM = childNodesArray[i];
        var key = getKey(oldChild, null);

        if (key != null) {
            oldKeyedMap[key] = { vnode: oldChild, dom: oldDOM, index: i };
        } else {
            oldUnkeyed.push({ vnode: oldChild, dom: oldDOM, index: i });
        }
    }

    var unkeyedIndex = 0;
    var usedOldDOMs = {};  // Track which old DOMs have been used

    // Process new children
    for (var j = 0; j < newLen; j++) {
        var newChild = newChildren[j];
        var newKey = getKey(newChild, null);
        var oldEntry = null;
        var oldDOM = null;

        if (newKey != null && oldKeyedMap[newKey]) {
            // Found matching keyed element
            oldEntry = oldKeyedMap[newKey];
            oldDOM = oldEntry.dom;
            usedOldDOMs[oldEntry.index] = true;
            // console.log('[diffChildren] Matched key=' + newKey + ' at oldIndex=' + oldEntry.index);
        } else if (newKey == null && unkeyedIndex < oldUnkeyed.length) {
            // Use next unkeyed element
            oldEntry = oldUnkeyed[unkeyedIndex++];
            oldDOM = oldEntry.dom;
            usedOldDOMs[oldEntry.index] = true;
        }

        var currentDOMAtPosition = parentDOM.childNodes[j];

        if (oldDOM) {
            // Diff with matched old DOM
            diffNode(oldEntry.vnode, newChild, parentDOM, oldDOM);

            // Move DOM to correct position if needed
            if (oldDOM !== currentDOMAtPosition) {
                if (currentDOMAtPosition) {
                    parentDOM.insertBefore(oldDOM, currentDOMAtPosition);
                } else {
                    parentDOM.appendChild(oldDOM);
                }
            }
        } else {
            // No matching old DOM, create new
            // console.log('[diffChildren] Creating new child at ' + j);
            var newDOM = createDOMElement(newChild);
            if (newDOM) {
                if (currentDOMAtPosition) {
                    parentDOM.insertBefore(newDOM, currentDOMAtPosition);
                } else {
                    parentDOM.appendChild(newDOM);
                }
            }
        }
    }

    // Remove unused old DOM nodes (iterate backwards to avoid index shifting)
    for (var m = oldLen - 1; m >= 0; m--) {
        if (!usedOldDOMs[m]) {
            var domToRemove = childNodesArray[m];
            if (domToRemove && domToRemove.parentNode === parentDOM) {
                // console.log('[diffChildren] Removing unused child at ' + m);
                parentDOM.removeChild(domToRemove);
            }
        }
    }

    // console.log('[diffChildren] Done, final childCount=' + parentDOM.childNodes.length);
}

/**
 * Create a ref object
 * @param {any} initialValue - Initial value
 * @returns {object} Ref object with .current property
 */
function createRef(initialValue) {
    return { current: initialValue };
}

    /**
     * Clone a VNode with new props
     * @param {object} vnode - VNode to clone
     * @param {object} props - New props
     * @param {...any} children - New children
     * @returns {object} Cloned VNode
     */
    function cloneElement(vnode, props) {
        // ES5 兼容的参数收集
        var children = [];
        for (var i = 2; i < arguments.length; i++) {
            children.push(arguments[i]);
        }

        // 复制 vnode
        var cloned = {};
        for (var key in vnode) {
            cloned[key] = vnode[key];
        }

        // 合并 props
        var newProps = {};
        for (var p in vnode.props) {
            newProps[p] = vnode.props[p];
        }
        for (var q in props) {
            newProps[q] = props[q];
        }
        cloned.props = newProps;

        // 设置 children
        cloned.children = children.length > 0 ? children : vnode.children;

        return cloned;
    }

/**
 * Check if a value is a valid element
 * @param {any} value - Value to check
 * @returns {boolean}
 */
function isValidElement(value) {
    return value != null && typeof value === 'object' && value.__v === VNODE_TYPE_ELEMENT;
}

    /**
     * Component base class (for class components)
     */
    function Component(props) {
        this.props = props;
        this.state = {};
    }

    Component.prototype.setState = function(update) {
        if (typeof update === 'function') {
            var newState = update(this.state, this.props);
            for (var key in newState) {
                this.state[key] = newState[key];
            }
        } else {
            for (var k in update) {
                this.state[k] = update[k];
            }
        }

        // Trigger re-render
        if (this.__container && this.__vnode) {
            var newVNode = this.render();
            diff(this.__vnode, newVNode, this.__container);
            this.__vnode = newVNode;
        }
    };

    Component.prototype.render = function() {
        return null;
    };

    // Export all APIs as global object (for script loading)
    var Preact = {
        h: h,
        createElement: createElement,
        render: render,
        Fragment: Fragment,
        Component: Component,
        createRef: createRef,
        cloneElement: cloneElement,
        isValidElement: isValidElement
    };

    // 添加小写别名以提高兼容性
    var preact = Preact;

    // 暴露到全局作用域
    global.Preact = Preact;
    global.preact = preact;

    // Note: For ES6 module usage, use js/preact/preact.mjs

})(typeof globalThis !== 'undefined' ? globalThis : typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : this);
