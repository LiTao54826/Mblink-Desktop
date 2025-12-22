/**
 * Flex Center 布局测试 - 验证子元素尺寸变化时居中是否正确更新
 */

import { h, render } from 'preact';
import { useState } from 'preact/hooks';

function App() {
  const [size1, setSize1] = useState(0);
  const [size2, setSize2] = useState(6);
  const [size3, setSize3] = useState(0);

  const boxStyle = {
    width: '50px',
    height: '50px',
    backgroundColor: '#3b82f6',
    borderRadius: '50%',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: '20px',
  };

  const dotStyle = (size) => ({
    width: `${size}px`,
    height: `${size}px`,
    backgroundColor: '#ffffff',
    borderRadius: '50%',
  });

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title', style: { marginBottom: '20px' } }, 'Flex Center Test'),
    
    h('p', { style: { marginBottom: '10px' } }, 'Click boxes to toggle inner dot:'),
    
    // Box 1
    h('div', { key: 'box1', style: boxStyle, onClick: () => setSize1(size1 === 0 ? 20 : 0) },
      h('div', { style: dotStyle(size1) })
    ),
    
    // Box 2 (initially has dot)
    h('div', { key: 'box2', style: boxStyle, onClick: () => setSize2(size2 === 0 ? 20 : 0) },
      h('div', { style: dotStyle(size2) })
    ),
    
    // Box 3
    h('div', { key: 'box3', style: boxStyle, onClick: () => setSize3(size3 === 0 ? 20 : 0) },
      h('div', { style: dotStyle(size3) })
    ),
    
    h('div', { key: 'info', style: { marginTop: '20px', padding: '12px', background: '#f5f5f5', borderRadius: '4px' } }, [
      h('p', {}, `Box 1 dot size: ${size1}px`),
      h('p', {}, `Box 2 dot size: ${size2}px`),
      h('p', {}, `Box 3 dot size: ${size3}px`),
    ]),
  ]);
}

console.log('Starting render...');
render(h(App), document.body);
console.log('Render complete');
