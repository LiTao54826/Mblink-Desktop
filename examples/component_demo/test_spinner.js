/**
 * Spinner 组件单独测试
 */

import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

// 复制 Button 中的 Spinner 组件
function Spinner({ size = 14 }) {
  const [angle, setAngle] = useState(0);

  useEffect(() => {
    console.log('Spinner useEffect called');
    const interval = setInterval(() => {
      setAngle((prev) => {
        console.log('Spinner angle:', prev);
        return (prev + 30) % 360;
      });
    }, 50);
    return () => clearInterval(interval);
  }, []);

  const spinnerStyle = {
    width: `${size}px`,
    height: `${size}px`,
    border: '2px solid transparent',
    borderTopColor: 'currentColor',
    borderRightColor: 'currentColor',
    borderRadius: '50%',
    display: 'inline-block',
    verticalAlign: 'middle',
    transform: `rotate(${angle}deg)`,
  };

  return h('span', { style: spinnerStyle });
}

// 模拟 Button 组件结构
function Button({ loading, children }) {
  const buttonStyle = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: '6px',
    padding: '8px 16px',
    background: '#3b82f6',
    color: 'white',
    border: 'none',
    borderRadius: '6px',
  };

  return h('button', { style: buttonStyle }, [
    loading ? h(Spinner, { size: 16 }) : null,
    children,
  ]);
}

function App() {
  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title' }, 'Spinner Test'),
    
    h('div', { key: 'test1', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 1: Spinner 单独使用'),
      h(Spinner, { size: 30 }),
    ]),
    
    h('div', { key: 'test2', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 2: Spinner 在 Button 内'),
      h(Button, { loading: true }, 'Loading'),
    ]),
    
    h('div', { key: 'test3', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 3: 直接的旋转方块'),
      h(RotatingBox),
    ]),
  ]);
}

function RotatingBox() {
  const [angle, setAngle] = useState(0);

  useEffect(() => {
    const interval = setInterval(() => {
      setAngle((prev) => (prev + 30) % 360);
    }, 50);
    return () => clearInterval(interval);
  }, []);

  return h('div', { 
    style: { 
      width: '30px', 
      height: '30px', 
      background: 'red',
      transform: `rotate(${angle}deg)`,
    } 
  });
}

console.log('Starting spinner test...');
render(h(App), document.body);
console.log('Render complete');
