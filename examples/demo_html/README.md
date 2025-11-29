# HTML Test Files

统一存放所有可在浏览器中测试的 HTML 文件。

## 使用方法

### 浏览器测试
直接在浏览器中打开 HTML 文件即可：
- `preact_counter.html` - 计数器示例
- `preact_todo.html` - Todo 应用
- `preact_form.html` - 表单验证示例

### MBink 运行
```bash
# 运行编译好的示例
./build/bin/Release/preact_counter.exe
./build/bin/Release/preact_todo_app.exe
./build/bin/Release/preact_form_demo.exe
```

## 对比测试
1. 在浏览器中打开 HTML 文件
2. 运行对应的 MBink 可执行文件
3. 对比两者的渲染结果

## 注意事项
- HTML 文件使用 CDN 加载 Preact (https://unpkg.com/preact@10.19.3)
- MBink 使用内置的 `js/preact/preact.js`
- 两者应该产生相同的渲染结果

