/**
 * @file layout_extractor.js
 * @brief 从浏览器中提取布局信息，用于与 MBink 渲染结果比较
 * 
 * 在浏览器中运行此脚本，获取所有带有 data-test 属性的元素的布局信息
 * 输出 JSON 格式的布局数据
 */

(function() {
    'use strict';

    /**
     * 获取元素的完整布局信息
     * @param {HTMLElement} element 
     * @returns {Object} 布局信息对象
     */
    function getElementLayout(element) {
        const rect = element.getBoundingClientRect();
        const computedStyle = window.getComputedStyle(element);
        
        // 获取相对于父元素的位置
        const parentRect = element.offsetParent ? 
            element.offsetParent.getBoundingClientRect() : 
            { left: 0, top: 0 };
        
        return {
            // 基本信息
            testId: element.dataset.test,
            tagName: element.tagName.toLowerCase(),
            
            // 绝对位置（相对于视口）
            viewport: {
                x: Math.round(rect.left * 100) / 100,
                y: Math.round(rect.top * 100) / 100,
                width: Math.round(rect.width * 100) / 100,
                height: Math.round(rect.height * 100) / 100
            },
            
            // 相对位置（相对于 offsetParent）
            offset: {
                left: element.offsetLeft,
                top: element.offsetTop,
                width: element.offsetWidth,
                height: element.offsetHeight
            },
            
            // 内容尺寸
            client: {
                width: element.clientWidth,
                height: element.clientHeight,
                left: element.clientLeft,
                top: element.clientTop
            },
            
            // 滚动尺寸
            scroll: {
                width: element.scrollWidth,
                height: element.scrollHeight
            },
            
            // 计算样式
            computedStyle: {
                display: computedStyle.display,
                position: computedStyle.position,
                
                // 盒模型
                width: computedStyle.width,
                height: computedStyle.height,
                
                marginTop: computedStyle.marginTop,
                marginRight: computedStyle.marginRight,
                marginBottom: computedStyle.marginBottom,
                marginLeft: computedStyle.marginLeft,
                
                paddingTop: computedStyle.paddingTop,
                paddingRight: computedStyle.paddingRight,
                paddingBottom: computedStyle.paddingBottom,
                paddingLeft: computedStyle.paddingLeft,
                
                borderTopWidth: computedStyle.borderTopWidth,
                borderRightWidth: computedStyle.borderRightWidth,
                borderBottomWidth: computedStyle.borderBottomWidth,
                borderLeftWidth: computedStyle.borderLeftWidth,
                
                // 文本相关
                fontSize: computedStyle.fontSize,
                lineHeight: computedStyle.lineHeight,
                textAlign: computedStyle.textAlign,
                textIndent: computedStyle.textIndent,
                letterSpacing: computedStyle.letterSpacing,
                wordSpacing: computedStyle.wordSpacing,
                whiteSpace: computedStyle.whiteSpace,
                verticalAlign: computedStyle.verticalAlign
            }
        };
    }

    /**
     * 获取文本节点的布局信息（使用 Range）
     * @param {HTMLElement} element 
     * @returns {Array} 文本行信息数组
     */
    function getTextLines(element) {
        const lines = [];
        const textNodes = [];
        
        // 收集所有文本节点
        const walker = document.createTreeWalker(
            element,
            NodeFilter.SHOW_TEXT,
            null,
            false
        );
        
        while (walker.nextNode()) {
            if (walker.currentNode.textContent.trim()) {
                textNodes.push(walker.currentNode);
            }
        }
        
        // 获取每个文本节点的边界
        textNodes.forEach(textNode => {
            const range = document.createRange();
            range.selectNodeContents(textNode);
            const rects = range.getClientRects();
            
            for (let i = 0; i < rects.length; i++) {
                const rect = rects[i];
                lines.push({
                    x: Math.round(rect.left * 100) / 100,
                    y: Math.round(rect.top * 100) / 100,
                    width: Math.round(rect.width * 100) / 100,
                    height: Math.round(rect.height * 100) / 100,
                    text: textNode.textContent.substring(0, 50) // 截取前50字符
                });
            }
        });
        
        return lines;
    }

    /**
     * 提取所有测试元素的布局信息
     * @returns {Object} 完整的布局数据
     */
    function extractAllLayouts() {
        const elements = document.querySelectorAll('[data-test]');
        const layouts = {};
        
        elements.forEach(element => {
            const testId = element.dataset.test;
            layouts[testId] = getElementLayout(element);
            
            // 对于包含文本的元素，获取文本行信息
            if (element.textContent.trim()) {
                layouts[testId].textLines = getTextLines(element);
            }
        });
        
        return {
            timestamp: new Date().toISOString(),
            userAgent: navigator.userAgent,
            viewport: {
                width: window.innerWidth,
                height: window.innerHeight,
                devicePixelRatio: window.devicePixelRatio
            },
            elements: layouts
        };
    }

    /**
     * 格式化输出 JSON
     */
    function outputJSON() {
        const data = extractAllLayouts();
        const json = JSON.stringify(data, null, 2);
        
        console.log('=== Layout Comparison Data ===');
        console.log(json);
        
        // 创建下载链接
        const blob = new Blob([json], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'browser_layout_data.json';
        a.textContent = 'Download Layout Data';
        a.style.cssText = 'position:fixed;top:10px;right:10px;padding:10px 20px;background:#4CAF50;color:white;text-decoration:none;border-radius:5px;z-index:10000;';
        document.body.appendChild(a);
        
        return data;
    }

    /**
     * 创建可视化覆盖层显示元素边界
     */
    function showVisualOverlay() {
        const overlay = document.createElement('div');
        overlay.id = 'layout-overlay';
        overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;pointer-events:none;z-index:9999;';
        
        const elements = document.querySelectorAll('[data-test]');
        elements.forEach(element => {
            const rect = element.getBoundingClientRect();
            const marker = document.createElement('div');
            marker.style.cssText = `
                position:fixed;
                left:${rect.left}px;
                top:${rect.top}px;
                width:${rect.width}px;
                height:${rect.height}px;
                border:2px solid rgba(255,0,0,0.5);
                background:rgba(255,0,0,0.1);
                pointer-events:none;
            `;
            
            const label = document.createElement('span');
            label.textContent = element.dataset.test;
            label.style.cssText = 'position:absolute;top:-18px;left:0;font-size:10px;background:red;color:white;padding:1px 4px;';
            marker.appendChild(label);
            
            overlay.appendChild(marker);
        });
        
        document.body.appendChild(overlay);
    }

    /**
     * 比较两个布局数据（用于后续比较）
     * @param {Object} browserData 浏览器数据
     * @param {Object} mbinkData MBink 数据
     * @returns {Object} 差异报告
     */
    function compareLayouts(browserData, mbinkData) {
        const report = {
            totalElements: 0,
            matchedElements: 0,
            differences: []
        };
        
        const tolerance = 1; // 允许 1px 误差
        
        for (const testId in browserData.elements) {
            report.totalElements++;
            
            if (!mbinkData.elements[testId]) {
                report.differences.push({
                    testId,
                    type: 'missing',
                    message: `Element ${testId} not found in MBink data`
                });
                continue;
            }
            
            const browser = browserData.elements[testId];
            const mbink = mbinkData.elements[testId];
            
            // 比较视口位置
            const diffs = [];
            
            if (Math.abs(browser.viewport.width - mbink.viewport.width) > tolerance) {
                diffs.push(`width: browser=${browser.viewport.width}, mbink=${mbink.viewport.width}`);
            }
            if (Math.abs(browser.viewport.height - mbink.viewport.height) > tolerance) {
                diffs.push(`height: browser=${browser.viewport.height}, mbink=${mbink.viewport.height}`);
            }
            if (Math.abs(browser.viewport.x - mbink.viewport.x) > tolerance) {
                diffs.push(`x: browser=${browser.viewport.x}, mbink=${mbink.viewport.x}`);
            }
            if (Math.abs(browser.viewport.y - mbink.viewport.y) > tolerance) {
                diffs.push(`y: browser=${browser.viewport.y}, mbink=${mbink.viewport.y}`);
            }
            
            if (diffs.length > 0) {
                report.differences.push({
                    testId,
                    type: 'mismatch',
                    details: diffs
                });
            } else {
                report.matchedElements++;
            }
        }
        
        report.matchRate = (report.matchedElements / report.totalElements * 100).toFixed(2) + '%';
        return report;
    }

    // 导出到全局
    window.LayoutExtractor = {
        extract: extractAllLayouts,
        output: outputJSON,
        showOverlay: showVisualOverlay,
        compare: compareLayouts
    };

    // 页面加载完成后自动提取
    if (document.readyState === 'complete') {
        console.log('LayoutExtractor ready. Call LayoutExtractor.output() to get JSON data.');
    } else {
        window.addEventListener('load', function() {
            console.log('LayoutExtractor ready. Call LayoutExtractor.output() to get JSON data.');
        });
    }
})();

