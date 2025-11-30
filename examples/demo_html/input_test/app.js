/**
 * 输入框测试应用 - 测试中文输入和快捷键
 */

// 日志组件
function LogPanel(props) {
    var logs = props.logs;
    var logStyle = 'background-color: #1e1e1e; color: #00ff00; padding: 10px; ' +
                   'font-family: Consolas, monospace; font-size: 12px; height: 150px; ' +
                   'overflow-y: auto; border-radius: 4px; margin-top: 10px;';

    var children = [];
    for (var i = 0; i < logs.length; i++) {
        children.push(Preact.h('div', { key: i, style: 'margin-bottom: 2px;' }, logs[i]));
    }

    return Preact.h('div', { style: logStyle }, children);
}

// 主应用
function InputTestApp() {
    var useState = PreactHooks.useState;

    var inputState = useState('');
    var inputValue = inputState[0];
    var setInputValue = inputState[1];

    var logsState = useState(['[启动] 输入框测试应用已加载']);
    var logs = logsState[0];
    var setLogs = logsState[1];

    var charCountState = useState(0);
    var charCount = charCountState[0];
    var setCharCount = charCountState[1];

    var byteCountState = useState(0);
    var byteCount = byteCountState[0];
    var setByteCount = byteCountState[1];

    var addLog = function(msg) {
        var time = new Date().toLocaleTimeString();
        setLogs(function(prev) {
            var newLogs = prev.slice(-20);
            newLogs.push('[' + time + '] ' + msg);
            return newLogs;
        });
    };

    // 计算字符数（简单实现，不使用 TextEncoder）
    var countChars = function(value) {
        // 使用 Array.from 来正确计算 Unicode 字符数
        return Array.from(value).length;
    };

    var countBytes = function(value) {
        // 简单估算 UTF-8 字节数
        var bytes = 0;
        for (var i = 0; i < value.length; i++) {
            var code = value.charCodeAt(i);
            if (code <= 0x7F) {
                bytes += 1;
            } else if (code <= 0x7FF) {
                bytes += 2;
            } else if (code >= 0xD800 && code <= 0xDBFF) {
                // 代理对的高位，跳过低位
                bytes += 4;
                i++;
            } else {
                bytes += 3;
            }
        }
        return bytes;
    };

    var handleInput = function(e) {
        var newValue = e.target.value;
        setInputValue(newValue);
        var chars = countChars(newValue);
        var bytes = countBytes(newValue);
        setCharCount(chars);
        setByteCount(bytes);
        addLog('输入: "' + newValue + '" (' + chars + '字符, ' + bytes + '字节)');
    };

    var handleKeyDown = function(e) {
        var key = e.key;
        var ctrl = e.ctrlKey;

        if (ctrl) {
            if (key === 'a' || key === 'A') {
                addLog('快捷键: Ctrl+A (全选)');
            } else if (key === 'c' || key === 'C') {
                addLog('快捷键: Ctrl+C (复制)');
            } else if (key === 'v' || key === 'V') {
                addLog('快捷键: Ctrl+V (粘贴)');
            } else if (key === 'x' || key === 'X') {
                addLog('快捷键: Ctrl+X (剪切)');
            }
        } else if (key === 'Backspace') {
            addLog('按键: Backspace (删除前一个字符)');
        } else if (key === 'Delete') {
            addLog('按键: Delete (删除后一个字符)');
        }
    };

    var handleClear = function() {
        setInputValue('');
        setCharCount(0);
        setByteCount(0);
        addLog('清空输入框');
    };

    var handleTestChinese = function() {
        var testText = '你好世界';
        setInputValue(testText);
        setCharCount(countChars(testText));
        setByteCount(countBytes(testText));
        addLog('设置测试文本: "' + testText + '"');
    };

    var handleTestMixed = function() {
        var testText = 'Hello你好123';
        setInputValue(testText);
        setCharCount(countChars(testText));
        setByteCount(countBytes(testText));
        addLog('设置混合文本: "' + testText + '"');
    };

    // 样式定义
    var containerStyle = 'padding: 20px; font-family: Microsoft YaHei, Arial, sans-serif; max-width: 600px; margin: 0 auto;';
    var titleStyle = 'color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; margin-bottom: 20px;';
    var infoBoxStyle = 'background-color: #e8f5e9; padding: 15px; border-radius: 8px; margin-bottom: 20px; font-size: 14px;';
    var inputContainerStyle = 'margin-bottom: 20px;';
    var labelStyle = 'display: block; margin-bottom: 8px; font-weight: bold; color: #555;';
    var inputStyle = 'width: 100%; padding: 12px; font-size: 18px; border: 2px solid #4CAF50; border-radius: 6px; box-sizing: border-box;';
    var statsStyle = 'display: flex; gap: 20px; margin-bottom: 20px; padding: 15px; background-color: #f5f5f5; border-radius: 8px;';
    var buttonContainerStyle = 'display: flex; gap: 10px; margin-bottom: 20px; flex-wrap: wrap;';
    var greenBtnStyle = 'padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var blueBtnStyle = 'padding: 10px 20px; background-color: #2196F3; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var redBtnStyle = 'padding: 10px 20px; background-color: #f44336; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var valueBoxStyle = 'margin-bottom: 10px; padding: 10px; background-color: #fff3e0; border-radius: 4px; word-break: break-all;';

    var avgBytesPerChar = charCount > 0 ? (byteCount / charCount).toFixed(2) : '0';

    return Preact.h('div', { style: containerStyle }, [
        // 标题
        Preact.h('h1', { style: titleStyle }, '输入框测试 - 中文和快捷键'),

        // 说明
        Preact.h('div', { style: infoBoxStyle }, [
            Preact.h('p', { style: 'margin: 5px 0; font-weight: bold;' }, '测试功能:'),
            Preact.h('ul', { style: 'margin: 10px 0; padding-left: 20px;' }, [
                Preact.h('li', null, '中文输入和删除'),
                Preact.h('li', null, 'Ctrl+A - 全选'),
                Preact.h('li', null, 'Ctrl+C - 复制'),
                Preact.h('li', null, 'Ctrl+V - 粘贴'),
                Preact.h('li', null, 'Ctrl+X - 剪切'),
                Preact.h('li', null, 'Backspace/Delete - 删除字符')
            ])
        ]),

        // 输入框
        Preact.h('div', { style: inputContainerStyle }, [
            Preact.h('label', { style: labelStyle }, '请输入文字 (支持中文):'),
            Preact.h('input', {
                type: 'text',
                value: inputValue,
                onInput: handleInput,
                onKeyDown: handleKeyDown,
                placeholder: '在此输入中文或英文...',
                style: inputStyle
            })
        ]),

        // 统计信息
        Preact.h('div', { style: statsStyle }, [
            Preact.h('div', null, [
                Preact.h('span', { style: 'font-weight: bold;' }, '字符数: '),
                Preact.h('span', { style: 'color: #4CAF50; font-size: 18px;' }, String(charCount))
            ]),
            Preact.h('div', null, [
                Preact.h('span', { style: 'font-weight: bold;' }, '字节数: '),
                Preact.h('span', { style: 'color: #2196F3; font-size: 18px;' }, String(byteCount))
            ]),
            Preact.h('div', null, [
                Preact.h('span', { style: 'font-weight: bold;' }, '平均字节/字符: '),
                Preact.h('span', { style: 'color: #FF9800; font-size: 18px;' }, avgBytesPerChar)
            ])
        ]),

        // 操作按钮
        Preact.h('div', { style: buttonContainerStyle }, [
            Preact.h('button', { onClick: handleTestChinese, style: greenBtnStyle }, '插入中文测试'),
            Preact.h('button', { onClick: handleTestMixed, style: blueBtnStyle }, '插入混合文本'),
            Preact.h('button', { onClick: handleClear, style: redBtnStyle }, '清空')
        ]),

        // 当前值显示
        Preact.h('div', { style: valueBoxStyle }, [
            Preact.h('strong', null, '当前值: '),
            Preact.h('code', { style: 'color: #e65100;' }, inputValue || '(空)')
        ]),

        // 日志面板
        Preact.h('h3', { style: 'color: #555; margin-top: 20px;' }, '操作日志'),
        Preact.h(LogPanel, { logs: logs })
    ]);
}

// 渲染应用
Preact.render(Preact.h(InputTestApp), document.body);

