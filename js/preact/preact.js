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
    var __mountedContainers = [];
    var __debugVNodeId = 1;
    var __debugComponentId = 1;

    function probeEnabled() {
        return !!global.__MBINK_LEAK_PROBE;
    }

    function probeLog(tag, payload) {
        if (!probeEnabled()) return;
        try {
            console.log('[LEAK_PROBE][' + tag + ']', JSON.stringify(payload || {}));
        } catch (_) {
            console.log('[LEAK_PROBE][' + tag + ']', payload || {});
        }
    }

    function ensureVNodeId(vnode) {
        if (!vnode || typeof vnode !== 'object') return null;
        if (!vnode.__debugId) {
            vnode.__debugId = 'v' + (__debugVNodeId++);
        }
        return vnode.__debugId;
    }

    function ensureComponentId(component) {
        if (!component) return null;
        if (!component.__debugId) {
            component.__debugId = 'c' + (__debugComponentId++);
        }
        return component.__debugId;
    }

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

    // Ensure props object exists
    var finalProps = props || {};
    
    // Add children to props for component access (standard Preact/React behavior)
    // This allows components to access children via props.children
    if (flatChildren.length > 0) {
        finalProps = {};
        for (var p in (props || {})) {
            finalProps[p] = props[p];
        }
        finalProps.children = flatChildren.length === 1 ? flatChildren[0] : flatChildren;
    }

    var vnode = {
        type: type,
        props: finalProps,
        children: flatChildren,
        key: props ? props.key : undefined,
        ref: props ? props.ref : undefined,
        __v: VNODE_TYPE_ELEMENT
    };

    ensureVNodeId(vnode);
    probeLog('preact.h', {
        vnodeId: vnode.__debugId,
        type: typeof type === 'function' ? (type.name || 'Anonymous') : String(type),
        childCount: flatChildren.length
    });
    return vnode;
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

function clearVNode(vnode) {
    if (!vnode || typeof vnode !== 'object') {
        return;
    }
    probeLog('preact.clearVNode', {
        vnodeId: ensureVNodeId(vnode),
        type: vnode.type ? (typeof vnode.type === 'function' ? (vnode.type.name || 'Anonymous') : String(vnode.type)) : null,
        hasDom: !!vnode.__dom,
        hasComponent: !!vnode.__component
    });
    if (Array.isArray(vnode)) {
        for (var i = 0; i < vnode.length; i++) {
            clearVNode(vnode[i]);
            vnode[i] = null;
        }
        return;
    }

    var component = vnode.__component;
    if (component) {
        if (typeof PreactHooks !== 'undefined' && PreactHooks.cleanupComponent) {
            PreactHooks.cleanupComponent(component);
        }
        if (component.__renderedVNode && component.__renderedVNode !== vnode) {
            clearVNode(component.__renderedVNode);
        }
        component.__dom = null;
        component.__rerender = null;
        component.__renderedVNode = null;
        component.__vnode = null;
        component.__hooks = [];
    }

    if (vnode.props) {
        var props = vnode.props;
        if (props.children) {
            clearVNode(props.children);
        }
        if (props.ref) {
            if (typeof props.ref === 'function') {
                try { props.ref(null); } catch (_) {}
            } else if (typeof props.ref === 'object') {
                props.ref.current = null;
            }
        }
        for (var key in props) {
            props[key] = null;
        }
        vnode.props = null;
    }

    if (vnode.children) {
        for (var j = 0; j < vnode.children.length; j++) {
            clearVNode(vnode.children[j]);
            vnode.children[j] = null;
        }
        vnode.children = null;
    }

    if (vnode.__dom) {
        releaseElementData(vnode.__dom);
    }

    vnode.__dom = null;
    vnode.__component = null;
    vnode.key = null;
    vnode.ref = null;
    vnode.type = null;
}

