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
        __v: VNODE_TYPE_ELEMENT,
        __dom: null,
        __component: null,
        __parentVNode: null,
        __index: 0,
        __root: null
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
function getKey(vnode) {
    if (vnode == null) return null;
    if (typeof vnode === 'string' || typeof vnode === 'number') return null;
    return vnode.key != null ? vnode.key : null;
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
     * Global options/hooks system for plugins and debugging
     * Based on official Preact options system
     */
    var options = {
        // Lifecycle hooks
        _diff: null,        // Called before diff
        _render: null,      // Called before render
        diffed: null,       // Called after diff
        _commit: null,      // Called after commit
        unmount: null,      // Called before unmount
        _root: null,        // Called on root render
        _catchError: null,  // Error handler
        _hook: null,        // Hook state access
        useDebugValue: null, // Debug value formatter
        requestAnimationFrame: null, // Custom RAF
        _skipEffects: false  // Skip effects flag
    };

/**
 * Bind vnode parent/root metadata used by incremental updates
 */
function setVNodeParent(vnode, parentVNode, index, root) {
    if (vnode == null || vnode === false || vnode === true) {
        return;
    }

    if (Array.isArray(vnode)) {
        for (var i = 0; i < vnode.length; i++) {
            setVNodeParent(vnode[i], parentVNode, i, root);
        }
        return;
    }

    if (typeof vnode !== 'object') {
        return;
    }

    vnode.__parentVNode = parentVNode || null;
    vnode.__index = index || 0;
    vnode.__root = root || null;
}

function bindChildVNodes(parentVNode, children, root) {
    if (!children) {
        return;
    }

    if (!Array.isArray(children)) {
        setVNodeParent(children, parentVNode, 0, root);
        return;
    }

    for (var i = 0; i < children.length; i++) {
        setVNodeParent(children[i], parentVNode, i, root);
    }
}

/**
 * Update parent DOM pointers after DOM changes
 * Based on official Preact updateParentDomPointers logic
 * Only updates component vnodes, clears and re-finds first DOM
 */
function updateParentDOMPointers(vnode) {
    var parentVNode = vnode && vnode.__parentVNode;

    while (parentVNode && typeof parentVNode === 'object') {
        // Only update component vnodes
        if (!parentVNode.__component) {
            parentVNode = parentVNode.__parentVNode;
            continue;
        }

        // Clear and re-find first DOM
        parentVNode.__dom = null;

        var children = parentVNode.children || [];
        for (var i = 0; i < children.length; i++) {
            var child = children[i];
            if (child != null && child !== false && child !== true) {
                var childDOM = getVNodeDOM(child);
                if (childDOM) {
                    parentVNode.__dom = childDOM;
                    if (parentVNode.__component) {
                        parentVNode.__component.__dom = childDOM;
                    }
                    break;
                }
            }
        }

        parentVNode = parentVNode.__parentVNode;
    }
}

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
        component.__root = null;
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
    vnode.__parentVNode = null;
    vnode.__index = 0;
    vnode.__root = null;
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
 * Render 焦点恢复辅助
 */
function isNodeInsideContainer(node, container) {
    if (!node || !container) {
        return false;
    }
    if (node === container) {
        return true;
    }
    if (typeof container.contains === 'function') {
        try {
            return !!container.contains(node);
        } catch (e) {}
    }
    var current = node;
    while (current) {
        if (current === container) {
            return true;
        }
        current = current.parentNode;
    }
    return false;
}

function getElementAttrSafe(element, name) {
    if (!element || typeof element.getAttribute !== 'function') {
        return null;
    }
    try {
        return element.getAttribute(name);
    } catch (e) {
        return null;
    }
}

function getChildNodeIndex(parent, child) {
    if (!parent || !parent.childNodes) {
        return -1;
    }
    for (var i = 0; i < parent.childNodes.length; i++) {
        if (parent.childNodes[i] === child) {
            return i;
        }
    }
    return -1;
}

function buildFocusPath(container, element) {
    var path = [];
    var current = element;
    while (current && current !== container) {
        var parent = current.parentNode;
        if (!parent) {
            return null;
        }
        var index = getChildNodeIndex(parent, current);
        if (index < 0) {
            return null;
        }
        path.push(index);
        current = parent;
    }
    if (current !== container) {
        return null;
    }
    path.reverse();
    return path;
}

function getNodeByPath(container, path) {
    var current = container;
    if (!current || !path) {
        return null;
    }
    for (var i = 0; i < path.length; i++) {
        if (!current.childNodes || path[i] >= current.childNodes.length) {
            return null;
        }
        current = current.childNodes[path[i]];
        if (!current) {
            return null;
        }
    }
    return current;
}

function isCompatibleFocusTarget(element, snapshot) {
    if (!element || !element.tagName || !snapshot) {
        return false;
    }
    if (element.tagName !== snapshot.tagName) {
        return false;
    }
    if (snapshot.type && element.type !== snapshot.type) {
        return false;
    }
    if (snapshot.name && getElementAttrSafe(element, 'name') !== snapshot.name) {
        return false;
    }
    if (snapshot.placeholder && getElementAttrSafe(element, 'placeholder') !== snapshot.placeholder) {
        return false;
    }
    return true;
}

function findFocusTargetByHints(container, snapshot) {
    if (!container || !snapshot || !container.childNodes) {
        return null;
    }
    var queue = [];
    for (var i = 0; i < container.childNodes.length; i++) {
        queue.push(container.childNodes[i]);
    }
    while (queue.length) {
        var node = queue.shift();
        if (isCompatibleFocusTarget(node, snapshot)) {
            return node;
        }
        if (node && node.childNodes) {
            for (var j = 0; j < node.childNodes.length; j++) {
                queue.push(node.childNodes[j]);
            }
        }
    }
    return null;
}

function captureFocusSnapshot(container) {
    if (typeof document === 'undefined' || !container) {
        return null;
    }
    var active = document.activeElement;
    if (!active || !active.tagName || !isNodeInsideContainer(active, container)) {
        return null;
    }
    var selectionStart = null;
    var selectionEnd = null;
    if (typeof active.selectionStart === 'number' && typeof active.selectionEnd === 'number') {
        selectionStart = active.selectionStart;
        selectionEnd = active.selectionEnd;
    }
    return {
        element: active,
        tagName: active.tagName,
        type: active.type || null,
        name: getElementAttrSafe(active, 'name'),
        placeholder: getElementAttrSafe(active, 'placeholder'),
        path: buildFocusPath(container, active),
        selectionStart: selectionStart,
        selectionEnd: selectionEnd
    };
}

function restoreFocusSnapshot(container, snapshot) {
    if (typeof document === 'undefined' || !container || !snapshot) {
        return;
    }
    var currentActive = document.activeElement;
    if (currentActive && currentActive !== snapshot.element && isNodeInsideContainer(currentActive, container)) {
        return;
    }
    var target = null;
    if (snapshot.element && isNodeInsideContainer(snapshot.element, container)) {
        target = snapshot.element;
    }
    if (!target && snapshot.path) {
        var pathTarget = getNodeByPath(container, snapshot.path);
        if (isCompatibleFocusTarget(pathTarget, snapshot)) {
            target = pathTarget;
        }
    }
    if (!target) {
        target = findFocusTargetByHints(container, snapshot);
    }
    if (!target || typeof target.focus !== 'function') {
        return;
    }
    if (document.activeElement !== target) {
        try {
            target.focus();
        } catch (e) {}
    }
    if (typeof snapshot.selectionStart === 'number' && typeof snapshot.selectionEnd === 'number' &&
        typeof target.setSelectionRange === 'function') {
        try {
            target.setSelectionRange(snapshot.selectionStart, snapshot.selectionEnd);
        } catch (e) {}
    }
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
    setVNodeParent(vnode, null, 0, trackedRoot);

    if (options._root) {
        options._root(vnode, container);
    }

    if (trackedRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkBeginRootTracking === 'function') {
        globalThis.__mbinkBeginRootTracking(trackedRoot);
    }

    var focusSnapshot = captureFocusSnapshot(container);

    try {
        if (typeof __preact_internal !== 'undefined' && __preact_internal.render) {
            return __preact_internal.render(vnode, container);
        }

        if (typeof document !== 'undefined' && typeof document.__beginBatch === 'function') {
            document.__beginBatch();
        }

        try {
            var oldVNode = container.__preactVNode;
            var oldDOM = container.__preactDOM;
            probeLog('preact.render', {
                oldVNodeId: ensureVNodeId(oldVNode),
                newVNodeId: ensureVNodeId(vnode),
                mountedContainers: __mountedContainers.length
            });

            var newDOM = diffNode(oldVNode, vnode, container, oldDOM);

            container.__preactVNode = vnode;
            container.__preactDOM = newDOM;
            if (trackedRoot) {
                trackedRoot.vnode = vnode;
            }
        } finally {
            if (typeof document !== 'undefined' && typeof document.__endBatch === 'function') {
                document.__endBatch();
            }
            restoreFocusSnapshot(container, focusSnapshot);
        }
    } finally {
        if (trackedRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkEndRootTracking === 'function') {
            globalThis.__mbinkEndRootTracking(trackedRoot);
        }
    }
}

/**
 * Hydrate a VNode tree into a DOM container (for SSR)
 * @param {object} vnode - Virtual DOM node
 * @param {Element} container - DOM container element with existing HTML
 */
function hydrate(vnode, container) {
    // Mark as hydrating
    if (vnode && typeof vnode === 'object') {
        vnode.__hydrating = true;
    }
    render(vnode, container);
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
    var componentRoot = vnode.__root;
    setVNodeParent(vnode, vnode.__parentVNode, vnode.__index, componentRoot);

    // Create component instance for hooks
    if (!vnode.__component) {
        vnode.__component = {
            __hooks: [],
            __vnode: vnode,
            __dom: null,
            __renderedVNode: null,
            __rerender: null,
            __root: componentRoot
        };
        probeLog('preact.createComponent', {
            vnodeId: ensureVNodeId(vnode)
        });
    }

    var component = vnode.__component;
    ensureComponentId(component);
    component.__vnode = vnode;
    component.__root = componentRoot;
    probeLog('preact.bindComponent', {
        componentId: component.__debugId,
        vnodeId: ensureVNodeId(vnode)
    });

    // Set up rerender function using Virtual DOM diffing
    if (!component.__rerender) {
        component.__rerender = function () {
            var currentVNode = component.__vnode;
            var currentRoot = component.__root || (currentVNode && currentVNode.__root) || null;
            var shouldTrackRoot = !!(currentRoot && currentVNode && !currentVNode.__parentVNode);

            if (shouldTrackRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkBeginRootTracking === 'function') {
                globalThis.__mbinkBeginRootTracking(currentRoot);
            }

            try {
                if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
                    PreactHooks.setCurrentComponent(component);
                }

                var newRenderedVNode = currentVNode.type(currentVNode.props);
                setVNodeParent(newRenderedVNode, currentVNode, 0, currentRoot);

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
                currentVNode.__dom = newDOM;

                // Update parent's children slot to point to current vnode (not old one)
                // This prevents parent from holding stale vnode references
                var parentVNode = currentVNode.__parentVNode;
                if (parentVNode && parentVNode.children && typeof currentVNode.__index === 'number') {
                    parentVNode.children[currentVNode.__index] = currentVNode;
                }

                updateParentDOMPointers(currentVNode);

                // 更新时 oldRenderedVNode 里的组件/子树可能已复用到新图，
                // 这里不能再 detach，避免把仍在使用的 hooks / __vnode / __renderedVNode 清空。
            } catch (e) {
            } finally {
                if (shouldTrackRoot && typeof globalThis !== 'undefined' && typeof globalThis.__mbinkEndRootTracking === 'function') {
                    globalThis.__mbinkEndRootTracking(currentRoot);
                }
            }
        };
    }

    // Set current component for hooks
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(component);
    }

    // Call component function
    var renderedVNode = vnode.type(vnode.props);
    setVNodeParent(renderedVNode, vnode, 0, componentRoot);

    // Clear current component
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(null);
    }

    // Create DOM element
    var dom = createDOMElement(renderedVNode);

    // Store DOM reference and rendered VNode
    component.__dom = dom;
    component.__renderedVNode = renderedVNode;
    vnode.__dom = dom;
    updateParentDOMPointers(vnode);

    return dom;
}

