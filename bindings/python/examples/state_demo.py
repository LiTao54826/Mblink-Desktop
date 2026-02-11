"""
LightUI 状态类型演示

演示所有状态类型的 API 用法（纯控制台输出，不创建窗口）。

演示功能：
- IntState: get/set/increment/decrement/multiply
- StringState: get/set/append/prepend/len
- ListState: get/append/pop/shift/unshift/len/索引
- DictState: get/set_key/remove_key/keys/索引/in
- watch/unwatch 状态监听
- batch 批量操作

运行方式：
    python state_demo.py
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import lightui as ui


def demo_int_state(app):
    print("=" * 50)
    print("【IntState 整数状态】")
    counter = app.state("counter", 0)
    print(f"  初始值: {counter.get()}")
    counter.set(10)
    print(f"  set(10): {counter.get()}")
    counter.increment()
    print(f"  increment(): {counter.get()}")
    counter.increment(5)
    print(f"  increment(5): {counter.get()}")
    counter.decrement()
    print(f"  decrement(): {counter.get()}")
    counter.decrement(3)
    print(f"  decrement(3): {counter.get()}")
    counter.multiply(2)
    print(f"  multiply(2): {counter.get()}")
    print()


def demo_string_state(app):
    print("=" * 50)
    print("【StringState 字符串状态】")
    greeting = app.state("greeting", "Hello")
    print(f"  初始值: '{greeting.get()}'")
    greeting.set("你好")
    print(f"  set('你好'): '{greeting.get()}'")
    greeting.append("，世界")
    print(f"  append('，世界'): '{greeting.get()}'")
    greeting.prepend("【")
    print(f"  prepend('【'): '{greeting.get()}'")
    greeting.append("】")
    print(f"  append('】'): '{greeting.get()}'")
    print(f"  len(): {len(greeting)}")
    print()


def demo_list_state(app):
    print("=" * 50)
    print("【ListState 列表状态】")
    items = app.state("items", [])
    print(f"  初始值: {items.get()}")
    items.append("苹果")
    items.append("香蕉")
    items.append("橙子")
    print(f"  append 三个: {items.get()}")
    print(f"  items[0]: '{items[0]}'")
    print(f"  len(): {len(items)}")
    items.pop()
    print(f"  pop(): {items.get()}")
    items.unshift("葡萄")
    print(f"  unshift('葡萄'): {items.get()}")
    items.shift()
    print(f"  shift(): {items.get()}")
    print()


def demo_dict_state(app):
    print("=" * 50)
    print("【DictState 字典状态】")
    config = app.state("config", {})
    config.set_key("theme", "dark")
    config.set_key("language", "zh-CN")
    config.set_key("font_size", 14)
    print(f"  set_key 三个: {config.get()}")
    print(f"  config['theme']: '{config['theme']}'")
    print(f"  'language' in config: {'language' in config}")
    print(f"  keys(): {config.keys()}")
    config.remove_key("font_size")
    print(f"  remove_key('font_size'): {config.get()}")
    print()


def demo_watch(app):
    print("=" * 50)
    print("【watch / unwatch 状态监听】")
    score = app.state("score", 0)

    def on_change(v):
        print(f"    [回调] score = {v}")

    wid = score.watch(on_change)
    print(f"  注册监听 (id={wid})")
    score.increment()
    score.set(100)
    score.unwatch(wid)
    print(f"  取消监听后 increment():")
    score.increment()
    print(f"  当前值: {score.get()}")
    print()


def demo_batch(app):
    print("=" * 50)
    print("【batch 批量操作】")
    amount = app.state("amount", 0)
    count = [0]

    def on_change(v):
        count[0] += 1

    amount.watch(on_change)

    amount.increment()
    amount.increment()
    amount.increment()
    print(f"  不用 batch: 通知 {count[0]} 次")

    count[0] = 0
    with app.batch():
        amount.increment()
        amount.increment()
        amount.increment()
    print(f"  用 batch: 通知 {count[0]} 次")
    print(f"  最终值: {amount.get()}")
    print()


def main():
    print("LightUI 状态类型 API 演示")
    print()
    app = ui.App("状态演示", 400, 300, headless=True, enable_devtools=False)
    demo_int_state(app)
    demo_string_state(app)
    demo_list_state(app)
    demo_dict_state(app)
    demo_watch(app)
    demo_batch(app)
    print("演示完成")


if __name__ == "__main__":
    main()
