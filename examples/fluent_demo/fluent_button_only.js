// 直接测试 Fluent Button
import { h, render } from 'preact';
import { Button } from '../../js/fluent/Button.js';

function App() {
    return h('div', { 
        style: { 
            padding: '50px',
            backgroundColor: '#f0f0f0',
            display: 'flex',
            gap: '20px',
            alignItems: 'flex-start'
        } 
    }, [
        h(Button, { key: 'small', size: 'small' }, 'Small'),
        h(Button, { key: 'medium', size: 'medium' }, 'Medium'),
        h(Button, { key: 'large', size: 'large' }, 'Large')
    ]);
}

render(h(App), document.body);
