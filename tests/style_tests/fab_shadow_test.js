/**
 * FAB (Floating Action Button) 圆形阴影测试
 * 
 * 测试目的：验证圆形按钮的 box-shadow 是否正确渲染为圆形阴影
 * 
 * 问题描述：
 * - 按钮使用 border-radius: 50% 创建圆形
 * - 使用 box-shadow: 0 4px 12px rgba(0,0,0,0.3) 添加阴影
 * - 期望：阴影应该是圆形的，跟随按钮形状
 * - 实际：可能出现矩形背景残留
 */

import { h, render } from 'preact';

function FABShadowTest() {
    return h('div', { 
        style: 'padding: 40px; font-family: Arial, sans-serif; background: #f0f0f0; min-height: 100vh;' 
    },
        h('h1', { style: 'color: #333; margin: 0 0 30px 0;' }, 'FAB 圆形阴影测试'),
        
        // 测试说明
        h('div', { style: 'margin-bottom: 30px; padding: 15px; background: #fff3cd; border-radius: 8px; color: #856404;' },
            h('p', { style: 'margin: 0 0 10px 0; font-weight: bold;' }, '测试说明：'),
            h('p', { style: 'margin: 0;' }, '下方的圆形按钮应该显示圆形阴影，而不是矩形阴影。如果阴影呈矩形或有矩形背景残留，则说明存在渲染问题。')
        ),
        
        // 测试区域 - 浅色背景
        h('div', { style: 'margin-bottom: 40px; padding: 60px; background: #ffffff; border-radius: 12px; position: relative;' },
            h('h3', { style: 'margin: 0 0 20px 0; color: #666;' }, '测试 1: 浅色背景上的 FAB'),
            
            // 小型 FAB
            h('div', { style: 'display: flex; gap: 40px; align-items: center; flex-wrap: wrap;' },
                // 40x40 FAB
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 40px; height: 40px; background: #4caf50; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.3); margin: 0 auto 10px;'
                    }, '+'),
                    h('span', { style: 'font-size: 12px; color: #666;' }, '40x40')
                ),
                
                // 56x56 FAB (标准尺寸)
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #2196f3; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin: 0 auto 10px;'
                    }, '+'),
                    h('span', { style: 'font-size: 12px; color: #666;' }, '56x56 (标准)')
                ),
                
                // 72x72 FAB
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 72px; height: 72px; background: #ff5722; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 32px; box-shadow: 0 6px 16px rgba(0,0,0,0.3); margin: 0 auto 10px;'
                    }, '+'),
                    h('span', { style: 'font-size: 12px; color: #666;' }, '72x72')
                )
            )
        ),
        
        // 测试区域 - 深色背景
        h('div', { style: 'margin-bottom: 40px; padding: 60px; background: #333; border-radius: 12px; position: relative;' },
            h('h3', { style: 'margin: 0 0 20px 0; color: #fff;' }, '测试 2: 深色背景上的 FAB'),
            
            h('div', { style: 'display: flex; gap: 40px; align-items: center; flex-wrap: wrap;' },
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #e91e63; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(0,0,0,0.5);' 
                    }, '♥'),
                    h('span', { style: 'font-size: 12px; color: #aaa; display: block; margin-top: 10px;' }, '深色阴影')
                ),
                
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #ffeb3b; color: #333; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px rgba(255,255,255,0.3);' 
                    }, '★'),
                    h('span', { style: 'font-size: 12px; color: #aaa; display: block; margin-top: 10px;' }, '浅色阴影')
                )
            )
        ),
        
        // 测试区域 - 不同阴影参数
        h('div', { style: 'margin-bottom: 40px; padding: 60px; background: #e8e8e8; border-radius: 12px;' },
            h('h3', { style: 'margin: 0 0 20px 0; color: #666;' }, '测试 3: 不同阴影参数'),
            
            h('div', { style: 'display: flex; gap: 50px; align-items: center; flex-wrap: wrap;' },
                // 无模糊
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #9c27b0; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 4px 4px 0 rgba(0,0,0,0.3);' 
                    }, 'A'),
                    h('span', { style: 'font-size: 12px; color: #666; display: block; margin-top: 10px;' }, '无模糊')
                ),
                
                // 小模糊
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #00bcd4; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 2px 4px rgba(0,0,0,0.3);' 
                    }, 'B'),
                    h('span', { style: 'font-size: 12px; color: #666; display: block; margin-top: 10px;' }, '小模糊 (4px)')
                ),
                
                // 大模糊
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #ff9800; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 8px 24px rgba(0,0,0,0.4);' 
                    }, 'C'),
                    h('span', { style: 'font-size: 12px; color: #666; display: block; margin-top: 10px;' }, '大模糊 (24px)')
                ),
                
                // 带扩展
                h('div', { style: 'text-align: center;' },
                    h('div', { 
                        style: 'width: 56px; height: 56px; background: #607d8b; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 24px; box-shadow: 0 4px 12px 4px rgba(0,0,0,0.3);' 
                    }, 'D'),
                    h('span', { style: 'font-size: 12px; color: #666; display: block; margin-top: 10px;' }, '带扩展 (spread: 4px)')
                )
            )
        ),
        
        // 固定位置的 FAB（右下角）
        h('div', { 
            style: 'position: fixed; bottom: 24px; right: 24px; width: 60px; height: 60px; background: #4caf50; color: white; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 28px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); cursor: pointer; z-index: 1000;' 
        }, '+'),
        
        // 状态指示
        h('div', { 
            style: 'padding: 15px; background: #4caf50; color: white; border-radius: 8px; text-align: center; font-weight: bold;' 
        }, '✓ FAB Shadow Test Loaded - 检查右下角的固定 FAB 按钮')
    );
}

const root = document.getElementById('root') || document.body;
render(h(FABShadowTest), root);

console.log('[FAB Shadow Test] Test rendered');

