#!/usr/bin/env python3
"""
生成MBlink项目结构脚本

功能：
- 批量创建项目文件和目录
- 生成带注释的文件框架
- 创建CMakeLists.txt文件

使用方法：
    python scripts/generate_project_structure.py
"""

import os
from pathlib import Path

# 项目根目录
ROOT_DIR = Path(__file__).parent.parent

# 文件模板
CPP_HEADER_TEMPLATE = """/**
 * @file {filename}
 * @brief {description}
 * 
 * TODO:
 * - [ ] 实现基本功能
 * - [ ] 添加单元测试
 * - [ ] 完善文档
 */

#pragma once

namespace mblink {{

// TODO: 添加类定义和函数声明

}} // namespace mblink
"""

CPP_SOURCE_TEMPLATE = """/**
 * @file {filename}
 * @brief {description}实现
 */

#include "{header_file}"

namespace mblink {{

// TODO: 实现函数

}} // namespace mblink
"""

CMAKE_TEMPLATE = """# {module_name} CMakeLists.txt

# TODO: 添加源文件
# set(SOURCES
#     file1.cpp
#     file2.cpp
# )

# TODO: 添加库或可执行文件
# add_library({module_name} ${{SOURCES}})
# target_link_libraries({module_name} ...)
"""

# 需要创建的文件列表
FILES_TO_CREATE = [
    # Core modules
    ("core/dom/node.cpp", "DOM节点基类实现"),
    ("core/dom/element.cpp", "DOM元素类实现"),
    ("core/dom/text.h", "文本节点类"),
    ("core/dom/text.cpp", "文本节点类实现"),
    ("core/dom/document.h", "Document类"),
    ("core/dom/document.cpp", "Document类实现"),
    ("core/dom/dom_bindings.h", "DOM JavaScript绑定"),
    ("core/dom/dom_bindings.cpp", "DOM JavaScript绑定实现"),
    
    # Event system
    ("core/event/event.h", "事件基类"),
    ("core/event/event.cpp", "事件基类实现"),
    ("core/event/mouse_event.h", "鼠标事件"),
    ("core/event/mouse_event.cpp", "鼠标事件实现"),
    ("core/event/keyboard_event.h", "键盘事件"),
    ("core/event/keyboard_event.cpp", "键盘事件实现"),
    ("core/event/event_system.h", "事件系统"),
    ("core/event/event_system.cpp", "事件系统实现"),
    
    # Layout engine
    ("core/layout/layout_engine.h", "布局引擎"),
    ("core/layout/layout_engine.cpp", "布局引擎实现"),
    ("core/layout/style_parser.h", "样式解析器"),
    ("core/layout/style_parser.cpp", "样式解析器实现"),
    
    # Render engine
    ("core/render/renderer.h", "渲染器"),
    ("core/render/renderer.cpp", "渲染器实现"),
    ("core/render/text_renderer.h", "文本渲染器"),
    ("core/render/text_renderer.cpp", "文本渲染器实现"),
    
    # Bridge
    ("core/bridge/bridge.h", "语言桥接"),
    ("core/bridge/bridge.cpp", "语言桥接实现"),
    
    # C API
    ("core/api/mblink.h", "C API头文件"),
    ("core/api/mblink.cpp", "C API实现"),
    
    # Utils
    ("core/utils/logger.h", "日志工具"),
    ("core/utils/logger.cpp", "日志工具实现"),
    ("core/utils/json.h", "JSON工具"),
    ("core/utils/json.cpp", "JSON工具实现"),
]

# CMakeLists.txt文件
CMAKE_FILES = [
    "core/CMakeLists.txt",
    "core/window/CMakeLists.txt",
    "core/quickjs/CMakeLists.txt",
    "core/dom/CMakeLists.txt",
    "core/event/CMakeLists.txt",
    "core/layout/CMakeLists.txt",
    "core/render/CMakeLists.txt",
    "core/bridge/CMakeLists.txt",
    "core/api/CMakeLists.txt",
    "core/utils/CMakeLists.txt",
    "third_party/CMakeLists.txt",
]

def create_file(filepath, description):
    """创建文件"""
    full_path = ROOT_DIR / filepath
    
    # 如果文件已存在，跳过
    if full_path.exists():
        print(f"跳过（已存在）: {filepath}")
        return
    
    # 确保目录存在
    full_path.parent.mkdir(parents=True, exist_ok=True)
    
    filename = full_path.name
    
    # 根据文件类型生成内容
    if filename.endswith('.h'):
        content = CPP_HEADER_TEMPLATE.format(
            filename=filename,
            description=description
        )
    elif filename.endswith('.cpp'):
        # 查找对应的头文件
        header_file = filename.replace('.cpp', '.h')
        content = CPP_SOURCE_TEMPLATE.format(
            filename=filename,
            description=description,
            header_file=header_file
        )
    else:
        content = f"// {description}\n// TODO: 实现\n"
    
    # 写入文件
    full_path.write_text(content, encoding='utf-8')
    print(f"创建: {filepath}")

def create_cmake_file(filepath):
    """创建CMakeLists.txt文件"""
    full_path = ROOT_DIR / filepath
    
    if full_path.exists():
        print(f"跳过（已存在）: {filepath}")
        return
    
    full_path.parent.mkdir(parents=True, exist_ok=True)
    
    module_name = full_path.parent.name
    content = CMAKE_TEMPLATE.format(module_name=module_name)
    
    full_path.write_text(content, encoding='utf-8')
    print(f"创建: {filepath}")

def main():
    """主函数"""
    print("开始生成MBlink项目结构...\n")
    
    # 创建源文件
    print("创建源文件:")
    for filepath, description in FILES_TO_CREATE:
        create_file(filepath, description)
    
    print("\n创建CMakeLists.txt文件:")
    for filepath in CMAKE_FILES:
        create_cmake_file(filepath)
    
    print("\n完成！")
    print("\n下一步:")
    print("1. 查看生成的文件")
    print("2. 根据需要修改文件内容")
    print("3. 开始实现功能")

if __name__ == "__main__":
    main()

