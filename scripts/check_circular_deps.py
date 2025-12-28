#!/usr/bin/env python3
"""
循环依赖检测脚本

检测 C++ 项目中的循环 include 依赖。

Property 9: 循环依赖检测
Validates: Requirements 12.2
"""

import os
import re
import sys
from pathlib import Path
from typing import Dict, Set, List, Tuple
from collections import defaultdict


class CircularDependencyChecker:
    """循环依赖检查器"""
    
    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir)
        self.core_dir = self.root_dir / "core"
        
        # 依赖图: file -> set of included files
        self.dependencies: Dict[str, Set[str]] = defaultdict(set)
        
        # 文件路径映射: filename -> full path
        self.file_paths: Dict[str, str] = {}
        
        # Include 正则
        self.include_pattern = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
        
    def build_dependency_graph(self):
        """构建依赖图"""
        print("构建依赖图...")
        
        # 收集所有头文件
        for path in self.core_dir.rglob("*.h"):
            rel_path = str(path.relative_to(self.root_dir)).replace("\\", "/")
            filename = path.name
            self.file_paths[filename] = rel_path
            self.file_paths[rel_path] = rel_path
            
        # 分析每个文件的依赖
        for path in self.core_dir.rglob("*.h"):
            self._analyze_file(path)
            
        for path in self.core_dir.rglob("*.cpp"):
            self._analyze_file(path)
            
    def _analyze_file(self, path: Path):
        """分析单个文件的依赖"""
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception:
            return
            
        rel_path = str(path.relative_to(self.root_dir)).replace("\\", "/")
        includes = self.include_pattern.findall(content)
        
        for inc in includes:
            # 标准化路径
            inc = inc.replace("\\", "/")
            
            # 只关注项目内部的 include
            if inc.startswith("core/") or inc in self.file_paths:
                # 解析相对路径
                resolved = self._resolve_include(path, inc)
                if resolved:
                    self.dependencies[rel_path].add(resolved)
                    
    def _resolve_include(self, from_file: Path, include: str) -> str:
        """解析 include 路径"""
        include = include.replace("\\", "/")
        
        # 如果是完整路径
        if include in self.file_paths:
            return self.file_paths[include]
            
        # 如果是相对路径
        if include.startswith("core/"):
            return include
            
        # 尝试从文件名查找
        filename = Path(include).name
        if filename in self.file_paths:
            return self.file_paths[filename]
            
        return None
        
    def find_cycles(self) -> List[List[str]]:
        """查找所有循环依赖"""
        print("检测循环依赖...")
        
        cycles = []
        visited = set()
        rec_stack = set()
        path = []
        
        def dfs(node: str) -> bool:
            visited.add(node)
            rec_stack.add(node)
            path.append(node)
            
            for neighbor in self.dependencies.get(node, []):
                if neighbor not in visited:
                    if dfs(neighbor):
                        return True
                elif neighbor in rec_stack:
                    # 找到循环
                    cycle_start = path.index(neighbor)
                    cycle = path[cycle_start:] + [neighbor]
                    cycles.append(cycle)
                    
            path.pop()
            rec_stack.remove(node)
            return False
            
        for node in list(self.dependencies.keys()):
            if node not in visited:
                dfs(node)
                
        return cycles
        
    def find_module_cycles(self) -> List[Tuple[str, str]]:
        """查找模块级别的循环依赖"""
        print("检测模块级循环依赖...")
        
        # 构建模块依赖图
        module_deps: Dict[str, Set[str]] = defaultdict(set)
        
        for file, deps in self.dependencies.items():
            # 提取模块名 (core/xxx/)
            parts = file.split("/")
            if len(parts) >= 2 and parts[0] == "core":
                from_module = parts[1]
                
                for dep in deps:
                    dep_parts = dep.split("/")
                    if len(dep_parts) >= 2 and dep_parts[0] == "core":
                        to_module = dep_parts[1]
                        if from_module != to_module:
                            module_deps[from_module].add(to_module)
                            
        # 查找模块间的双向依赖
        bidirectional = []
        checked = set()
        
        for module, deps in module_deps.items():
            for dep in deps:
                pair = tuple(sorted([module, dep]))
                if pair not in checked:
                    checked.add(pair)
                    if module in module_deps.get(dep, set()):
                        bidirectional.append((module, dep))
                        
        return bidirectional
        
    def print_report(self, cycles: List[List[str]], module_cycles: List[Tuple[str, str]]):
        """打印报告"""
        print("\n" + "=" * 60)
        print("循环依赖检测报告")
        print("=" * 60)
        
        if module_cycles:
            print(f"\n⚠️  模块间双向依赖 ({len(module_cycles)}):")
            for m1, m2 in module_cycles:
                print(f"  - core/{m1}/ <-> core/{m2}/")
                
        if cycles:
            print(f"\n❌ 文件级循环依赖 ({len(cycles)}):")
            for i, cycle in enumerate(cycles[:10]):  # 只显示前10个
                print(f"  {i+1}. {' -> '.join(cycle)}")
            if len(cycles) > 10:
                print(f"  ... 还有 {len(cycles) - 10} 个循环")
                
        if not cycles and not module_cycles:
            print("\n✅ 未检测到循环依赖!")
        else:
            print(f"\n总计: {len(cycles)} 个文件级循环, {len(module_cycles)} 个模块级双向依赖")
            
        print("=" * 60)


def main():
    """主函数"""
    script_dir = Path(__file__).parent
    root_dir = script_dir.parent
    
    core_dir = root_dir / "core"
    if not core_dir.exists():
        print(f"错误: 找不到 core 目录: {core_dir}")
        sys.exit(1)
        
    checker = CircularDependencyChecker(root_dir)
    checker.build_dependency_graph()
    
    cycles = checker.find_cycles()
    module_cycles = checker.find_module_cycles()
    
    checker.print_report(cycles, module_cycles)
    
    # 如果有循环依赖，返回非零退出码
    sys.exit(1 if cycles else 0)


if __name__ == "__main__":
    main()