function detachVNodeGraph(vnode, preserveComponent) {
    if (!vnode || typeof vnode !== 'object') {
        return;
    }

    probeLog('preact.detachVNodeGraph', {
        vnodeId: ensureVNodeId(vnode),
        preserveComponent: !!preserveComponent,
        hasComponent: !!vnode.__component
    });

    if (Array.isArray(vnode)) {
        for (var i = 0; i < vnode.length; i++) {
            detachVNodeGraph(vnode[i], preserveComponent);
            vnode[i] = null;
        }
        return;
    }

    var component = vnode.__component;
    var ownsComponent = !!(component && component.__vnode === vnode);
    var shouldPreserveComponent = !!(preserveComponent && ownsComponent);

    if (ownsComponent && component.__renderedVNode && component.__renderedVNode !== vnode) {
        detachVNodeGraph(component.__renderedVNode, false);
        component.__renderedVNode = null;
    }

    if (vnode.props && vnode.props.children && vnode.props.children !== vnode.children) {
        detachVNodeGraph(vnode.props.children, false);
    }

    if (vnode.children) {
        for (var j = 0; j < vnode.children.length; j++) {
            detachVNodeGraph(vnode.children[j], false);
            vnode.children[j] = null;
        }
        vnode.children = null;
    }

    if (ownsComponent && !shouldPreserveComponent) {
        if (typeof PreactHooks !== 'undefined' && PreactHooks.cleanupComponent) {
            PreactHooks.cleanupComponent(component);
        }
        component.__dom = null;
        component.__rerender = null;
        component.__vnode = null;
        component.__hooks = [];
    }

    vnode.props = null;
    vnode.__dom = null;
    if (!shouldPreserveComponent) {
        vnode.__component = null;
    }
    vnode.key = null;
    vnode.ref = null;
    vnode.type = null;
}


/**
 * Render a VNode tree into a DOM container
 * @param {object} vnode - Virtual DOM node
 * @param {Element} container - DOM container element
 */
function render(vnode, container) {
    if (__mountedContainers.indexOf(container) === -1) {
        __mountedContainers.push(container);
    }

    if (typeof globalThis !== 'undefined' && typeof globalThis.__mbinkRegisterPreactRoot === 'function') {
        globalThis.__mbinkRegisterPreactRoot(vnode, container, render);
    }

    var trackedRoot = container && container.__preactRoot ? container.__preactRoot : null;

    if (trackedRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkBeginRootTracking === 'function') {
        globalThis.__mbinkBeginRootTracking(trackedRoot);
    }

    try {
        // Use C++ implementation if available
        if (typeof __preact_internal !== 'undefined' && __preact_internal.render) {
            return __preact_internal.render(vnode, container);
        }

        // 开始批量更新，避免每次 DOM 操作都触发重绘
        if (typeof document !== 'undefined' && typeof document.__beginBatch === 'function') {
            document.__beginBatch();
        }

        try {
            // Get old vnode from container
            var oldVNode = container.__preactVNode;
            var oldDOM = container.__preactDOM;
            probeLog('preact.render', {
                oldVNodeId: ensureVNodeId(oldVNode),
                newVNodeId: ensureVNodeId(vnode),
                mountedContainers: __mountedContainers.length
            });

            // Diff and patch
            var newDOM = diffNode(oldVNode, vnode, container, oldDOM);

            // Store references
            container.__preactVNode = vnode;
            container.__preactDOM = newDOM;

            if (oldVNode && oldVNode !== vnode) {
                detachVNodeGraph(oldVNode, false);
            }
        } finally {
            // 结束批量更新，触发一次性重绘
            if (typeof document !== 'undefined' && typeof document.__endBatch === 'function') {
                document.__endBatch();
            }
        }
    } finally {
        if (trackedRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkEndRootTracking === 'function') {
            globalThis.__mbinkEndRootTracking(trackedRoot);
        }
    }
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
        var textNode = document.createTextNode(String(vnode));
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
            __renderedVNode: null,
            __rerender: null
        };
        probeLog('preact.createComponent', {
            vnodeId: ensureVNodeId(vnode)
        });
    }

    var component = vnode.__component;
    ensureComponentId(component);
    component.__vnode = vnode;
    probeLog('preact.bindComponent', {
        componentId: component.__debugId,
        vnodeId: ensureVNodeId(vnode)
    });

    // Set up rerender function using Virtual DOM diffing
    if (!component.__rerender) {
        component.__rerender = function () {
            try {
                if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                    PreactHooks.setCurrentComponent(component);
                }

                var currentVNode = component.__vnode;
                var newRenderedVNode = currentVNode.type(currentVNode.props);

                if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                    PreactHooks.setCurrentComponent(null);
                }

                var oldDOM = component.__dom;
                if (!oldDOM || !oldDOM.parentNode) {
                    return;
                }

                var parent = oldDOM.parentNode;
                var oldRenderedVNode = component.__renderedVNode;
                var newDOM = diffNode(oldRenderedVNode, newRenderedVNode, parent, oldDOM);
                probeLog('preact.componentRerender', {
                    componentId: component.__debugId,
                    oldRenderedVNodeId: ensureVNodeId(oldRenderedVNode),
                    newRenderedVNodeId: ensureVNodeId(newRenderedVNode)
                });

                component.__dom = newDOM;
                component.__renderedVNode = newRenderedVNode;

                if (oldRenderedVNode && oldRenderedVNode !== newRenderedVNode) {
                    detachVNodeGraph(oldRenderedVNode, false);
                }
            } catch (e) {}
        };
    }

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

    return dom;
}

