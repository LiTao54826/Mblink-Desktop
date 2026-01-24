# LightUI Python + Preact 示例应用

这是一个完整的待办事项应用，展示如何使用 Python 绑定和 Preact 构建交互式桌面应用。

## 功能特性

- ✨ **Preact 组件化开发** - 使用现代前端框架构建 UI
- 🔄 **Python-JS 双向通信** - 前端调用 Python 函数，Python 管理数据
- 💾 **数据持久化** - 自动保存到本地 JSON 文件
- 🎨 **现代 UI 设计** - 渐变色、卡片布局、响应式设计
- 📊 **实时统计** - 显示总计、待完成、已完成数量

## 文件结构

```
bindings/python/examples/
├── preact_todo_app.py      # Python 后端（数据管理、业务逻辑）
├── preact_todo_app.js      # Preact 前端（UI 组件、交互）
├── todos.json              # 数据存储文件（自动生成）
└── README_PREACT.md        # 本文档
```

## 运行示例

### 方法 1：直接运行 Python 脚本

```bash
cd bindings/python/examples
python preact_todo_app.py
```

### 方法 2：使用 Python 模块

```bash
cd bindings/python
python -m examples.preact_todo_app
```

## 应用架构

### Python 后端（preact_todo_app.py）

负责：
- 数据存储和加载（JSON 文件）
- 业务逻辑处理（添加、删除、编辑、统计）
- 提供 API 供前端调用

主要函数：
- `addTodo(text)` - 添加新待办事项
- `toggleTodo(id)` - 切换完成状态
- `deleteTodo(id)` - 删除待办事项
- `editTodo({id, text})` - 编辑待办事项
- `clearCompleted()` - 清除已完成项
- `getTodos()` - 获取所有待办事项
- `getStats()` - 获取统计信息

### Preact 前端（preact_todo_app.js）

负责：
- UI 渲染和交互
- 组件化开发
- 调用 Python API

主要组件：
- `App` - 主应用组件
- `StatsBar` - 统计栏
- `InputSection` - 输入区域
- `FilterTabs` - 过滤标签
- `TodosList` - 待办列表
- `TodoItem` - 单个待办项
- `Footer` - 底部栏

## 技术要点

### 1. Python 调用 JS

```python
# 加载 Preact 库
app.load_js_file('js/preact/preact.js')
app.load_js_file('js/preact/hooks.js')

# 加载应用代码
app.load_js_file('preact_todo_app.js')
```

### 2. JS 调用 Python

```javascript
// 在 JS 中调用 Python 函数
const result = host.call('addTodo', 'Learn LightUI');

// 处理返回结果
if (result && result.success) {
    console.log('添加成功:', result.todo);
}
```

### 3. Python 绑定函数

```python
@app.bind("addTodo")
def add_todo(text):
    # 处理逻辑
    return {"success": True, "todo": new_todo}
```

### 4. Preact 组件

```javascript
function TodoItem({ todo, onToggle, onDelete }) {
    return h('div', { className: 'todo-item' },
        h('input', {
            type: 'checkbox',
            checked: todo.completed,
            onChange: () => onToggle(todo.id)
        }),
        h('span', null, todo.text),
        h('button', { onClick: () => onDelete(todo.id) }, '删除')
    );
}
```

### 5. Hooks 使用

```javascript
function App() {
    const [todos, setTodos] = useState([]);
    const [filter, setFilter] = useState('all');
    
    useEffect(() => {
        // 加载初始数据
        const data = host.call('getTodos');
        setTodos(data);
    }, []);
    
    // ...
}
```

## 数据流

```
用户操作 → Preact 组件 → host.call() → Python 函数 → 数据处理 → 返回结果 → 更新 UI
```

## 扩展建议

1. **添加更多功能**
   - 优先级标记
   - 截止日期
   - 分类标签
   - 搜索过滤

2. **改进 UI**
   - 拖拽排序
   - 动画效果
   - 主题切换
   - 响应式布局

3. **数据同步**
   - 云端同步
   - 多设备同步
   - 导入导出

4. **性能优化**
   - 虚拟滚动
   - 懒加载
   - 缓存策略

## 相关示例

- `sidebar_app.py` - 侧边栏应用示例
- `todo_app.py` - 简单待办应用（不使用 Preact）
- `counter.py` - 计数器示例
- `state_demo.py` - 状态管理示例

## 故障排除

### 问题：应用无法启动

检查：
1. 是否正确安装了 lightui 模块
2. Preact 库文件是否存在（`js/preact/preact.js`）
3. Python 版本是否 >= 3.7

### 问题：数据无法保存

检查：
1. 是否有文件写入权限
2. `todos.json` 文件是否被占用
3. 查看控制台错误信息

### 问题：UI 显示异常

检查：
1. 浏览器控制台是否有 JS 错误
2. Preact 库是否正确加载
3. CSS 样式是否正确应用

## 许可证

本示例遵循 LightUI 项目的许可证。

