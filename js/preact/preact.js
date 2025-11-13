/**
 * Preact Core - Simplified version for MBink
 * Based on Preact 10.x API
 * 
 * This is a lightweight implementation optimized for MBink's QuickJS runtime.
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

    // Fallback to JavaScript implementation
    if (container.__preactRoot) {
        // Update existing tree
        diff(container.__preactRoot, vnode, container);
    } else {
        // Initial render
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
        return document.createTextNode(String(vnode));
    }
    
    // Handle component functions
    if (typeof vnode.type === 'function') {
        // Create component instance for hooks
        if (!vnode.__component) {
            vnode.__component = {
                __hooks: [],
                __vnode: vnode,
                __dom: null,  // Will store the rendered DOM element
                __rerender: null  // Will be set below
            };
        }

        const component = vnode.__component;

        // Set up rerender function
        component.__rerender = function() {
            // Set current component for hooks
            if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                PreactHooks.setCurrentComponent(component);
            }

            // Call component function to get new VNode
            const newVNode = vnode.type(vnode.props);

            // Clear current component
            if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                PreactHooks.setCurrentComponent(null);
            }

            // Get old DOM and parent
            const oldDOM = component.__dom;
            if (!oldDOM) {
                console.log('[Preact] __rerender: oldDOM is null');
                return;
            }

            if (!oldDOM.parentNode) {
                console.log('[Preact] __rerender: oldDOM.parentNode is null');
                return;
            }

            const parent = oldDOM.parentNode;
            console.log('[Preact] __rerender: parent tag=' + parent.tagName);

            // Create new DOM
            const newDOM = createDOMElement(newVNode);

            if (newDOM) {
                console.log('[Preact] __rerender: About to replaceChild, old=' + oldDOM.tagName + ', new=' + newDOM.tagName);

                try {
                    // Replace old DOM with new DOM
                    parent.replaceChild(newDOM, oldDOM);
                    console.log('[Preact] __rerender: replaceChild succeeded');
                } catch (e) {
                    console.log('[Preact] __rerender: replaceChild failed: ' + e);
                    return;
                }

                // Update component state
                component.__dom = newDOM;
                component.__renderedVNode = newVNode;

                // Store reference to the component VNode on the new DOM
                newDOM.__componentVNode = vnode;

                console.log('[Preact] __rerender: Component state updated');
            } else {
                console.log('[Preact] __rerender: newDOM is null');
            }
        };

        // Set current component for hooks
        if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
            PreactHooks.setCurrentComponent(component);
        }

        // Call component function
        const componentVNode = vnode.type(vnode.props);

        // Clear current component
        if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
            PreactHooks.setCurrentComponent(null);
        }

        // Create DOM element
        const dom = createDOMElement(componentVNode);

        // Store DOM reference and rendered VNode
        component.__dom = dom;
        component.__renderedVNode = componentVNode;

        // Store reference to the component VNode on the DOM
        if (dom) {
            dom.__componentVNode = vnode;
        }

        return dom;
    }
    
    // Handle Fragment
    if (vnode.type === Fragment) {
        const fragment = document.createDocumentFragment();
        for (let child of vnode.children) {
            const childDOM = createDOMElement(child);
            if (childDOM) {
                fragment.appendChild(childDOM);
            }
        }
        return fragment;
    }
    
    // Handle regular elements
    const element = document.createElement(vnode.type);
    
    // Set properties
    if (vnode.props) {
        for (let [key, value] of Object.entries(vnode.props)) {
            if (key === 'key' || key === 'ref' || key === 'children') {
                continue;
            }
            
            // Handle event listeners
            if (key.startsWith('on') && typeof value === 'function') {
                const eventName = key.substring(2).toLowerCase();
                element.addEventListener(eventName, value);
            }
            // Handle className
            else if (key === 'className') {
                element.className = value;
            }
            // Handle style object
            else if (key === 'style' && typeof value === 'object') {
                for (let [styleProp, styleValue] of Object.entries(value)) {
                    element.style[styleProp] = styleValue;
                }
            }
            // Handle boolean attributes
            else if (typeof value === 'boolean') {
                if (value) {
                    element.setAttribute(key, '');
                }
            }
            // Handle regular attributes
            else if (value != null) {
                element.setAttribute(key, String(value));
            }
        }
    }
    
    // Append children
    if (vnode.children) {
        for (let child of vnode.children) {
            const childDOM = createDOMElement(child);
            if (childDOM) {
                element.appendChild(childDOM);
            }
        }
    }
    
    // Store VNode reference
    element.__vnode = vnode;
    
    // Handle ref
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
 * Diff and update DOM based on VNode changes
 * @param {object} oldVNode - Old Virtual DOM node
 * @param {object} newVNode - New Virtual DOM node
 * @param {Element} container - DOM container
 */
function diff(oldVNode, newVNode, container) {
    // Simple diff implementation
    // For now, just re-render (can be optimized later)
    container.innerHTML = '';
    const dom = createDOMElement(newVNode);
    if (dom) {
        container.appendChild(dom);
    }
    container.__preactRoot = newVNode;
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

// Export all APIs as global object (QuickJS compatible)
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

