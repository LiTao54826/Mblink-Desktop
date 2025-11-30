/**
 * Preact Core - Simplified version for MBink
 * Based on Preact 10.x API
 *
 * This is a lightweight implementation optimized for MBink's QuickJS runtime.
 * Implements real Virtual DOM diffing for efficient updates.
 */

// VNode type constants
const VNODE_TYPE_ELEMENT = 1;
const VNODE_TYPE_TEXT = 2;
const VNODE_TYPE_COMPONENT = 3;

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
const createElement = h;

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

    console.log('[Preact render] Starting render');

    // Get old vnode from container
    var oldVNode = container.__preactVNode;
    var oldDOM = container.__preactDOM;

    // Diff and patch
    var newDOM = diffNode(oldVNode, vnode, container, oldDOM);

    // Store references
    container.__preactVNode = vnode;
    container.__preactDOM = newDOM;

    console.log('[Preact render] Render complete');
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
    var element = document.createElement(vnode.type);

    // Set properties
    setDOMProps(element, {}, vnode.props || {});

    // Append children
    if (vnode.children) {
        for (var j = 0; j < vnode.children.length; j++) {
            var child = vnode.children[j];
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
    component.__rerender = function() {
        console.log('[Preact __rerender] Starting rerender');

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
            if (!oldDOM) {
                console.log('[Preact __rerender] oldDOM is null');
                return;
            }
            if (!oldDOM.parentNode) {
                console.log('[Preact __rerender] oldDOM.parentNode is null');
                return;
            }

            var parent = oldDOM.parentNode;
            var oldRenderedVNode = component.__renderedVNode;

            console.log('[Preact __rerender] Calling diffNode');

            // Use Virtual DOM diffing to update in place
            var newDOM = diffNode(oldRenderedVNode, newRenderedVNode, parent, oldDOM);

            // Update component state
            component.__dom = newDOM;
            component.__renderedVNode = newRenderedVNode;
            if (newDOM) {
                newDOM.__componentVNode = vnode;
            }

            console.log('[Preact __rerender] Rerender complete');
        } catch (e) {
            console.log('[Preact __rerender] Error: ' + e.message);
            console.log('[Preact __rerender] Stack: ' + e.stack);
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
    return function(event) {
        console.log('[StableHandler] Called for elementId=' + elementId + ', eventKey=' + eventKey);
        var data = __elementDataStore[elementId];
        console.log('[StableHandler] data exists=' + !!data);
        if (data && data.handlers) {
            console.log('[StableHandler] handlers keys=' + Object.keys(data.handlers).join(','));
            console.log('[StableHandler] handler exists=' + !!data.handlers[eventKey]);
        }
        if (data && data.handlers && data.handlers[eventKey]) {
            console.log('[StableHandler] Calling handler for ' + eventKey);
            data.handlers[eventKey](event);
            console.log('[StableHandler] Handler completed');
        } else {
            console.log('[StableHandler] WARNING: No handler found!');
        }
    };
}

/**
 * Set DOM properties, handling events, styles, etc.
 * Uses stable event handler wrappers to avoid add/remove listener on every render
 */
function setDOMProps(element, oldProps, newProps) {
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

        // Debug: log value prop handling
        if (prop === 'value') {
            console.log('[setDOMProps] DEBUG: prop=value, tagName=' + element.tagName + ', newValue=' + newValue + ', oldValue=' + oldValue);
        }

        // Debug: log style prop handling
        if (prop === 'style') {
            console.log('[setDOMProps] DEBUG: prop=style, oldValue=' + oldValue + ', newValue=' + newValue);
        }

        if (prop.substring(0, 2) === 'on' && typeof newValue === 'function') {
            var evtName = prop.substring(2).toLowerCase();

            // Always update the handler reference (so latest closure is called)
            console.log('[setDOMProps] Updating handler for ' + prop + ' on element ' + elementId);
            handlers[prop] = newValue;
            if (evtName === 'change') {
                handlers[prop + '_input'] = newValue;
            }

            // Only add listener if not already added
            if (!listeners[prop]) {
                console.log('[setDOMProps] Adding NEW listener for ' + evtName + ' on element ' + elementId);
                // Create stable wrapper and add listener
                var stableHandler = createStableHandler(elementId, prop);
                var listenerId = element.addEventListener(evtName, stableHandler);
                console.log('[setDOMProps] Got listenerId=' + listenerId);
                listeners[prop] = listenerId;

                if (evtName === 'change') {
                    var stableInputHandler = createStableHandler(elementId, prop + '_input');
                    var inputListenerId = element.addEventListener('input', stableInputHandler);
                    listeners[prop + '_input'] = inputListenerId;
                }
            }
        } else if (prop === 'value' && (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA')) {
            // For controlled inputs, always check against current DOM value
            // Don't skip based on oldValue because DOM value can be changed by user input
            console.log('[setDOMProps] value prop: newValue=' + newValue + ' element.value=' + element.value);
            if (element.value !== String(newValue)) {
                console.log('[setDOMProps] Setting element.value to ' + newValue);
                element.value = newValue;
            }
        } else if (newValue === oldValue) {
            // Skip unchanged non-event props
            continue;
        } else if (prop === 'className') {
            element.className = newValue || '';
        } else if (prop === 'style') {
            console.log('[setDOMProps] SETTING style, typeof newValue=' + typeof newValue);
            if (typeof newValue === 'string') {
                console.log('[setDOMProps] Setting cssText to: ' + newValue);
                element.style.cssText = newValue;
                console.log('[setDOMProps] After setting, cssText=' + element.style.cssText);
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
    console.log('[diffNode] old=' + oldType + ', new=' + newType);

    try {
        // New node is null - remove old
        if (newVNode == null || newVNode === false || newVNode === true) {
            console.log('[diffNode] newVNode is null/false/true, removing old');
            if (oldDOM && parentDOM) {
                parentDOM.removeChild(oldDOM);
            }
            return null;
        }

        // Old node is null - create new
        if (oldVNode == null || oldVNode === false || oldVNode === true || !oldDOM) {
            console.log('[diffNode] oldVNode is null, creating new');
            var newDOM = createDOMElement(newVNode);
            if (newDOM && parentDOM) {
                parentDOM.appendChild(newDOM);
            }
            return newDOM;
        }

        // Both are text nodes
        if ((typeof oldVNode === 'string' || typeof oldVNode === 'number') &&
            (typeof newVNode === 'string' || typeof newVNode === 'number')) {
            console.log('[diffNode] Both text nodes, old="' + oldVNode + '" new="' + newVNode + '"');
            if (String(oldVNode) !== String(newVNode)) {
                console.log('[diffNode] Text changed, updating DOM');
                oldDOM.textContent = String(newVNode);
            }
            return oldDOM;
        }

        // Type changed - replace entirely
        if (!isSameVNodeType(oldVNode, newVNode)) {
            console.log('[diffNode] Type changed, replacing');
            var replacementDOM = createDOMElement(newVNode);
            if (parentDOM && oldDOM) {
                parentDOM.replaceChild(replacementDOM, oldDOM);
            }
            return replacementDOM;
        }

        // Both are components of the same type
        if (typeof newVNode.type === 'function') {
            console.log('[diffNode] Both components, calling diffComponent');
            // IMPORTANT: Transfer component reference from old to new VNode
            // This is necessary because newVNode is freshly created and doesn't have __component
            if (oldVNode.__component && !newVNode.__component) {
                newVNode.__component = oldVNode.__component;
            }
            return diffComponent(oldVNode, newVNode, parentDOM, oldDOM);
        }

        // Both are elements of the same type - update in place
        console.log('[diffNode] Both elements of type ' + newVNode.type + ', calling diffElement');
        return diffElement(oldVNode, newVNode, oldDOM);
    } catch (e) {
        console.log('[diffNode] Error: ' + e.message);
        throw e;
    }
}

/**
 * Diff component VNodes
 */
function diffComponent(oldVNode, newVNode, parentDOM, oldDOM) {
    var componentName = newVNode.type.name || 'Anonymous';
    console.log('[diffComponent] Component: ' + componentName);

    // Reuse the component instance
    var component = oldVNode.__component;
    if (!component) {
        // No old component, create new
        console.log('[diffComponent] No old component, creating new');
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
    console.log('[diffComponent] ' + componentName + ' old props keys: ' + Object.keys(oldVNode.props || {}).join(','));
    console.log('[diffComponent] ' + componentName + ' new props keys: ' + Object.keys(newVNode.props || {}).join(','));

    // Get new rendered VNode
    var newRenderedVNode = newVNode.type(newVNode.props);

    // Clear current component
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(null);
    }

    var oldRenderedVNode = component.__renderedVNode;
    console.log('[diffComponent] ' + componentName + ' calling diffNode on rendered output');

    // Diff the rendered output
    var newDOM = diffNode(oldRenderedVNode, newRenderedVNode, parentDOM, oldDOM);

    // Update component
    component.__dom = newDOM;
    component.__renderedVNode = newRenderedVNode;

    // Update rerender function - use component.__vnode to get latest props
    component.__rerender = function() {
        console.log('[Preact __rerender] Starting rerender');
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
            console.log('[Preact __rerender] No DOM or parent, skipping');
            return;
        }

        console.log('[Preact __rerender] Calling diffNode');
        var resultDOM = diffNode(component.__renderedVNode, updatedVNode, currentDOM.parentNode, currentDOM);
        component.__dom = resultDOM;
        component.__renderedVNode = updatedVNode;
        console.log('[Preact __rerender] Rerender complete');
    };

    return newDOM;
}

/**
 * Diff element VNodes (same tag type)
 */
function diffElement(oldVNode, newVNode, dom) {
    var tagName = dom && dom.tagName ? dom.tagName.toLowerCase() : 'unknown';
    console.log('[diffElement] Updating ' + tagName + ' element');

    // Update props - event listeners use stable wrappers so we can update handlers safely
    setDOMProps(dom, oldVNode.props || {}, newVNode.props || {});

    // Diff children
    var oldChildCount = oldVNode.children ? oldVNode.children.length : 0;
    var newChildCount = newVNode.children ? newVNode.children.length : 0;
    console.log('[diffElement] ' + tagName + ' has ' + oldChildCount + ' old children, ' + newChildCount + ' new children');
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

    console.log('[diffElement] Done updating ' + tagName);
    return dom;
}

/**
 * Diff children arrays using key-based algorithm
 */
function diffChildren(oldChildren, newChildren, parentDOM) {
    if (!parentDOM) {
        console.log('[diffChildren] No parentDOM, returning');
        return;
    }

    oldChildren = oldChildren || [];
    newChildren = newChildren || [];

    var oldLen = oldChildren.length;
    var newLen = newChildren.length;

    console.log('[diffChildren] oldLen=' + oldLen + ', newLen=' + newLen);

    // Build a map of old children by key
    var oldKeyedMap = {};  // key -> { vnode, dom, index }
    var oldUnkeyed = [];   // [{ vnode, dom, index }]

    // Get current DOM children as static array
    var childNodesArray = [];
    var childNodes = parentDOM.childNodes;
    for (var k = 0; k < childNodes.length; k++) {
        childNodesArray.push(childNodes[k]);
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
            console.log('[diffChildren] Matched key=' + newKey + ' at oldIndex=' + oldEntry.index);
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
            console.log('[diffChildren] Creating new child at ' + j);
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
                console.log('[diffChildren] Removing unused child at ' + m);
                parentDOM.removeChild(domToRemove);
            }
        }
    }

    console.log('[diffChildren] Done, final childCount=' + parentDOM.childNodes.length);
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
function cloneElement(vnode, props, ...children) {
    return {
        ...vnode,
        props: { ...vnode.props, ...props },
        children: children.length > 0 ? children : vnode.children
    };
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
class Component {
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
        
        // Trigger re-render
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

// Note: For ES6 module usage, use js/preact/preact.mjs