/**
 * Get the next DOM sibling for insertion anchor
 * Based on official Preact getDomSibling logic
 * @param {object} vnode - Parent vnode
 * @param {number} childIndex - Start searching from this child index (null means resume from parent)
 * @returns {Element|null} Next DOM sibling or null
 */
function getDomSibling(vnode, childIndex) {
    if (childIndex == null) {
        // Resume search from parent's next sibling
        return vnode.__parentVNode
            ? getDomSibling(vnode.__parentVNode, vnode.__index + 1)
            : null;
    }

    var sibling;
    var children = vnode.children || [];
    for (; childIndex < children.length; childIndex++) {
        sibling = children[childIndex];

        if (sibling != null && sibling !== false && sibling !== true) {
            var siblingDOM = getVNodeDOM(sibling);
            if (siblingDOM) {
                return siblingDOM;
            }
        }
    }

    // No DOM found in children, climb up if this is a function component
    return typeof vnode.type === 'function' ? getDomSibling(vnode, null) : null;
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
            vnode.__dom = vnode.__component.__dom;
            return vnode.__dom;
        }
        if (vnode.__component.__renderedVNode) {
            vnode.__dom = getVNodeDOM(vnode.__component.__renderedVNode);
            return vnode.__dom;
        }
    }

    if (vnode.children && vnode.children.length) {
        for (var j = 0; j < vnode.children.length; j++) {
            var childDOM = getVNodeDOM(vnode.children[j]);
            if (childDOM) {
                vnode.__dom = childDOM;
                return childDOM;
            }
        }
    }

    if (vnode.props && vnode.props.children) {
        vnode.__dom = getVNodeDOM(vnode.props.children);
        return vnode.__dom;
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

            handlers[prop] = newValue;
            if (evtName === 'change') {
                handlers[prop + '_input'] = newValue;
            }

            if (!listeners[prop]) {
                var stableHandler = createStableHandler(elementId, prop);
                element.addEventListener(evtName, stableHandler);
                listeners[prop] = stableHandler;

                if (evtName === 'change') {
                    var stableInputHandler = createStableHandler(elementId, prop + '_input');
                    element.addEventListener('input', stableInputHandler);
                    listeners[prop + '_input'] = stableInputHandler;
                }
            }
        } else if (prop === 'value' && (element.tagName === 'INPUT' || element.tagName === 'TEXTAREA')) {
            if (element.value !== String(newValue)) {
                element.value = newValue;
            }
        } else if (newValue === oldValue) {
            continue;
        } else if (prop === 'className') {
            element.className = newValue || '';
        } else if (prop === 'style') {
            if (typeof newValue === 'string') {
                if (element.style.cssText !== newValue) {
                    element.style.cssText = newValue;
                }
            } else if (typeof newValue === 'object') {
                if (typeof oldValue === 'object' && oldValue) {
                    for (var oldStyle in oldValue) {
                        if (!(oldStyle in newValue)) {
                            element.style[oldStyle] = '';
                        }
                    }
                }
                for (var styleProp in newValue) {
                    var newStyleValue = newValue[styleProp];
                    var oldStyleValue = oldValue && oldValue[styleProp];
                    if (newStyleValue !== oldStyleValue) {
                        element.style[styleProp] = newStyleValue;
                    }
                }
            }
        } else if (prop === 'contentEditable') {
            element.setAttribute('contenteditable', newValue === true ? 'true' : String(newValue));
        } else if (typeof newValue === 'boolean') {
            if (newValue) {
                element.setAttribute(prop, '');
            } else {
                element.removeAttribute(prop);
            }
        } else if (prop === 'dangerouslySetInnerHTML') {
            if (newValue && newValue.__html != null) {
                element.innerHTML = newValue.__html;
            }
        } else if (prop === 'innerHTML') {
            element.innerHTML = newValue || '';
        } else if (prop === 'textContent') {
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
        if (newVNode && typeof newVNode === 'object') {
            if (!newVNode.__parentVNode && oldVNode && typeof oldVNode === 'object') {
                newVNode.__parentVNode = oldVNode.__parentVNode || null;
                newVNode.__index = oldVNode.__index || 0;
                newVNode.__root = oldVNode.__root || null;
            }
        }

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
            if (oldVNode && typeof oldVNode === 'object') {
                updateParentDOMPointers(oldVNode);
            }
            return null;
        }

        // Old node is null - create new
        if (oldVNode == null || oldVNode === false || oldVNode === true) {
            var newDOM = createDOMElement(newVNode);
            if (newDOM && parentDOM) {
                parentDOM.appendChild(newDOM);
            }
            if (newVNode && typeof newVNode === 'object') {
                newVNode.__dom = newDOM;
                updateParentDOMPointers(newVNode);
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
            if (newVNode && typeof newVNode === 'object') {
                newVNode.__dom = replacementDOM;
                updateParentDOMPointers(newVNode);
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
            if (newVNode && typeof newVNode === 'object') {
                newVNode.__dom = recreatedDOM;
                updateParentDOMPointers(newVNode);
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

    // Reuse the component instance
    var component = oldVNode.__component;
    if (!component) {
        // No old component, create new
        return createDOMElement(newVNode);
    }

    // Transfer component to new vnode
    newVNode.__component = component;
    newVNode.__parentVNode = oldVNode.__parentVNode;
    newVNode.__index = oldVNode.__index;
    newVNode.__root = oldVNode.__root;
    ensureComponentId(component);
    component.__vnode = newVNode;
    component.__root = newVNode.__root;

    // Set current component for hooks
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(component);
    }

    // Get new rendered VNode
    var newRenderedVNode = newVNode.type(newVNode.props);
    setVNodeParent(newRenderedVNode, newVNode, 0, newVNode.__root);

    // Clear current component
    if (typeof PreactHooks !== 'undefined' && PreactHooks.setCurrentComponent) {
        PreactHooks.setCurrentComponent(null);
    }

    var oldRenderedVNode = component.__renderedVNode;

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
    newVNode.__dom = newDOM;
    updateParentDOMPointers(newVNode);

    // 更新路径里 old vnode 可能仍承载被复用的 component 实例，
    // 这里如果 detach 会把新图正在使用的 hooks / __vnode 一起清掉，
    // 复杂页面下会直接表现为 SharedState 变了但 UI 不再刷新。

    return newDOM;
}

/**
 * Diff element VNodes (same tag type)
 */
function diffElement(oldVNode, newVNode, dom) {
    var tagName = dom && dom.tagName ? dom.tagName.toLowerCase() : 'unknown';

    if (!newVNode.__root) {
        newVNode.__root = oldVNode.__root;
    }

    // Check if this is an SVG element
    var SVG_TAGS = ['svg', 'circle', 'ellipse', 'line', 'path', 'polygon', 'polyline', 'rect', 'g', 'text', 'tspan', 'defs', 'use', 'symbol', 'clipPath', 'mask', 'pattern', 'image', 'foreignObject', 'linearGradient', 'radialGradient', 'stop'];
    var isSVG = SVG_TAGS.indexOf(tagName) !== -1;

    // Update props - event listeners use stable wrappers so we can update handlers safely
    setDOMProps(dom, oldVNode.props || {}, newVNode.props || {}, isSVG);

    // Diff children
    bindChildVNodes(oldVNode, oldVNode.children || [], oldVNode.__root);
    bindChildVNodes(newVNode, newVNode.children || [], newVNode.__root);
    diffChildren(oldVNode, newVNode, dom);

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

    return dom;
}

/**
 * Diff children arrays using key-based algorithm
 */
function diffChildren(oldParentVNode, newParentVNode, parentDOM) {
    if (!parentDOM) {
        return;
    }

    var oldChildren = oldParentVNode && oldParentVNode.children ? oldParentVNode.children : [];
    var newChildren = newParentVNode && newParentVNode.children ? newParentVNode.children : [];
    var parentRoot = newParentVNode ? newParentVNode.__root : null;
    bindChildVNodes(newParentVNode, newChildren, parentRoot);

    var oldLen = oldChildren.length;
    var newLen = newChildren.length;

    if (!parentDOM.childNodes) {
        for (var n = 0; n < newLen; n++) {
            setVNodeParent(newChildren[n], newParentVNode, n, parentRoot);
            var dom = createDOMElement(newChildren[n]);
            if (dom) {
                parentDOM.appendChild(dom);
            }
        }
        newParentVNode.__dom = getVNodeDOM(newParentVNode);
        return;
    }

    var oldEntries = [];
    var usedOld = {};
    var childNodesArray = [];
    var childNodes = parentDOM.childNodes;
    var childNodesLen = childNodes.length;
    var domCursor = 0;

    for (var k = 0; k < childNodesLen; k++) {
        childNodesArray.push(childNodes[k]);
    }

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

        oldEntries[i] = {
            vnode: oldChild,
            dom: oldDOM,
            index: i,
            key: getKey(oldChild)
        };
    }

    function canReuseEntry(entry, newChild) {
        return !!(entry && !usedOld[entry.index] && isSameVNodeType(entry.vnode, newChild));
    }

    function findMatchingEntry(newChild, startIndex) {
        var newKey = getKey(newChild);
        var candidate = null;
        var x;

        if (startIndex >= 0 && startIndex < oldEntries.length) {
            candidate = oldEntries[startIndex];
            if (canReuseEntry(candidate, newChild) && candidate.key === newKey) {
                return candidate;
            }
        }

        for (x = startIndex - 1; x >= 0; x--) {
            candidate = oldEntries[x];
            if (canReuseEntry(candidate, newChild) && candidate.key === newKey) {
                return candidate;
            }
        }

        for (x = startIndex + 1; x < oldEntries.length; x++) {
            candidate = oldEntries[x];
            if (canReuseEntry(candidate, newChild) && candidate.key === newKey) {
                return candidate;
            }
        }

        return null;
    }

    for (var j = 0; j < newLen; j++) {
        var newChild = newChildren[j];
        setVNodeParent(newChild, newParentVNode, j, parentRoot);
        var oldEntry = findMatchingEntry(newChild, j);
        var matchedOldDOM = oldEntry ? oldEntry.dom : null;
        var currentDOMAtPosition = parentDOM.childNodes[j] || null;
        var domToPlace = null;

        if (oldEntry) {
            usedOld[oldEntry.index] = true;
            domToPlace = diffNode(oldEntry.vnode, newChild, parentDOM, matchedOldDOM) || matchedOldDOM;
        } else {
            domToPlace = createDOMElement(newChild);
        }

        if (domToPlace) {
            if (domToPlace.parentNode !== parentDOM) {
                if (currentDOMAtPosition) {
                    parentDOM.insertBefore(domToPlace, currentDOMAtPosition);
                } else {
                    parentDOM.appendChild(domToPlace);
                }
            } else if (domToPlace !== currentDOMAtPosition) {
                if (currentDOMAtPosition) {
                    parentDOM.insertBefore(domToPlace, currentDOMAtPosition);
                } else {
                    parentDOM.appendChild(domToPlace);
                }
            }
        }
    }

    for (var m = oldLen - 1; m >= 0; m--) {
        if (!usedOld[m]) {
            var oldEntryToRemove = oldEntries[m];
            var oldChildVNode = oldEntryToRemove ? oldEntryToRemove.vnode : oldChildren[m];
            var domToRemove = oldEntryToRemove ? oldEntryToRemove.dom : null;

            if (oldChildVNode) {
                clearVNode(oldChildVNode);
            }

            if (domToRemove && domToRemove.parentNode === parentDOM) {
                try {
                    parentDOM.removeChild(domToRemove);
                } catch (e) {
                    // Ignore removal errors
                }
            }
        }
    }

    if (newParentVNode) {
        newParentVNode.__dom = getVNodeDOM(newParentVNode);
        updateParentDOMPointers(newParentVNode);
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

    /**
     * Flatten and loop through the children of a virtual node
     * Based on official Preact toChildArray
     * @param {any} children - The unflattened children
     * @param {Array} out - Output array
     * @returns {Array} Flattened children array
     */
    function toChildArray(children, out) {
        out = out || [];
        if (children == null || typeof children === 'boolean') {
            // Skip null, undefined, true, false
        } else if (Array.isArray(children)) {
            // Flatten arrays recursively
            for (var i = 0; i < children.length; i++) {
                toChildArray(children[i], out);
            }
        } else {
            // Add primitives and vnodes
            out.push(children);
        }
        return out;
    }

    // Export all APIs as global object (for script loading)
    var Preact = {
        h: h,
        createElement: createElement,
        render: render,
        hydrate: hydrate,
        Fragment: Fragment,
        Component: Component,
        createRef: createRef,
        cloneElement: cloneElement,
        isValidElement: isValidElement,
        createContext: createContext,
        toChildArray: toChildArray,
        options: options
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