function getVNodeDOM(vnode) {
    if (vnode == null || vnode === false || vnode === true) {
        return null;
    }

    if (typeof vnode === 'string' || typeof vnode === 'number') {
        return null;
    }

    if (Array.isArray(vnode)) {
        for (var i = 0; i < vnode.length; i++) {
            var arrayDOM = getVNodeDOM(vnode[i]);
            if (arrayDOM) {
                return arrayDOM;
            }
        }
        return null;
    }

    if (vnode.__dom) {
        return vnode.__dom;
    }

    if (vnode.__component) {
        if (vnode.__component.__dom) {
            return vnode.__component.__dom;
        }
        if (vnode.__component.__renderedVNode) {
            return getVNodeDOM(vnode.__component.__renderedVNode);
        }
    }

    if (vnode.children && vnode.children.length) {
        for (var j = 0; j < vnode.children.length; j++) {
            var childDOM = getVNodeDOM(vnode.children[j]);
            if (childDOM) {
                return childDOM;
            }
        }
    }

    if (vnode.props && vnode.props.children) {
        return getVNodeDOM(vnode.props.children);
    }

    return null;
}

/**
 * Global storage for DOM element data - avoids setting custom properties on DOM elements
 * which can cause issues with QuickJS
 */
var __elementDataStore = {};
var __elementIdCounter = 1;

function releaseElementData(element) {
    if (!element || typeof element.getAttribute !== 'function') {
        return;
    }

    var id = element.getAttribute('data-preact-id');
    if (!id) {
        return;
    }

    var data = __elementDataStore[id];
    if (!data) {
        try { element.removeAttribute('data-preact-id'); } catch (_) {}
        return;
    }

    if (data.listeners) {
        for (var k in data.listeners) {
            var stable = data.listeners[k];
            if (typeof stable !== 'function') continue;

            var eventName = null;
            if (k.length > 6 && k.substring(k.length - 6) === '_input') {
                eventName = 'input';
            } else if (k.length > 2 && k.substring(0, 2) === 'on') {
                eventName = k.substring(2).toLowerCase();
            }

            if (eventName) {
                try { element.removeEventListener(eventName, stable); } catch (_) {}
            }

            delete data.listeners[k];
        }
    }

    if (data.handlers) {
        for (var hk in data.handlers) {
            delete data.handlers[hk];
        }
    }

    if (data.vnode) {
        data.vnode = null;
    }

    if (data.element) {
        try { data.element.removeAttribute('data-preact-id'); } catch (_) {}
        data.element = null;
    } else {
        try { element.removeAttribute('data-preact-id'); } catch (_) {}
    }

    delete __elementDataStore[id];
}

