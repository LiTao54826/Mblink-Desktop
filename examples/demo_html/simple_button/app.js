// 简单按钮示例
var h = preact.h;
var render = preact.render;
var useState = preactHooks.useState;

function App() {
    var state = useState(0);
    var count = state[0];
    var setCount = state[1];

    return h('div', {
        style: 'display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; background-color: #f0f0f0; font-family: Arial, sans-serif;'
    }, [
        h('h1', {
            style: 'color: #333; margin-bottom: 20px;'
        }, 'Click Count: ' + count),

        h('button', {
            style: 'padding: 15px 40px; font-size: 18px; background-color: #4CAF50; color: white; border: none; border-radius: 8px; cursor: pointer;',
            onclick: function() {
                setCount(count + 1);
            }
        }, 'Click Me!')
    ]);
}

render(h(App), document.body);

