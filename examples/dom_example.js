// LightUI DOM API JavaScript 示例
// 演示如何使用 DOM API 构建和操作 DOM 树

// ========== 示例 1: 创建简单的 DOM 结构 ==========

console.log("=== 示例 1: 创建简单的 DOM 结构 ===");

// 创建容器
var container = document.createElement('div');
container.id = 'container';
container.className = 'main-container';

// 创建标题
var title = document.createElement('h1');
title.id = 'title';
var titleText = document.createTextNode('Welcome to LightUI');
title.appendChild(titleText);

// 创建段落
var paragraph = document.createElement('p');
paragraph.className = 'intro';
var paraText = document.createTextNode('This is a lightweight UI framework.');
paragraph.appendChild(paraText);

// 组装 DOM 树
container.appendChild(title);
container.appendChild(paragraph);

console.log("Container created with ID:", container.id);
console.log("Container class:", container.className);

// ========== 示例 2: 属性操作 ==========

console.log("\n=== 示例 2: 属性操作 ===");

var button = document.createElement('button');
button.id = 'myButton';
button.className = 'btn btn-primary';

// 设置自定义属性
button.setAttribute('data-action', 'submit');
button.setAttribute('data-target', '#form');

// 读取属性
console.log("Button ID:", button.id);
console.log("Button class:", button.className);
console.log("Data action:", button.getAttribute('data-action'));
console.log("Data target:", button.getAttribute('data-target'));

// ========== 示例 3: 构建列表 ==========

console.log("\n=== 示例 3: 构建列表 ===");

var list = document.createElement('ul');
list.id = 'itemList';
list.className = 'list';

// 创建 5 个列表项
for (var i = 1; i <= 5; i++) {
    var item = document.createElement('li');
    item.className = 'list-item';
    item.id = 'item-' + i;
    
    var itemText = document.createTextNode('Item ' + i);
    item.appendChild(itemText);
    
    list.appendChild(item);
}

console.log("Created list with 5 items");

// ========== 示例 4: 查询元素 ==========

console.log("\n=== 示例 4: 查询元素 ===");

// 通过 ID 查询（最快）
var foundButton = document.getElementById('myButton');
if (foundButton) {
    console.log("Found button by ID:", foundButton.id);
}

// 通过 QuerySelector 查询
var foundTitle = container.querySelector('#title');
if (foundTitle) {
    console.log("Found title by QuerySelector");
}

// 查询所有列表项
var items = list.querySelectorAll('.list-item');
console.log("Found", items.length, "list items");

// ========== 示例 5: 事件处理 ==========

console.log("\n=== 示例 5: 事件处理 ===");

// 添加点击事件监听器
button.addEventListener('click', function(event) {
    console.log("Button clicked!");
    console.log("Event type:", event.type);
    console.log("Event target:", event.target.id);
});

// 添加鼠标悬停事件
button.addEventListener('mouseover', function(event) {
    console.log("Mouse over button");
});

console.log("Event listeners added to button");

// ========== 示例 6: 事件冒泡 ==========

console.log("\n=== 示例 6: 事件冒泡 ===");

var outer = document.createElement('div');
outer.id = 'outer';
outer.className = 'outer';

var middle = document.createElement('div');
middle.id = 'middle';
middle.className = 'middle';

var inner = document.createElement('div');
inner.id = 'inner';
inner.className = 'inner';

// 组装嵌套结构
middle.appendChild(inner);
outer.appendChild(middle);

// 在每一层添加事件监听器
outer.addEventListener('click', function(event) {
    console.log("Outer div clicked, current target:", event.currentTarget.id);
});

middle.addEventListener('click', function(event) {
    console.log("Middle div clicked, current target:", event.currentTarget.id);
});

inner.addEventListener('click', function(event) {
    console.log("Inner div clicked, current target:", event.currentTarget.id);
    // 可以停止冒泡
    // event.stopPropagation();
});

console.log("Event bubbling example set up");

// ========== 示例 7: 动态修改 DOM ==========

console.log("\n=== 示例 7: 动态修改 DOM ===");

var dynamicContainer = document.createElement('div');
dynamicContainer.id = 'dynamic';

// 添加初始内容
var initialText = document.createTextNode('Initial content');
dynamicContainer.appendChild(initialText);

console.log("Initial content added");

// 添加更多元素
var newPara = document.createElement('p');
var newText = document.createTextNode('Dynamically added paragraph');
newPara.appendChild(newText);
dynamicContainer.appendChild(newPara);

console.log("New paragraph added");

// 移除元素
// dynamicContainer.removeChild(newPara);
// console.log("Paragraph removed");

// ========== 示例 8: 表单元素 ==========

console.log("\n=== 示例 8: 表单元素 ===");

var form = document.createElement('form');
form.id = 'loginForm';
form.className = 'form';

