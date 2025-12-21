/**
 * Button 组件测试
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Button } from '../../js/components/button.js';
import { colors } from '../../js/components/theme.js';

function App() {
  const [count, setCount] = useState(0);

  console.log('Rendering App, count =', count);

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Button Test'),
    
    h('p', { key: 'count' }, `Count: ${count}`),
    
    h('div', { key: 'buttons', style: { display: 'flex', gap: '8px', marginTop: '16px' } }, [
      h(Button, { 
        key: 'primary',
        variant: 'primary', 
        onClick: () => {
          console.log('Primary clicked');
          setCount(count + 1);
        }
      }, 'Primary'),
      
      h(Button, { 
        key: 'secondary',
        variant: 'secondary', 
        onClick: () => setCount(count - 1)
      }, 'Secondary'),
      
      h(Button, { 
        key: 'outline',
        variant: 'outline'
      }, 'Outline'),
      
      h(Button, { 
        key: 'danger',
        variant: 'danger'
      }, 'Danger'),
    ]),
    
    h('div', { key: 'sizes', style: { display: 'flex', gap: '8px', marginTop: '16px', alignItems: 'center' } }, [
      h(Button, { key: 'sm', size: 'sm' }, 'Small'),
      h(Button, { key: 'md', size: 'md' }, 'Medium'),
      h(Button, { key: 'lg', size: 'lg' }, 'Large'),
    ]),
    
    h('div', { key: 'states', style: { display: 'flex', gap: '8px', marginTop: '16px' } }, [
      h(Button, { key: 'disabled', disabled: true }, 'Disabled'),
      h(Button, { key: 'loading', loading: true }, 'Loading'),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
