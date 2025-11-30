/**
 * TextArea多行文本框测试应用
 * 测试功能:
 * 1. 多行文本输入
 * 2. 换行处理 (Enter键)
 * 3. 中文输入
 * 4. 快捷键 (Ctrl+A/C/V/X)
 * 5. 光标导航 (上下左右箭头)
 */

// 日志组件
function LogPanel(props) {
    var logs = props.logs;
    var logStyle = 'background-color: #1e1e1e; color: #00ff00; padding: 10px; ' +
                   'font-family: Consolas, monospace; font-size: 12px; height: 120px; ' +
                   'overflow-y: auto; border-radius: 4px; margin-top: 10px;';

    var children = [];
    for (var i = 0; i < logs.length; i++) {
        children.push(Preact.h('div', { key: i, style: 'margin-bottom: 2px;' }, logs[i]));
    }

    return Preact.h('div', { style: logStyle }, children);
}

// 主应用
function TextAreaTestApp() {
    var useState = PreactHooks.useState;

    // textarea1 状态
    var textarea1State = useState('');
    var textarea1Value = textarea1State[0];
    var setTextarea1Value = textarea1State[1];

    // textarea2 状态 (带初始值)
    var textarea2State = useState('这是一个多行文本\n第二行内容\n第三行内容');
    var textarea2Value = textarea2State[0];
    var setTextarea2Value = textarea2State[1];

    // 日志状态
    var logsState = useState(['[启动] TextArea测试应用已加载']);
    var logs = logsState[0];
    var setLogs = logsState[1];

    // 统计状态
    var statsState = useState({ lines: 0, chars: 0 });
    var stats = statsState[0];
    var setStats = statsState[1];

    var addLog = function(msg) {
        var time = new Date().toLocaleTimeString();
        setLogs(function(prev) {
            var newLogs = prev.slice(-15);
            newLogs.push('[' + time + '] ' + msg);
            return newLogs;
        });
    };

    // 计算行数
    var countLines = function(value) {
        if (!value) return 0;
        return value.split('\n').length;
    };

    // 计算字符数
    var countChars = function(value) {
        return Array.from(value).length;
    };

    var handleTextarea1Input = function(e) {
        var newValue = e.target.value;
        setTextarea1Value(newValue);
        var lines = countLines(newValue);
        var chars = countChars(newValue);
        setStats({ lines: lines, chars: chars });
        addLog('输入框1: ' + lines + '行, ' + chars + '字符');
    };

    var handleTextarea2Input = function(e) {
        var newValue = e.target.value;
        setTextarea2Value(newValue);
        addLog('输入框2已更新');
    };

    var handleKeyDown = function(e) {
        var key = e.key;
        var ctrl = e.ctrlKey;

        if (key === 'Enter') {
            addLog('按键: Enter (换行)');
        } else if (ctrl) {
            if (key === 'a' || key === 'A') {
                addLog('快捷键: Ctrl+A (全选)');
            } else if (key === 'c' || key === 'C') {
                addLog('快捷键: Ctrl+C (复制)');
            } else if (key === 'v' || key === 'V') {
                addLog('快捷键: Ctrl+V (粘贴)');
            } else if (key === 'x' || key === 'X') {
                addLog('快捷键: Ctrl+X (剪切)');
            }
        } else if (key === 'ArrowUp') {
            addLog('导航: 上移');
        } else if (key === 'ArrowDown') {
            addLog('导航: 下移');
        } else if (key === 'Backspace') {
            addLog('按键: Backspace');
        }
    };

    var handleClear = function() {
        setTextarea1Value('');
        setStats({ lines: 0, chars: 0 });
        addLog('已清空输入框1');
    };

    var handleTestMultiline = function() {
        var testText = '第一行文本\n第二行文本\n第三行文本\n第四行文本';
        setTextarea1Value(testText);
        setStats({ lines: countLines(testText), chars: countChars(testText) });
        addLog('插入多行测试文本');
    };

    var handleTestChinese = function() {
        var testText = '你好世界\n这是中文测试\n多行文本框';
        setTextarea1Value(testText);
        setStats({ lines: countLines(testText), chars: countChars(testText) });
        addLog('插入中文测试文本');
    };

    var handleTestLong = function() {
        var lines = [];
        for (var i = 1; i <= 10; i++) {
            lines.push('这是第' + i + '行内容，用于测试滚动功能');
        }
        var testText = lines.join('\n');
        setTextarea1Value(testText);
        setStats({ lines: countLines(testText), chars: countChars(testText) });
        addLog('插入长文本测试');
    };

    // 样式定义
    var containerStyle = 'padding: 20px; font-family: Microsoft YaHei, Arial, sans-serif; max-width: 700px; margin: 0 auto;';
    var titleStyle = 'color: #333; border-bottom: 2px solid #9C27B0; padding-bottom: 10px; margin-bottom: 20px;';
    var infoBoxStyle = 'background-color: #f3e5f5; padding: 15px; border-radius: 8px; margin-bottom: 20px; font-size: 14px;';
    var textareaContainerStyle = 'margin-bottom: 20px;';
    var labelStyle = 'display: block; margin-bottom: 8px; font-weight: bold; color: #555;';
    var textareaStyle = 'width: 100%; height: 120px; padding: 12px; font-size: 16px; border: 2px solid #9C27B0; border-radius: 6px; box-sizing: border-box; resize: vertical; font-family: Microsoft YaHei, Arial, sans-serif;';
    var statsStyle = 'display: flex; gap: 20px; margin-bottom: 20px; padding: 15px; background-color: #f5f5f5; border-radius: 8px;';
    var buttonContainerStyle = 'display: flex; gap: 10px; margin-bottom: 20px; flex-wrap: wrap;';
    var purpleBtnStyle = 'padding: 10px 20px; background-color: #9C27B0; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var blueBtnStyle = 'padding: 10px 20px; background-color: #2196F3; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var greenBtnStyle = 'padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';
    var redBtnStyle = 'padding: 10px 20px; background-color: #f44336; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;';

    return Preact.h('div', { style: containerStyle }, [
        // 标题
        Preact.h('h1', { style: titleStyle }, 'TextArea多行文本框测试'),

        // 说明
        Preact.h('div', { style: infoBoxStyle }, [
            Preact.h('p', { style: 'margin: 5px 0; font-weight: bold;' }, '测试功能:'),
            Preact.h('ul', { style: 'margin: 10px 0; padding-left: 20px;' }, [
                Preact.h('li', null, '多行文本输入'),
                Preact.h('li', null, 'Enter键换行'),
                Preact.h('li', null, '中文输入支持'),
                Preact.h('li', null, '上下左右箭头导航'),
                Preact.h('li', null, 'Ctrl+A/C/V/X 快捷键')
            ])
        ]),

        // 第一个textarea
        Preact.h('div', { style: textareaContainerStyle }, [
            Preact.h('label', { style: labelStyle }, 'TextArea 1 (可编辑):'),
            Preact.h('textarea', {
                value: textarea1Value,
                onInput: handleTextarea1Input,
                onKeyDown: handleKeyDown,
                placeholder: '在此输入多行文本...\n按Enter键换行\n支持中文输入',
                style: textareaStyle
            })
        ]),

        // 统计信息
        Preact.h('div', { style: statsStyle }, [
            Preact.h('div', null, [
                Preact.h('span', { style: 'font-weight: bold;' }, '行数: '),
                Preact.h('span', { style: 'color: #9C27B0; font-size: 18px;' }, String(stats.lines))
            ]),
            Preact.h('div', null, [
                Preact.h('span', { style: 'font-weight: bold;' }, '字符数: '),
                Preact.h('span', { style: 'color: #2196F3; font-size: 18px;' }, String(stats.chars))
            ])
        ]),

        // 操作按钮
        Preact.h('div', { style: buttonContainerStyle }, [
            Preact.h('button', { onClick: handleTestMultiline, style: purpleBtnStyle }, '插入多行文本'),
            Preact.h('button', { onClick: handleTestChinese, style: greenBtnStyle }, '插入中文文本'),
            Preact.h('button', { onClick: handleTestLong, style: blueBtnStyle }, '插入长文本'),
            Preact.h('button', { onClick: handleClear, style: redBtnStyle }, '清空')
        ]),

        // 第二个textarea (带初始值)
        Preact.h('div', { style: textareaContainerStyle }, [
            Preact.h('label', { style: labelStyle }, 'TextArea 2 (带初始值):'),
            Preact.h('textarea', {
                value: textarea2Value,
                onInput: handleTextarea2Input,
                onKeyDown: handleKeyDown,
                style: textareaStyle
            })
        ]),

        // 日志面板
        Preact.h('h3', { style: 'color: #555; margin-top: 20px;' }, '操作日志'),
        Preact.h(LogPanel, { logs: logs })
    ]);
}

// 渲染应用
Preact.render(Preact.h(TextAreaTestApp), document.body);

