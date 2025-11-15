/**
 * @file html_tests.js
 * @brief HTML 内容测试
 */

describe('HTML 内容', () => {
    
    // ========== innerHTML ==========
    
    test('innerHTML - 设置简单 HTML', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<span>Hello</span>';
        
        assertEqual(div.childNodes.length, 1, '应有 1 个子节点');
        assertEqual(div.firstChild.tagName.toLowerCase(), 'span', '第一个子节点应为 span');
        assertEqual(div.firstChild.textContent, 'Hello', '内容应为 Hello');
    });
    
    test('innerHTML - 设置复杂 HTML', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<h1>Title</h1><p>Paragraph</p><ul><li>Item 1</li><li>Item 2</li></ul>';
        
        assertEqual(div.childNodes.length, 3, '应有 3 个子节点');
        assertEqual(div.childNodes[0].tagName.toLowerCase(), 'h1', '第 1 个子节点应为 h1');
        assertEqual(div.childNodes[1].tagName.toLowerCase(), 'p', '第 2 个子节点应为 p');
        assertEqual(div.childNodes[2].tagName.toLowerCase(), 'ul', '第 3 个子节点应为 ul');
    });
    
    test('innerHTML - 设置带属性的 HTML', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<a href="https://example.com" class="link">Link</a>';
        
        const link = div.firstChild;
        assertEqual(link.tagName.toLowerCase(), 'a', '应为 a 元素');
        assertEqual(link.getAttribute('href'), 'https://example.com', 'href 应为 https://example.com');
        assertEqual(link.className, 'link', 'className 应为 link');
        assertEqual(link.textContent, 'Link', '内容应为 Link');
    });
    
    test('innerHTML - 获取 HTML', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Hello</span><p>World</p>';
        
        const html = div.innerHTML;
        
        assertNotEqual(html, '', 'innerHTML 不应为空');
        assertTrue(html.includes('<span>'), 'innerHTML 应包含 <span>');
        assertTrue(html.includes('Hello'), 'innerHTML 应包含 Hello');
        assertTrue(html.includes('<p>'), 'innerHTML 应包含 <p>');
        assertTrue(html.includes('World'), 'innerHTML 应包含 World');
    });
    
    test('innerHTML - 清空内容', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Hello</span><p>World</p>';
        
        div.innerHTML = '';
        
        assertEqual(div.childNodes.length, 0, '应没有子节点');
        assertEqual(div.innerHTML, '', 'innerHTML 应为空');
    });
    
    test('innerHTML - 替换内容', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Old</span>';
        
        div.innerHTML = '<p>New</p>';
        
        assertEqual(div.childNodes.length, 1, '应有 1 个子节点');
        assertEqual(div.firstChild.tagName.toLowerCase(), 'p', '第一个子节点应为 p');
        assertEqual(div.firstChild.textContent, 'New', '内容应为 New');
    });
    
    // ========== outerHTML ==========
    
    test('outerHTML - 获取外部 HTML', () => {
        const div = document.createElement('div');
        div.id = 'test';
        div.className = 'container';
        div.innerHTML = '<span>Hello</span>';
        
        const html = div.outerHTML;
        
        assertNotEqual(html, '', 'outerHTML 不应为空');
        assertTrue(html.includes('<div'), 'outerHTML 应包含 <div');
        assertTrue(html.includes('id="test"'), 'outerHTML 应包含 id="test"');
        assertTrue(html.includes('class="container"'), 'outerHTML 应包含 class="container"');
        assertTrue(html.includes('<span>Hello</span>'), 'outerHTML 应包含 <span>Hello</span>');
    });
    
    test('outerHTML - 设置外部 HTML', () => {
        const parent = document.createElement('div');
        const child = document.createElement('span');
        child.textContent = 'Old';
        parent.appendChild(child);
        
        child.outerHTML = '<p>New</p>';
        
        assertEqual(parent.childNodes.length, 1, '应有 1 个子节点');
        assertEqual(parent.firstChild.tagName.toLowerCase(), 'p', '第一个子节点应为 p');
        assertEqual(parent.firstChild.textContent, 'New', '内容应为 New');
    });
    
    // ========== textContent ==========
    
    test('textContent - 设置文本内容', () => {
        const div = document.createElement('div');
        
        div.textContent = 'Hello World';
        
        assertEqual(div.textContent, 'Hello World', 'textContent 应为 Hello World');
        assertEqual(div.childNodes.length, 1, '应有 1 个子节点（文本节点）');
    });
    
    test('textContent - 获取文本内容', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Hello</span> <p>World</p>';
        
        const text = div.textContent;
        
        assertTrue(text.includes('Hello'), 'textContent 应包含 Hello');
        assertTrue(text.includes('World'), 'textContent 应包含 World');
    });
    
    test('textContent - 清空内容', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Hello</span><p>World</p>';
        
        div.textContent = '';
        
        assertEqual(div.childNodes.length, 0, '应没有子节点');
        assertEqual(div.textContent, '', 'textContent 应为空');
    });
    
    test('textContent - 替换 HTML 为纯文本', () => {
        const div = document.createElement('div');
        div.innerHTML = '<span>Hello</span>';
        
        div.textContent = 'Plain Text';
        
        assertEqual(div.childNodes.length, 1, '应有 1 个子节点（文本节点）');
        assertEqual(div.textContent, 'Plain Text', 'textContent 应为 Plain Text');
        assertFalse(div.innerHTML.includes('<span>'), 'innerHTML 不应包含 <span>');
    });
    
    // ========== 嵌套 HTML ==========
    
    test('嵌套 HTML - 多层嵌套', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<div><div><div><span>Deep</span></div></div></div>';
        
        const deep = div.querySelector('span');
        assertNotNull(deep, '应找到深层嵌套的 span');
        assertEqual(deep.textContent, 'Deep', '内容应为 Deep');
    });
    
    test('嵌套 HTML - 列表结构', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<ul><li>Item 1</li><li>Item 2</li><li>Item 3</li></ul>';
        
        const ul = div.firstChild;
        assertEqual(ul.tagName.toLowerCase(), 'ul', '应为 ul 元素');
        assertEqual(ul.childNodes.length, 3, '应有 3 个 li 元素');
    });
    
    test('嵌套 HTML - 表格结构', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<table><tr><td>Cell 1</td><td>Cell 2</td></tr></table>';
        
        const table = div.firstChild;
        assertEqual(table.tagName.toLowerCase(), 'table', '应为 table 元素');
        
        const cells = div.querySelectorAll('td');
        assertEqual(cells.length, 2, '应有 2 个 td 元素');
    });
    
    // ========== 特殊字符 ==========
    
    test('特殊字符 - HTML 实体', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '&lt;div&gt;';
        
        const text = div.textContent;
        assertTrue(text.includes('<div>'), 'textContent 应包含 <div>');
    });
    
    test('特殊字符 - 引号', () => {
        const div = document.createElement('div');
        
        div.innerHTML = '<span title="Hello &quot;World&quot;">Text</span>';
        
        const span = div.firstChild;
        const title = span.getAttribute('title');
        assertTrue(title.includes('"'), 'title 应包含引号');
    });
});

