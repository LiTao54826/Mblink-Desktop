"""
LightUI Python绑定

功能：
- 提供Python友好的API
- 封装C API
- 装饰器支持
- 类型提示

使用示例：
    import lightui
    
    app = lightui.Window("My App", 800, 600)
    
    @app.bind("getData")
    def get_data():
        return {"message": "Hello from Python!"}
    
    app.load_ui('''
        import { render } from 'preact';
        function App() {
            return <h1>Hello World</h1>;
        }
        render(<App />, document.body);
    ''')
    
    app.run()

TODO:
- [ ] 实现Window类
- [ ] 实现函数绑定装饰器
- [ ] 添加类型提示
- [ ] 添加错误处理
- [ ] 添加文档字符串
"""

__version__ = "0.1.0"
__author__ = "LightUI Team"

from .window import Window

__all__ = ["Window"]

