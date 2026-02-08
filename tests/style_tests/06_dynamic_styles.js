/**
 * 测试场景 6: 动态样式更新
 * 测试目标: 验证样式动态变化、重新布局、重绘性能
 * 潜在 BUG: 样式更新不触发重绘、布局抖动、内存泄漏
 * 
 * 运行: build\bin\Release\esm_loader.exe tests\style_tests\06_dynamic_styles.js
 */

import { h, render } from 'preact';
import { useState, useEffect } from 'preact/hooks';

function DynamicStylesTest() {
    const [size, setSize] = useState(100);
    const [color, setColor] = useState('#2196f3');
    const [rotation, setRotation] = useState(0);
    const [opacity, setOpacity] = useState(1);
    const [borderRadius, setBorderRadius] = useState(0);
    const [flexDirection, setFlexDirection] = useState('row');
    const [justifyContent, setJustifyContent] = useState('flex-start');
    const [animating, setAnimating] = useState(false);
    
    // 自动动画
    useEffect(() => {
        if (animating) {
            const interval = setInterval(() => {
                setRotation(r => (r + 5) % 360);
                setColor(c => {
                    const colors = ['#2196f3', '#4caf50', '#ff9800', '#f44336', '#9c27b0'];
                    const idx = colors.indexOf(c);
                    return colors[(idx + 1) % colors.length];
                });
            }, 100);
            return () => clearInterval(interval);
        }
    }, [animating]);
    
    return h('div', { 
        style: 'padding: 20px; font-family: Arial, sans-serif; background: #f5f5f5;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 20px 0;' }, 'Dynamic Styles Test'),
        
        // 尺寸动态变化
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #2196f3;' }, 'Dynamic Size'),
            h('div', { style: 'display: flex; align-items: center; gap: 20px; margin-bottom: 15px;' },
                h('label', { style: 'font-weight: bold;' }, `Size: ${size}px`),
                h('input', {
                    type: 'range',
                    min: '50',
                    max: '300',
                    value: size,
                    oninput: (e) => setSize(parseInt(e.target.value)),
                    style: 'flex: 1;'
                })
            ),
            h('div', { style: 'display: flex; justify-content: center; padding: 20px; background: #e3f2fd; border-radius: 4px;' },
                h('div', { 
                    style: `width: ${size}px; height: ${size}px; background: #2196f3; border-radius: 8px; display: flex; align-items: center; justify-content: center; color: white; font-weight: bold; transition: all 0.3s;` 
                }, `${size}x${size}`)
            )
        ),
        
        // 颜色动态变化
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #4caf50;' }, 'Dynamic Color'),
            h('div', { style: 'display: flex; gap: 10px; margin-bottom: 15px; flex-wrap: wrap;' },
                ['#2196f3', '#4caf50', '#ff9800', '#f44336', '#9c27b0', '#00bcd4'].map(c =>
                    h('button', {
                        key: c,
                        onclick: () => setColor(c),
                        style: `padding: 10px 20px; background: ${c}; color: white; border: ${color === c ? '3px solid #333' : 'none'}; border-radius: 4px; cursor: pointer;`
                    }, c)
                )
            ),
            h('div', { style: 'display: flex; justify-content: center; padding: 20px; background: #f5f5f5; border-radius: 4px;' },
                h('div', { 
                    style: `width: 150px; height: 150px; background: ${color}; border-radius: 8px; transition: background 0.3s;` 
                })
            )
        ),
        
        // 旋转和透明度
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #ff9800;' }, 'Rotation & Opacity'),
            h('div', { style: 'margin-bottom: 15px;' },
                h('div', { style: 'display: flex; align-items: center; gap: 20px; margin-bottom: 10px;' },
                    h('label', { style: 'font-weight: bold; min-width: 120px;' }, `Rotation: ${rotation}°`),
                    h('input', {
                        type: 'range',
                        min: '0',
                        max: '360',
                        value: rotation,
                        oninput: (e) => setRotation(parseInt(e.target.value)),
                        style: 'flex: 1;'
                    })
                ),
                h('div', { style: 'display: flex; align-items: center; gap: 20px;' },
                    h('label', { style: 'font-weight: bold; min-width: 120px;' }, `Opacity: ${opacity.toFixed(2)}`),
                    h('input', {
                        type: 'range',
                        min: '0',
                        max: '1',
                        step: '0.01',
                        value: opacity,
                        oninput: (e) => setOpacity(parseFloat(e.target.value)),
                        style: 'flex: 1;'
                    })
                )
            ),
            h('div', { style: 'display: flex; justify-content: center; padding: 40px; background: #fff3e0; border-radius: 4px;' },
                h('div', { 
                    style: `width: 120px; height: 120px; background: #ff9800; border-radius: 8px; transform: rotate(${rotation}deg); opacity: ${opacity}; transition: all 0.3s;` 
                })
            )
        ),
        
        // 圆角动态变化
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #9c27b0;' }, 'Dynamic Border Radius'),
            h('div', { style: 'display: flex; align-items: center; gap: 20px; margin-bottom: 15px;' },
                h('label', { style: 'font-weight: bold;' }, `Border Radius: ${borderRadius}px`),
                h('input', {
                    type: 'range',
                    min: '0',
                    max: '100',
                    value: borderRadius,
                    oninput: (e) => setBorderRadius(parseInt(e.target.value)),
                    style: 'flex: 1;'
                })
            ),
            h('div', { style: 'display: flex; justify-content: center; padding: 20px; background: #f3e5f5; border-radius: 4px;' },
                h('div', { 
                    style: `width: 150px; height: 150px; background: #9c27b0; border-radius: ${borderRadius}px; transition: border-radius 0.3s;` 
                })
            )
        ),
        
        // Flexbox 动态变化
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #f44336;' }, 'Dynamic Flexbox'),
            h('div', { style: 'margin-bottom: 15px;' },
                h('div', { style: 'margin-bottom: 10px;' },
                    h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'flex-direction:'),
                    h('div', { style: 'display: flex; gap: 10px;' },
                        ['row', 'column', 'row-reverse', 'column-reverse'].map(dir =>
                            h('button', {
                                key: dir,
                                onclick: () => setFlexDirection(dir),
                                style: `padding: 8px 16px; background: ${flexDirection === dir ? '#f44336' : '#e0e0e0'}; color: ${flexDirection === dir ? 'white' : 'black'}; border: none; border-radius: 4px; cursor: pointer;`
                            }, dir)
                        )
                    )
                ),
                h('div', null,
                    h('label', { style: 'font-weight: bold; display: block; margin-bottom: 5px;' }, 'justify-content:'),
                    h('div', { style: 'display: flex; gap: 10px; flex-wrap: wrap;' },
                        ['flex-start', 'center', 'flex-end', 'space-between', 'space-around'].map(jc =>
                            h('button', {
                                key: jc,
                                onclick: () => setJustifyContent(jc),
                                style: `padding: 8px 16px; background: ${justifyContent === jc ? '#f44336' : '#e0e0e0'}; color: ${justifyContent === jc ? 'white' : 'black'}; border: none; border-radius: 4px; cursor: pointer;`
                            }, jc)
                        )
                    )
                )
            ),
            h('div', { 
                style: `display: flex; flex-direction: ${flexDirection}; justify-content: ${justifyContent}; gap: 10px; padding: 20px; background: #ffebee; border-radius: 4px; min-height: 200px; transition: all 0.3s;` 
            },
                h('div', { style: 'width: 60px; height: 60px; background: #f44336; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '1'),
                h('div', { style: 'width: 60px; height: 60px; background: #e53935; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '2'),
                h('div', { style: 'width: 60px; height: 60px; background: #d32f2f; color: white; display: flex; align-items: center; justify-content: center; border-radius: 4px;' }, '3')
            )
        ),
        
        // 自动动画
        h('div', { style: 'margin-bottom: 30px; padding: 20px; background: white; border-radius: 8px;' },
            h('h2', { style: 'margin: 0 0 15px 0; color: #00bcd4;' }, 'Auto Animation'),
            h('button', {
                onclick: () => setAnimating(!animating),
                style: `padding: 10px 20px; background: ${animating ? '#f44336' : '#4caf50'}; color: white; border: none; border-radius: 4px; cursor: pointer; font-weight: bold; margin-bottom: 15px;`
            }, animating ? 'Stop Animation' : 'Start Animation'),
            h('div', { style: 'display: flex; justify-content: center; padding: 40px; background: #e0f7fa; border-radius: 4px;' },
                h('div', { 
                    style: `width: 100px; height: 100px; background: ${color}; border-radius: 8px; transform: rotate(${rotation}deg);` 
                })
            )
        ),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ Dynamic Styles Test Loaded')
    );
}

const root = document.getElementById('root') || document.body;
render(h(DynamicStylesTest), root);

console.log('[Test 06] Dynamic styles test rendered');

