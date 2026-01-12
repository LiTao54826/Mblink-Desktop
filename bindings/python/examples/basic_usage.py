"""
LightUI Python Binding - 基础使用示例

演示如何使用 LightUI Python 绑定进行状态管理。
"""

import sys
import os

# 添加构建目录到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../build/python/Release'))

import lightui_core as lui

def main():
    print("=" * 50)
    print("LightUI Python Binding 示例")
    print(f"版本: {lui.version()}")
    print("=" * 50)
    
    # 创建应用
    app = lui.App("Demo App", 800, 600)
    
    # ========== 1. IntState 示例 ==========
    print("\n--- IntState 示例 ---")
    counter = app.state("counter", 0)
    print(f"初始值: {counter.get()}")
    
    counter.increment()
    print(f"increment() 后: {counter.get()}")
    
    counter.increment(5)
    print(f"increment(5) 后: {counter.get()}")
    
    counter.multiply(2)
    print(f"multiply(2) 后: {counter.get()}")
    
    counter.set(100)
    print(f"set(100) 后: {counter.get()}")
    
    # ========== 2. StringState 示例 ==========
    print("\n--- StringState 示例 ---")
    message = app.state("message", "Hello")
    print(f"初始值: '{message.get()}'")
    
    message.append(" World")
    print(f"append(' World') 后: '{message.get()}'")
    
    message.prepend(">>> ")
    print(f"prepend('>>> ') 后: '{message.get()}'")
    
    print(f"字符串长度: {len(message)}")
    
    # ========== 3. ListState 示例 ==========
    print("\n--- ListState 示例 ---")
    items = app.state("items", [1, 2, 3])
    print(f"初始值: {items.get()}")
    
    items.append(4)
    print(f"append(4) 后: {items.get()}")
    
    items[0] = 10
    print(f"items[0] = 10 后: {items.get()}")
    
    print(f"items[1] = {items[1]}")
    
    items.pop()
    print(f"pop() 后: {items.get()}")
    
    items.remove(1)  # 移除索引 1 的元素
    print(f"remove(1) 后: {items.get()}")
    
    print(f"列表长度: {len(items)}")
    
    # ========== 4. DictState 示例 ==========
    print("\n--- DictState 示例 ---")
    config = app.state("config", {"theme": "dark", "fontSize": 14})
    print(f"初始值: {config.get()}")
    
    config.set_key("language", "zh-CN")
    print(f"set_key('language', 'zh-CN') 后: {config.get()}")
    
    config["debug"] = True
    print(f"config['debug'] = True 后: {config.get()}")
    
    print(f"config['theme'] = {config['theme']}")
    print(f"'debug' in config: {'debug' in config}")
    print(f"'unknown' in config: {'unknown' in config}")
    print(f"所有键: {config.keys()}")
    
    config.remove_key("debug")
    print(f"remove_key('debug') 后: {config.get()}")
    
    # ========== 5. 状态监听示例 ==========
    print("\n--- 状态监听示例 ---")
    score = app.state("score", 0)
    
    def on_score_change(value):
        print(f"  [回调] score 变化为: {value}")
    
    watch_id = score.watch(on_score_change)
    print("已添加监听器")
    
    print("执行 score.increment(10):")
    score.increment(10)
    
    print("执行 score.increment(20):")
    score.increment(20)
    
    score.unwatch(watch_id)
    print("已移除监听器")
    
    print("执行 score.increment(30) (无回调):")
    score.increment(30)
    print(f"最终 score: {score.get()}")
    
    # ========== 6. 批量操作示例 ==========
    print("\n--- 批量操作示例 ---")
    batch_counter = app.state("batch_counter", 0)
    
    notifications = []
    def on_batch_change(value):
        notifications.append(value)
    
    watch_id = batch_counter.watch(on_batch_change)
    
    print("普通模式下 3 次 increment:")
    batch_counter.increment()
    batch_counter.increment()
    batch_counter.increment()
    print(f"  通知次数: {len(notifications)}")
    
    notifications.clear()
    
    print("批量模式下 3 次 increment:")
    with lui.BatchContext(app):
        batch_counter.increment()
        batch_counter.increment()
        batch_counter.increment()
    print(f"  通知次数: {len(notifications)} (批量结束后统一通知)")
    
    batch_counter.unwatch(watch_id)
    print(f"最终 batch_counter: {batch_counter.get()}")
    
    # ========== 7. 类型推断示例 ==========
    print("\n--- 类型推断示例 ---")
    
    int_state = app.state("auto_int", 42)
    print(f"app.state('auto_int', 42) -> {type(int_state).__name__}")
    
    str_state = app.state("auto_str", "hello")
    print(f"app.state('auto_str', 'hello') -> {type(str_state).__name__}")
    
    list_state = app.state("auto_list", [1, 2, 3])
    print(f"app.state('auto_list', [1,2,3]) -> {type(list_state).__name__}")
    
    dict_state = app.state("auto_dict", {"a": 1})
    print(f"app.state('auto_dict', {{'a':1}}) -> {type(dict_state).__name__}")
    
    # ========== 8. 状态代理身份示例 ==========
    print("\n--- 状态代理身份示例 ---")
    state1 = app.state("shared", 100)
    state2 = app.state("shared", 999)  # 初始值被忽略
    
    print(f"state1 is state2: {state1 is state2}")
    print(f"state1.get(): {state1.get()}")
    print(f"state2.get(): {state2.get()}")
    
    state1.set(200)
    print(f"state1.set(200) 后:")
    print(f"  state1.get(): {state1.get()}")
    print(f"  state2.get(): {state2.get()}")
    
    print("\n" + "=" * 50)
    print("示例完成!")
    print("=" * 50)


if __name__ == "__main__":
    main()
