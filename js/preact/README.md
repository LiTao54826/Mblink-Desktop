# Preact Integration for MBink

This directory contains Preact library files for MBink's React ecosystem support.

## Version
- Preact: 10.19.3 (latest stable)
- Size: ~3KB gzipped

## Files
- `preact.js` - Core Preact library (development build)
- `preact.min.js` - Minified production build
- `hooks.js` - Preact Hooks (useState, useEffect, etc.)
- `hooks.min.js` - Minified hooks

## Usage

```javascript
import { h, render } from './preact.js';
import { useState, useEffect } from './hooks.js';

function App() {
  const [count, setCount] = useState(0);
  
  return h('div', null,
    h('h1', null, 'Counter: ', count),
    h('button', { onClick: () => setCount(count + 1) }, 'Increment')
  );
}

render(h(App), document.body);
```

## Integration with MBink

Preact is integrated with MBink through:
1. QuickJS module system (ES6 import/export)
2. PreactRenderer (Virtual DOM → MBink DOM mapping)
3. DOM bindings (document, window, events)

See `core/quickjs/preact_bindings.h` for implementation details.

