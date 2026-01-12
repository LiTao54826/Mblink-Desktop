"""
LightUI Python Binding - 状态管理演示

展示所有状态类型的使用：
- IntState: 整数状态，支持原子操作
- StringState: 字符串状态，支持追加/前置
- ListState: 列表状态，支持数组操作
- DictState: 字典状态，支持对象操作
"""

import sys
import os

# 添加 lightui 包到路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from lightui import LightUIApp


def main():
    print("=" * 60)
    print("LightUI 状态管理演示")
    print("=" * 60)
    
    with LightUIApp("State Demo", 800, 600, headless=True) as app:
        
        # ========== IntState 演示 ==========
        print("\n--- IntState 演示 ---")
        counter = app.state("counter", 0)
        print(f"类型: {type(counter).__name__}")
        print(f"初始值: {counter.get()}")
        
        counter.increment()
        print(f"increment() 后: {counter.get()}")
        
        counter.increment(5)
        print(f"increment(5) 后: {counter.get()}")
        
        counter.multiply(2)
        print(f"multiply(2) 后: {counter.get()}")
        
        counter.set(100)
        print(f"set(100) 后: {counter.get()}")
        
        # ========== StringState 演示 ==========
        print("\n--- StringState 演示 ---")
        message = app.state("message", "Hello")
        print(f"类型: {type(message).__name__}")
        print(f"初始值: '{message.get()}'")
        
        message.append(" World")
        print(f"append(' World') 后: '{message.get()}'")
        
        message.prepend(">>> ")
        print(f"prepend('>>> ') 后: '{message.get()}'")
        
        print(f"字符串长度: {len(message)}")
        
        # ========== ListState 演示 ==========
        print("\n--- ListState 演示 ---")
        items = app.state("items", [1, 2, 3])
        print(f"类型: {type(items).__name__}")
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
        
        items.clear()
        print(f"clear() 后: {items.get()}")
        
        # ========== DictState 演示 ==========
        print("\n--- DictState 演示 ---")
        config = app.state("config", {"theme": "dark", "fontSize": 14})
        print(f"类型: {type(config).__name__}")
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
        
        config.clear()
        print(f"clear() 后: {config.get()}")
        
        # ========== 状态监听演示 ==========
        print("\n--- 状态监听演示 ---")
        score = app.state("score", 0)
        
        changes = []
        def on_score_change(value):
            changes.append(value)
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
        print(f"收到的变化通知: {changes}")
        
        # ========== 批量操作演示 ==========
        print("\n--- 批量操作演示 ---")
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
        with app.batch():
            batch_counter.increment()
            batch_counter.increment()
            batch_counter.increment()
        print(f"  通知次数: {len(notifications)} (批量结束后统一通知)")
        
        batch_counter.unwatch(watch_id)
        print(f"最终 batch_counter: {batch_counter.get()}")
        
        # ========== 类型推断演示 ==========
        print("\n--- 类型推断演示 ---")
        
        int_state = app.state("auto_int", 42)
        print(f"app.state('auto_int', 42) -> {type(int_state).__name__}")
        
        float_state = app.state("auto_float", 3.14)
        print(f"app.state('auto_float', 3.14) -> {type(float_state).__name__}")
        
        str_state = app.state("auto_str", "hello")
        print(f"app.state('auto_str', 'hello') -> {type(str_state).__name__}")
        
        list_state = app.state("auto_list", [1, 2, 3])
        print(f"app.state('auto_list', [1,2,3]) -> {type(list_state).__name__}")
        
        dict_state = app.state("auto_dict", {"a": 1})
        print(f"app.state('auto_dict', {{'a':1}}) -> {type(dict_state).__name__}")
        
        bool_state = app.state("auto_bool", True)
        print(f"app.state('auto_bool', True) -> {type(bool_state).__name__}")
        
        null_state = app.state("auto_null", None)
        print(f"app.state('auto_null', None) -> {type(null_state).__name__}")
        
        # ========== 状态代理身份演示 ==========
        print("\n--- 状态代理身份演示 ---")
        state1 = app.state("shared", 100)
        state2 = app.state("shared", 999)  # 初始值被忽略
        
        print(f"state1 is state2: {state1 is state2}")
        print(f"state1.get(): {state1.get()}")
        print(f"state2.get(): {state2.get()}")
        
        state1.set(200)
        print(f"state1.set(200) 后:")
        print(f"  state1.get(): {state1.get()}")
        print(f"  state2.get(): {state2.get()}")
        
        print("\n" + "=" * 60)
        print("演示完成!")
        print("=" * 60)


if __name__ == "__main__":
    main()
