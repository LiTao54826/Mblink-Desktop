/**
 * @file hello_world.cpp
 * @brief LightUI C++示例 - Hello World
 * 
 * 功能：
 * - 使用C API创建窗口
 * - 显示简单UI
 * - 演示基本用法
 * 
 * 编译方法：
 *     g++ -o hello_world hello_world.cpp -llightui
 * 
 * 运行方法：
 *     ./hello_world
 * 
 * TODO:
 * - [ ] 实现完整示例
 * - [ ] 添加错误处理
 * - [ ] 添加注释
 */

#include "core/api/lightui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// JavaScript UI代码
const char* UI_CODE = R"(
    import { render } from 'preact';
    import { useState } from 'preact/hooks';
    
    function App() {
        const [count, setCount] = useState(0);
        
        return (
            <div style={{
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'center',
                justifyContent: 'center',
                height: '100vh',
                fontFamily: 'Arial, sans-serif'
            }}>
                <h1>Hello from C++!</h1>
                <p>Welcome to LightUI</p>
                <p>Count: {count}</p>
                <button onClick={() => setCount(count + 1)}>
                    Click Me
                </button>
            </div>
        );
    }
    
    render(<App />, document.body);
)";

int main() {
    // 初始化LightUI
    int result = lightui_init();
    if (result != LIGHTUI_OK) {
        fprintf(stderr, "Failed to initialize LightUI: %d\n", result);
        return 1;
    }
    
    printf("LightUI version: %s\n", lightui_get_version());
    
    // 创建窗口
    LightUIWindowHandle window = lightui_create_window("Hello World", 400, 300);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        lightui_cleanup();
        return 1;
    }
    
    // 加载UI
    result = lightui_load_ui(window, UI_CODE);
    if (result != LIGHTUI_OK) {
        fprintf(stderr, "Failed to load UI: %s\n", lightui_get_last_error());
        lightui_destroy_window(window);
        lightui_cleanup();
        return 1;
    }
    
    // 显示窗口
    lightui_show_window(window);
    
    // 运行事件循环
    printf("Running event loop...\n");
    lightui_run(window);
    
    // 清理
    printf("Cleaning up...\n");
    lightui_destroy_window(window);
    lightui_cleanup();
    
    return 0;
}

