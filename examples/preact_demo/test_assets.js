// 测试嵌入资源的透明加载
const { h, render } = preact;
const { useState, useEffect } = preactHooks;

function App() {
    const [assetList, setAssetList] = useState([]);
    
    useEffect(() => {
        // 列出所有嵌入的资源
        if (typeof listAssets === 'function') {
            const assets = listAssets();
            setAssetList(assets);
            console.log('嵌入资源列表:', assets);
        }
    }, []);
    
    return h('div', { style: 'padding: 20px; font-family: sans-serif;' }, [
        h('h1', { style: 'color: #333;' }, '资源透明加载测试'),
        
        // 显示嵌入资源列表
        h('div', { style: 'margin: 20px 0; padding: 10px; background: #f5f5f5; border-radius: 4px;' }, [
            h('h3', null, '嵌入资源列表:'),
            assetList.length > 0 
                ? h('ul', null, assetList.map(path => h('li', { key: path }, path)))
                : h('p', null, '(无嵌入资源或非嵌入模式)')
        ]),
        
        // 测试图片加载 - 使用相对路径（与未打包时一致）
        h('div', { style: 'margin: 20px 0;' }, [
            h('h3', null, '图片测试 (src="assets/logo.png"):'),
            h('p', { style: 'color: #666; font-size: 12px;' }, 
                '使用相对路径 assets/logo.png，打包和未打包时路径一致'),
            h('img', { 
                src: 'assets/logo.png', 
                style: 'max-width: 200px; border: 1px solid #ccc;',
                onError: (e) => {
                    e.target.style.display = 'none';
                    console.log('图片加载失败: assets/logo.png');
                }
            })
        ]),
        
        // 使用 JS API 获取资源
        h('div', { style: 'margin: 20px 0;' }, [
            h('h3', null, 'JS API 测试:'),
            h('button', {
                style: 'padding: 8px 16px; margin-right: 10px;',
                onClick: () => {
                    if (typeof hasAsset === 'function') {
                        alert('hasAsset("assets/style.css"): ' + hasAsset('assets/style.css'));
                    } else {
                        alert('hasAsset 函数不可用');
                    }
                }
            }, '检查 assets/style.css'),
            h('button', {
                style: 'padding: 8px 16px;',
                onClick: () => {
                    if (typeof loadAsset === 'function') {
                        const data = loadAsset('assets/test.txt');
                        if (data) {
                            const text = new TextDecoder().decode(data);
                            alert('assets/test.txt 内容:\n' + text);
                        } else {
                            alert('资源不存在');
                        }
                    } else {
                        alert('loadAsset 函数不可用');
                    }
                }
            }, '读取 assets/test.txt')
        ]),
        
        h('p', { style: 'margin-top: 30px; color: #999; font-size: 12px;' }, 
            '提示: 使用 app_bundler --assets assets 打包资源后，图片和CSS会自动从嵌入资源加载')
    ]);
}

render(h(App), document.body);
