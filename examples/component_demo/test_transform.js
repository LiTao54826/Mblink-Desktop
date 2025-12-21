/**
 * Transform 动画测试
 */

import { h, render } from 'preact';
import { useRef, useEffect, useState } from 'preact/hooks';

function TransformTest() {
  const ref = useRef(null);
  const [angle, setAngle] = useState(0);

  // 方法1: 使用 useState 触发重渲染
  useEffect(() => {
    const interval = setInterval(() => {
      setAngle(prev => (prev + 30) % 360);
    }, 100);
    return () => clearInterval(interval);
  }, []);

  // 方法2: 直接操作 DOM (在另一个元素上测试)
  useEffect(() => {
    if (!ref.current) return;
    let a = 0;
    const interval = setInterval(() => {
      a = (a + 30) % 360;
      if (ref.current) {
        // 测试 setAttribute
        ref.current.setAttribute('style', 
          `width:50px;height:50px;background:blue;transform:rotate(${a}deg);`
        );
      }
    }, 100);
    return () => clearInterval(interval);
  }, []);

  return h('div', { style: { padding: '20px' } }, [
    h('h1', { key: 'title' }, 'Transform Animation Test'),
    
    h('div', { key: 'test1', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 1: useState + inline style transform'),
      h('div', { 
        style: { 
          width: '50px', 
          height: '50px', 
          background: 'red',
          transform: `rotate(${angle}deg)`
        } 
      }),
      h('p', {}, `Angle: ${angle}`),
    ]),
    
    h('div', { key: 'test2', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 2: setAttribute with transform'),
      h('div', { 
        ref,
        style: { 
          width: '50px', 
          height: '50px', 
          background: 'blue'
        } 
      }),
    ]),
    
    h('div', { key: 'test3', style: { marginTop: '20px' } }, [
      h('p', {}, 'Test 3: Static transform (should be rotated 45deg)'),
      h('div', { 
        style: { 
          width: '50px', 
          height: '50px', 
          background: 'green',
          transform: 'rotate(45deg)'
        } 
      }),
    ]),
  ]);
}

console.log('Starting transform test...');
render(h(TransformTest), document.body);
console.log('Render complete');
