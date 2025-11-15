/**
 * @file form_tests.js
 * @brief 表单元素测试
 */

describe('表单元素', () => {
    
    // ========== Input 元素 ==========
    
    test('Input - 创建 text 输入框', () => {
        const input = document.createElement('input');
        input.type = 'text';
        input.value = 'Hello';
        
        assertEqual(input.type, 'text', 'type 应为 text');
        assertEqual(input.value, 'Hello', 'value 应为 Hello');
    });
    
    test('Input - 创建 checkbox', () => {
        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        checkbox.checked = true;
        
        assertEqual(checkbox.type, 'checkbox', 'type 应为 checkbox');
        assertTrue(checkbox.checked, 'checked 应为 true');
    });
    
    test('Input - 创建 radio', () => {
        const radio = document.createElement('input');
        radio.type = 'radio';
        radio.name = 'group1';
        radio.value = 'option1';
        
        assertEqual(radio.type, 'radio', 'type 应为 radio');
        assertEqual(radio.name, 'group1', 'name 应为 group1');
        assertEqual(radio.value, 'option1', 'value 应为 option1');
    });
    
    test('Input - placeholder 属性', () => {
        const input = document.createElement('input');
        input.placeholder = 'Enter text...';
        
        assertEqual(input.placeholder, 'Enter text...', 'placeholder 应为 Enter text...');
    });
    
    test('Input - disabled 属性', () => {
        const input = document.createElement('input');
        input.disabled = true;
        
        assertTrue(input.disabled, 'disabled 应为 true');
    });
    
    test('Input - required 属性', () => {
        const input = document.createElement('input');
        input.required = true;
        
        assertTrue(input.required, 'required 应为 true');
    });
    
    // ========== Textarea 元素 ==========
    
    test('Textarea - 创建文本域', () => {
        const textarea = document.createElement('textarea');
        textarea.value = 'Multi-line\ntext';
        
        assertEqual(textarea.value, 'Multi-line\ntext', 'value 应为 Multi-line\\ntext');
    });
    
    test('Textarea - rows 和 cols 属性', () => {
        const textarea = document.createElement('textarea');
        textarea.rows = 10;
        textarea.cols = 50;
        
        assertEqual(textarea.rows, 10, 'rows 应为 10');
        assertEqual(textarea.cols, 50, 'cols 应为 50');
    });
    
    // ========== Button 元素 ==========
    
    test('Button - 创建按钮', () => {
        const button = document.createElement('button');
        button.textContent = 'Click Me';
        button.type = 'button';
        
        assertEqual(button.textContent, 'Click Me', 'textContent 应为 Click Me');
        assertEqual(button.type, 'button', 'type 应为 button');
    });
    
    test('Button - disabled 属性', () => {
        const button = document.createElement('button');
        button.disabled = true;
        
        assertTrue(button.disabled, 'disabled 应为 true');
    });
    
    // ========== Select 元素 ==========
    
    test('Select - 创建下拉框', () => {
        const select = document.createElement('select');
        
        const option1 = document.createElement('option');
        option1.value = '1';
        option1.textContent = 'Option 1';
        
        const option2 = document.createElement('option');
        option2.value = '2';
        option2.textContent = 'Option 2';
        
        select.appendChild(option1);
        select.appendChild(option2);
        
        assertEqual(select.childNodes.length, 2, '应有 2 个选项');
    });
    
    test('Select - selectedIndex 属性', () => {
        const select = document.createElement('select');
        select.selectedIndex = 1;
        
        assertEqual(select.selectedIndex, 1, 'selectedIndex 应为 1');
    });
    
    // ========== Form 元素 ==========
    
    test('Form - 创建表单', () => {
        const form = document.createElement('form');
        form.action = '/submit';
        form.method = 'POST';
        
        assertEqual(form.action, '/submit', 'action 应为 /submit');
        assertEqual(form.method, 'POST', 'method 应为 POST');
    });
    
    test('Form - 添加表单元素', () => {
        const form = document.createElement('form');
        
        const input = document.createElement('input');
        input.name = 'username';
        
        const button = document.createElement('button');
        button.type = 'submit';
        
        form.appendChild(input);
        form.appendChild(button);
        
        assertEqual(form.childNodes.length, 2, '应有 2 个子元素');
    });
    
    // ========== Label 元素 ==========
    
    test('Label - 创建标签', () => {
        const label = document.createElement('label');
        label.htmlFor = 'input1';
        label.textContent = 'Username:';
        
        assertEqual(label.htmlFor, 'input1', 'htmlFor 应为 input1');
        assertEqual(label.textContent, 'Username:', 'textContent 应为 Username:');
    });
    
    // ========== 表单验证 ==========
    
    test('表单验证 - required 属性', () => {
        const input = document.createElement('input');
        input.required = true;
        input.value = '';
        
        assertTrue(input.required, 'required 应为 true');
        assertEqual(input.value, '', 'value 应为空');
    });
    
    test('表单验证 - pattern 属性', () => {
        const input = document.createElement('input');
        input.pattern = '[0-9]{3}';
        
        assertEqual(input.pattern, '[0-9]{3}', 'pattern 应为 [0-9]{3}');
    });
    
    test('表单验证 - min/max 属性', () => {
        const input = document.createElement('input');
        input.type = 'number';
        input.min = '0';
        input.max = '100';
        
        assertEqual(input.min, '0', 'min 应为 0');
        assertEqual(input.max, '100', 'max 应为 100');
    });
});

