console.log('=== Adopted StyleSheets Test ===');

// 检查 adoptedStyleSheets 支持
console.log('document.adoptedStyleSheets:', document.adoptedStyleSheets);
console.log('typeof document.adoptedStyleSheets:', typeof document.adoptedStyleSheets);
console.log('window.CSSStyleSheet:', window.CSSStyleSheet);
console.log('typeof window.CSSStyleSheet:', typeof window.CSSStyleSheet);

// 检查 document.head
console.log('document.head:', document.head);

// 尝试创建 CSSStyleSheet
if (window.CSSStyleSheet) {
    try {
        const sheet = new CSSStyleSheet();
        console.log('Created CSSStyleSheet:', sheet);
        
        // 尝试插入规则
        sheet.insertRule('.test { color: red; }', 0);
        console.log('Inserted rule, rules count:', sheet.cssRules?.length);
        
        // 尝试添加到 adoptedStyleSheets
        if (document.adoptedStyleSheets) {
            document.adoptedStyleSheets = [sheet];
            console.log('Set adoptedStyleSheets, length:', document.adoptedStyleSheets.length);
        }
    } catch (e) {
        console.log('Error creating CSSStyleSheet:', e.message);
    }
}

// 测试传统方式
console.log('\n=== Testing traditional style tag ===');
const style = document.createElement('style');
style.textContent = '.test2 { color: blue; }';
document.head.appendChild(style);
console.log('Style tag added');
console.log('Style tags in head:', document.head.querySelectorAll('style').length);

// 检查 style.sheet
console.log('style.sheet:', style.sheet);
if (style.sheet) {
    console.log('style.sheet.cssRules:', style.sheet.cssRules);
}