function getElementId(element) {
    // Use a data attribute to store element ID (safer than custom JS property)
    var id = element.getAttribute('data-preact-id');
    if (!id) {
        id = String(__elementIdCounter++);
        element.setAttribute('data-preact-id', id);
        __elementDataStore[id] = { element: element, listeners: {}, handlers: {} };
    }
    return id;
}

function getElementData(element) {
    var id = getElementId(element);
    if (!__elementDataStore[id]) {
        __elementDataStore[id] = { element: element, listeners: {}, handlers: {} };
    } else if (!__elementDataStore[id].element) {
        __elementDataStore[id].element = element;
    }
    return __elementDataStore[id];
}

function setElementVNode(element, vnode) {
    var data = getElementData(element);
    data.vnode = vnode || null;
}

function getElementVNode(element) {
    var data = getElementData(element);
    return data.vnode || null;
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
            console.log('[setDOMProps] Setting value on', element.tagName, 'from', element.value, 'to', newValue);
            if (element.value !== String(newValue)) {
                element.value = newValue;
                console.log('[setDOMProps] After set, element.value =', element.value);
            }
        } else if (newValue === oldValue) {
            // Skip unchanged non-event props
            continue;
        } else if (prop === 'className') {
            element.className = newValue || '';
        } else if (prop === 'style') {
            if (typeof newValue === 'string') {
                // 优化：只在字符串值真正变化时才设置
                if (element.style.cssText !== newValue) {
                    element.style.cssText = newValue;
                }
            } else if (typeof newValue === 'object') {
                // Clear old styles first if old value was also object
                if (typeof oldValue === 'object' && oldValue) {
                    for (var oldStyle in oldValue) {
                        if (!(oldStyle in newValue)) {
                            element.style[oldStyle] = '';
                        }
                    }
                }
                // 优化：只在属性值真正变化时才设置
                // 这避免了不必要的 DOM 更新和重绘
                for (var styleProp in newValue) {
                    var newStyleValue = newValue[styleProp];
                    var oldStyleValue = oldValue && oldValue[styleProp];
                    if (newStyleValue !== oldStyleValue) {
                        element.style[styleProp] = newStyleValue;
                    }
                }
            }
        } else if (prop === 'contentEditable') {
            // contentEditable 需要设置为字符串 "true" 或 "false"
            element.setAttribute('contenteditable', newValue === true ? 'true' : String(newValue));
        } else if (typeof newValue === 'boolean') {
            if (newValue) {
                element.setAttribute(prop, '');
            } else {
                element.removeAttribute(prop);
            }
        } else if (prop === 'dangerouslySetInnerHTML') {
            // Handle React/Preact's dangerouslySetInnerHTML
            if (newValue && newValue.__html != null) {
                element.innerHTML = newValue.__html;
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
    try {
        // New node is null - remove old
        if (newVNode == null || newVNode === false || newVNode === true) {
            probeLog('preact.diffNode.remove', {
                oldVNodeId: ensureVNodeId(oldVNode),
                hasOldDOM: !!oldDOM,
                parentTag: parentDOM && parentDOM.tagName ? parentDOM.tagName : null
            });
            if (oldVNode) {
                clearVNode(oldVNode);
            }
            if (oldDOM && parentDOM && oldDOM.parentNode === parentDOM) {
                try {
                    parentDOM.removeChild(oldDOM);
                } catch (e) {
                    // Ignore removal errors
                }
            }
            return null;
        }

        // Old node is null - create new
        if (oldVNode == null || oldVNode === false || oldVNode === true) {
            var newDOM = createDOMElement(newVNode);
            if (newDOM && parentDOM) {
                parentDOM.appendChild(newDOM);
            }
            return newDOM;
        }

        // Both are text nodes
        if ((typeof oldVNode === 'string' || typeof oldVNode === 'number') &&
            (typeof newVNode === 'string' || typeof newVNode === 'number')) {
            if (!oldDOM) {
                var createdTextDOM = createDOMElement(newVNode);
                if (createdTextDOM && parentDOM) {
                    parentDOM.appendChild(createdTextDOM);
                }
                return createdTextDOM;
            }
            if (String(oldVNode) !== String(newVNode)) {
                oldDOM.textContent = String(newVNode);
            }
            return oldDOM;
        }

        // Type changed - replace entirely
        if (!isSameVNodeType(oldVNode, newVNode)) {
            clearVNode(oldVNode);
            var replacementDOM = createDOMElement(newVNode);
            if (parentDOM && oldDOM && oldDOM.parentNode === parentDOM) {
                try {
                    parentDOM.replaceChild(replacementDOM, oldDOM);
                } catch (e) {
                    // If replace fails, try append
                    parentDOM.appendChild(replacementDOM);
                }
            } else if (parentDOM && replacementDOM) {
                parentDOM.appendChild(replacementDOM);
            }
            return replacementDOM;
        }

        // Both are components of the same type
        if (typeof newVNode.type === 'function') {
            // Transfer component reference from old to new VNode
            if (oldVNode.__component && !newVNode.__component) {
                newVNode.__component = oldVNode.__component;
            }
            return diffComponent(oldVNode, newVNode, parentDOM, oldDOM);
        }

        if (!oldDOM) {
            var recreatedDOM = createDOMElement(newVNode);
            if (recreatedDOM && parentDOM) {
                parentDOM.appendChild(recreatedDOM);
            }
            return recreatedDOM;
        }

        // Both are elements of the same type - update in place
        return diffElement(oldVNode, newVNode, oldDOM);
    } catch (e) {
        // On error, try to create new element as fallback
        try {
            var fallbackDOM = createDOMElement(newVNode);
            if (fallbackDOM && parentDOM) {
                parentDOM.appendChild(fallbackDOM);
            }
            return fallbackDOM;
        } catch (e2) {
            return oldDOM;
        }
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
    ensureComponentId(component);
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
    probeLog('preact.diffComponent', {
        componentName: componentName,
        componentId: component.__debugId,
        oldVNodeId: ensureVNodeId(oldVNode),
        newVNodeId: ensureVNodeId(newVNode),
        oldRenderedVNodeId: ensureVNodeId(oldRenderedVNode),
        newRenderedVNodeId: ensureVNodeId(newRenderedVNode)
    });

    // Update component
    component.__dom = newDOM;
    component.__renderedVNode = newRenderedVNode;

    if (oldRenderedVNode && oldRenderedVNode !== newRenderedVNode) {
        detachVNodeGraph(oldRenderedVNode, false);
    }

    if (oldVNode && oldVNode !== newVNode) {
        detachVNodeGraph(oldVNode, false);
    }

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
        return;
    }

    oldChildren = oldChildren || [];
    newChildren = newChildren || [];

    var oldLen = oldChildren.length;
    var newLen = newChildren.length;

    // Safety check: if parentDOM has no childNodes property, skip
    if (!parentDOM.childNodes) {
        for (var n = 0; n < newLen; n++) {
            var dom = createDOMElement(newChildren[n]);
            if (dom) {
                parentDOM.appendChild(dom);
            }
        }
        return;
    }

    // Build a map of old children by key
    var oldKeyedMap = {};
    var oldUnkeyed = [];
    var oldEntries = [];

    // Get current DOM children as static array (snapshot)
    var childNodesArray = [];
    var childNodes = parentDOM.childNodes;
    var childNodesLen = childNodes.length;
    for (var k = 0; k < childNodesLen; k++) {
        childNodesArray.push(childNodes[k]);
    }

    var domCursor = 0;

    // Map old children using vnode-owned DOM first, then fallback to DOM snapshot cursor.
    // This keeps null-rendering components in the old children list so they can still be reused.
    for (var i = 0; i < oldLen; i++) {
        var oldChild = oldChildren[i];
        var oldDOM = getVNodeDOM(oldChild);

        if (!oldDOM) {
            var isPrimitiveChild = typeof oldChild === 'string' || typeof oldChild === 'number';
            var isElementVNode = oldChild && typeof oldChild === 'object' && typeof oldChild.type === 'string';
            if (isPrimitiveChild || isElementVNode) {
                oldDOM = childNodesArray[domCursor] || null;
            }
        }

        if (oldDOM && childNodesArray[domCursor] === oldDOM) {
            domCursor++;
        }

        oldEntries[i] = { vnode: oldChild, dom: oldDOM, index: i };
        var key = getKey(oldChild, null);

        if (key != null) {
            oldKeyedMap[key] = oldEntries[i];
        } else {
            oldUnkeyed.push(oldEntries[i]);
        }
    }

    var unkeyedIndex = 0;
    var usedOldDOMs = {};

    // Process new children
    for (var j = 0; j < newLen; j++) {
        var newChild = newChildren[j];
        var newKey = getKey(newChild, null);
        var oldEntry = null;
        var matchedOldDOM = null;

        if (newKey != null && oldKeyedMap[newKey]) {
            oldEntry = oldKeyedMap[newKey];
            matchedOldDOM = oldEntry.dom;
            usedOldDOMs[oldEntry.index] = true;
        } else if (newKey == null && unkeyedIndex < oldUnkeyed.length) {
            oldEntry = oldUnkeyed[unkeyedIndex++];
            matchedOldDOM = oldEntry.dom;
            usedOldDOMs[oldEntry.index] = true;
        }

        var currentDOMAtPosition = parentDOM.childNodes[j];

        if (oldEntry) {
            // Diff with matched old vnode - even function components without direct DOM must be reused.
            var updatedDOM = null;
            try {
                updatedDOM = diffNode(oldEntry.vnode, newChild, parentDOM, matchedOldDOM);
            } catch (e) {
                // If diff fails, create new element
                var recoveryDOM = createDOMElement(newChild);
                if (recoveryDOM) {
                    if (matchedOldDOM && matchedOldDOM.parentNode === parentDOM) {
                        parentDOM.replaceChild(recoveryDOM, matchedOldDOM);
                    } else if (currentDOMAtPosition) {
                        parentDOM.insertBefore(recoveryDOM, currentDOMAtPosition);
                    } else {
                        parentDOM.appendChild(recoveryDOM);
                    }
                }
                continue;
            }

            var domToPlace = updatedDOM || matchedOldDOM;

            // Move DOM to correct position if needed
            if (domToPlace && domToPlace !== currentDOMAtPosition && domToPlace.parentNode === parentDOM) {
                if (currentDOMAtPosition) {
                    parentDOM.insertBefore(domToPlace, currentDOMAtPosition);
                } else {
                    parentDOM.appendChild(domToPlace);
                }
            }
        } else {
            // No matching old vnode, create new
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
            var oldEntryToRemove = oldEntries[m];
            var oldChildVNode = oldEntryToRemove ? oldEntryToRemove.vnode : oldChildren[m];
            if (oldChildVNode) {
                clearVNode(oldChildVNode);
            }
            var domToRemove = oldEntryToRemove ? oldEntryToRemove.dom : null;
            try {
                if (domToRemove && domToRemove.parentNode === parentDOM) {
                    parentDOM.removeChild(domToRemove);
                }
            } catch (e) {
                // Ignore removal errors
            }
        }
    }
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

    /**
     * Create a Context object for passing data through the component tree
     * @param {any} defaultValue - Default value when no Provider is found
     * @returns {object} Context object with Provider and Consumer
     */
    function createContext(defaultValue) {
        var context = {
            __defaultValue: defaultValue,
            __currentValue: defaultValue,
            __listeners: []
        };

        /**
         * Provider component - provides value to descendants
         */
        function Provider(props) {
            // Update current value
            var value = props.value !== undefined ? props.value : defaultValue;
            context.__currentValue = value;

            // Notify listeners (for future use with useContext)
            for (var i = 0; i < context.__listeners.length; i++) {
                context.__listeners[i](value);
            }

            // Just render children
            return props.children;
        }

        /**
         * Consumer component - consumes context value via render prop
         */
        function Consumer(props) {
            var children = props.children;
            if (typeof children === 'function') {
                return children(context.__currentValue);
            }
            return children;
        }

        // Attach Provider and Consumer to context
        context.Provider = Provider;
        context.Consumer = Consumer;

        // For useContext hook support
        context.__getValue = function() {
            return context.__currentValue;
        };

        context.__subscribe = function(listener) {
            context.__listeners.push(listener);
            return function() {
                var idx = context.__listeners.indexOf(listener);
                if (idx > -1) {
                    context.__listeners.splice(idx, 1);
                }
            };
        };

        return context;
    }

    // Export all APIs as global object (for script loading)
    var Preact = {
        h: h,
        createElement: createElement,
        render: render,
        Fragment: Fragment,
        Component: Component,
        createRef: createRef,
        cloneElement: cloneElement,
        isValidElement: isValidElement,
        createContext: createContext
    };

    // 添加小写别名以提高兼容性
    var preact = Preact;

    // 暴露到全局作用域
    global.Preact = Preact;
    global.preact = preact;
    // 暴露清理函数，供 C++ 关闭时调用以释放 IIFE 内部的函数引用
    // __elementDataStore 是 IIFE 局部变量，外部无法直接访问
    global.__preactCleanup = function() {
        for (var i = 0; i < __mountedContainers.length; i++) {
            var container = __mountedContainers[i];
            if (!container) continue;
            try {
                if (container.__preactVNode) {
                    clearVNode(container.__preactVNode);
                }
                container.__preactVNode = null;
                container.__preactDOM = null;
            } catch (_) {}
        }
        __mountedContainers.length = 0;

        for (var id in __elementDataStore) {
            var data = __elementDataStore[id];
            if (data) {
                // 先从 DOM 上移除稳定监听器（关键：触发 C++ RemoveEventListener 释放 JSValue）
                if (data.element && data.listeners) {
                    for (var k in data.listeners) {
                        var stable = data.listeners[k];
                        if (typeof stable !== 'function') continue;

                        // _input 后缀必须优先判断，否则会被前面的 onXxx 分支吞掉
                        var eventName = null;
                        if (k.length > 6 && k.substring(k.length - 6) === '_input') {
                            eventName = 'input';
                        } else if (k.length > 2 && k.substring(0, 2) === 'on') {
                            eventName = k.substring(2).toLowerCase();
                        }

                        if (eventName) {
                            try { data.element.removeEventListener(eventName, stable); } catch (_) {}
                        }

                        delete data.listeners[k];
                    }
                } else if (data.listeners) {
                    for (var lk in data.listeners) { delete data.listeners[lk]; }
                }

                if (data.handlers) {
                    for (var hk in data.handlers) { delete data.handlers[hk]; }
                }

                // 删除回指，打断 element <-> store 的引用链
                if (data.element) {
                    try { data.element.removeAttribute('data-preact-id'); } catch (_) {}
                    data.element = null;
                }
                if (data.vnode) {
                    clearVNode(data.vnode);
                    data.vnode = null;
                }
            }
            delete __elementDataStore[id];
        }
    };

    // Note: For ES6 module usage, use js/preact/preact.mjs

})(typeof globalThis !== 'undefined' ? globalThis : typeof window !== 'undefined' ? window : typeof global !== 'undefined' ? global : this);
