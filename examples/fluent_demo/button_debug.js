/**
 * 按钮文字居中调试测试
 * 对比原生按钮和 Fluent 按钮的文字垂直居中
 */

import { h, render } from 'preact';
import { Button } from '../../js/fluent/index.js';

function App() {
  return h('div', {
    style: {
      padding: '40px',
      display: 'flex',
      flexDirection: 'column',
      gap: '20px',
    }
  }, [
    // 标题
    h('div', { key: 'title', style: { fontSize: '18px', fontWeight: 'bold' } }, 
      '按钮文字垂直居中测试'),
    
    // 原生按钮 - 使用默认字体
    h('div', { key: 'native-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, '原生按钮 (默认字体):'),
      h('button', {
        key: 'native-btn',
        style: {
          display: 'inline-flex',
          alignItems: 'center',
          justifyContent: 'center',
          height: '32px',
          padding: '0 12px',
          fontSize: '14px',
          lineHeight: 'normal',
          border: '1px solid #ccc',
          borderRadius: '4px',
          backgroundColor: '#f0f0f0',
        }
      }, 'Native Button'),
    ]),
    
    // 原生按钮 - 使用 Segoe UI 字体
    h('div', { key: 'native-segoe-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, '原生按钮 (Segoe UI 字体):'),
      h('button', {
        key: 'native-segoe-btn',
        style: {
          display: 'inline-flex',
          alignItems: 'center',
          justifyContent: 'center',
          height: '32px',
          padding: '0 12px',
          fontSize: '14px',
          fontFamily: "'Segoe UI', 'Segoe UI Web (West European)', -apple-system, BlinkMacSystemFont, Roboto, 'Helvetica Neue', sans-serif",
          lineHeight: 'normal',
          border: '1px solid #ccc',
          borderRadius: '4px',
          backgroundColor: '#f0f0f0',
        }
      }, 'Native Segoe UI'),
    ]),
    
    // Fluent 按钮
    h('div', { key: 'fluent-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, 'Fluent 按钮:'),
      h(Button, { key: 'fluent-btn', appearance: 'secondary' }, 'Fluent Button'),
    ]),
    
    // Fluent Primary 按钮
    h('div', { key: 'fluent-primary-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, 'Fluent Primary 按钮:'),
      h(Button, { key: 'fluent-primary-btn', appearance: 'primary' }, 'Fluent Primary'),
    ]),
    
    // 对比行 - 并排显示
    h('div', { key: 'compare-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, '并排对比:'),
      h('div', { 
        key: 'compare-row',
        style: { 
          display: 'flex', 
          gap: '16px', 
          alignItems: 'center',
          padding: '10px',
          backgroundColor: '#f5f5f5',
          borderRadius: '4px',
        } 
      }, [
        h('button', {
          key: 'native',
          style: {
            display: 'inline-flex',
            alignItems: 'center',
            justifyContent: 'center',
            height: '32px',
            padding: '0 12px',
            fontSize: '14px',
            lineHeight: 'normal',
            border: '1px solid #ccc',
            borderRadius: '4px',
            backgroundColor: '#fff',
          }
        }, 'Native'),
        h(Button, { key: 'fluent', appearance: 'secondary' }, 'Fluent'),
        h(Button, { key: 'fluent-p', appearance: 'primary' }, 'Primary'),
      ]),
    ]),
    
    // 不同尺寸对比
    h('div', { key: 'sizes-section' }, [
      h('div', { key: 'label', style: { marginBottom: '8px' } }, '不同尺寸:'),
      h('div', { 
        key: 'sizes-row',
        style: { 
          display: 'flex', 
          gap: '16px', 
          alignItems: 'center',
        } 
      }, [
        h(Button, { key: 'small', size: 'small', appearance: 'primary' }, 'Small'),
        h(Button, { key: 'medium', size: 'medium', appearance: 'primary' }, 'Medium'),
        h(Button, { key: 'large', size: 'large', appearance: 'primary' }, 'Large'),
      ]),
    ]),
  ]);
}

console.log('=== Button Debug Test ===');
render(h(App), document.body);
console.log('Render complete');