// 用户名输入
var usernameLabel = document.createElement('label');
usernameLabel.setAttribute('for', 'username');
var labelText = document.createTextNode('Username:');
usernameLabel.appendChild(labelText);

var usernameInput = document.createElement('input');
usernameInput.id = 'username';
usernameInput.setAttribute('type', 'text');
usernameInput.setAttribute('name', 'username');
usernameInput.setAttribute('placeholder', 'Enter username');

// 密码输入
var passwordLabel = document.createElement('label');
passwordLabel.setAttribute('for', 'password');
var pwdLabelText = document.createTextNode('Password:');
passwordLabel.appendChild(pwdLabelText);

var passwordInput = document.createElement('input');
passwordInput.id = 'password';
passwordInput.setAttribute('type', 'password');
passwordInput.setAttribute('name', 'password');
passwordInput.setAttribute('placeholder', 'Enter password');

// 提交按钮
var submitButton = document.createElement('button');
submitButton.setAttribute('type', 'submit');
var submitText = document.createTextNode('Login');
submitButton.appendChild(submitText);

// 组装表单
form.appendChild(usernameLabel);
form.appendChild(usernameInput);
form.appendChild(passwordLabel);
form.appendChild(passwordInput);
form.appendChild(submitButton);

console.log("Login form created");

// ========== 示例 9: 卡片网格 ==========

console.log("\n=== 示例 9: 卡片网格 ===");

var grid = document.createElement('div');
grid.id = 'cardGrid';
grid.className = 'grid';

// 创建 3x3 卡片网格
for (var row = 0; row < 3; row++) {
    for (var col = 0; col < 3; col++) {
        var card = document.createElement('div');
        card.className = 'card';
        card.id = 'card-' + row + '-' + col;
        
        var cardTitle = document.createElement('h3');
        var cardTitleText = document.createTextNode('Card ' + (row * 3 + col + 1));
        cardTitle.appendChild(cardTitleText);
        
        var cardBody = document.createElement('p');
        var cardBodyText = document.createTextNode('This is card content');
        cardBody.appendChild(cardBodyText);
        
        card.appendChild(cardTitle);
        card.appendChild(cardBody);
        grid.appendChild(card);
    }
}

console.log("Created 3x3 card grid");

// ========== 示例 10: 导航菜单 ==========

console.log("\n=== 示例 10: 导航菜单 ===");

var nav = document.createElement('nav');
nav.id = 'mainNav';
nav.className = 'navigation';

var navList = document.createElement('ul');
navList.className = 'nav-list';

var menuItems = ['Home', 'About', 'Services', 'Contact'];

for (var i = 0; i < menuItems.length; i++) {
    var navItem = document.createElement('li');
    navItem.className = 'nav-item';
    
    var navLink = document.createElement('a');
    navLink.setAttribute('href', '#' + menuItems[i].toLowerCase());
    navLink.className = 'nav-link';
    
    var linkText = document.createTextNode(menuItems[i]);
    navLink.appendChild(linkText);
    
    navItem.appendChild(navLink);
    navList.appendChild(navItem);
}

nav.appendChild(navList);

console.log("Navigation menu created with", menuItems.length, "items");

// ========== 示例 11: 查询和遍历 ==========

console.log("\n=== 示例 11: 查询和遍历 ===");

// 查询所有卡片
var allCards = grid.querySelectorAll('.card');
console.log("Found", allCards.length, "cards in grid");

// 查询特定卡片
var specificCard = grid.querySelector('#card-1-1');
if (specificCard) {
    console.log("Found specific card:", specificCard.id);
}

// 查询所有导航链接
var navLinks = nav.querySelectorAll('.nav-link');
console.log("Found", navLinks.length, "navigation links");

// ========== 示例 12: 克隆节点 ==========

console.log("\n=== 示例 12: 克隆节点 ===");

// 克隆按钮（浅克隆）
var buttonClone = button.cloneNode(false);
buttonClone.id = 'clonedButton';
console.log("Cloned button (shallow):", buttonClone.id);

// 克隆容器（深克隆）
var containerClone = container.cloneNode(true);
containerClone.id = 'clonedContainer';
console.log("Cloned container (deep):", containerClone.id);

// ========== 总结 ==========

console.log("\n=== 示例完成 ===");
console.log("演示了以下功能:");
console.log("1. 创建元素和文本节点");
console.log("2. 设置和获取属性");
console.log("3. 构建 DOM 树");
console.log("4. 查询元素 (getElementById, querySelector, querySelectorAll)");
console.log("5. 事件处理和事件冒泡");
console.log("6. 动态修改 DOM");
console.log("7. 创建表单");
console.log("8. 创建网格布局");
console.log("9. 创建导航菜单");
console.log("10. 克隆节点");

