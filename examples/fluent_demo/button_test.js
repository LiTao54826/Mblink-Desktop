// 简单的 Fluent Button 测试
import { h, render } from 'preact';
import { Button } from '../../js/fluent/Button.js';

function App() {
    return h('div', { 
        style: { 
            padding: '20px',
            backgroundColor: '#f5f5f5'
        } 
    }, [
        h(Button, { key: 'btn1' }, 'Click Me'),
        h('div', { style: { height: '10px' } }),
        h(Button, { key: 'btn2', appearance: 'primary' }, 'Primary')
    ]);
}

render(h(App), document.body);
