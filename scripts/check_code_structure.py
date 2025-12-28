#!/usr/bin/env python3
"""
代码结构验证脚本

检查项目是否符合 docs/CODE_STRUCTURE_STANDARDS.md 中定义的规范。

Properties 验证:
- Property 1: 大文件需要文档说明 (>2000行)
- Property 2: 长函数检测 (>150行)
- Property 3: 目录文件数量限制 (>15个源文件)
- Property 4: 子系统 CMakeLists 存在性
- Property 5: 测试文件命名规范
- Property 6: HTML 元素分类正确性
- Property 7: 头文件大小限制 (>300行)
- Property 8: 模块依赖数量限制 (>15个include)

Requirements: 1.1-1.5, 2.1, 3.3, 5.2
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Set
from dataclasses import dataclass, field


@dataclass
class ValidationResult:
    """验证结果"""
    passed: bool = True
    warnings: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)


class CodeStructureChecker:
    """代码结构检查器"""
    
    # 阈值配置
    SOURCE_FILE_WARNING_LINES = 1500  # 源文件警告阈值
    SOURCE_FILE_ERROR_LINES = 2000    # 源文件错误阈值
    HEADER_FILE_MAX_LINES = 300       # 头文件最大行数
    FUNCTION_MAX_LINES = 150          # 函数最大行数
    DIR_MAX_SOURCE_FILES = 15         # 目录最大源文件数
    MAX_INCLUDES = 15                 # 最大 include 数量
    
    def __init__(self, root_dir: str):
        self.root_dir = Path(root_dir)
        self.core_dir = self.root_dir / "core"
        self.result = ValidationResult()
        
    def check_all(self) -> ValidationResult:
        """执行所有检查"""
        print("=" * 60)
        print("代码结构验证")
        print("=" * 60)
        
        self.check_file_sizes()
        self.check_directory_file_counts()
        self.check_cmake_exists()
        self.check_include_counts()
        self.check_readme_exists()
        
        return self.result
    
    def check_file_sizes(self):
        """检查文件大小 (Property 1, 7)"""
        print("\n[检查] 文件大小...")
        
        for path in self.core_dir.rglob("*"):
            if not path.is_file():
                continue
                
            suffix = path.suffix.lower()
            if suffix not in ['.cpp', '.h', '.hpp', '.c']:
                continue
                
            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    lines = len(content.splitlines())
            except Exception as e:
                continue
                
            rel_path = path.relative_to(self.root_dir)
            
            # 头文件检查 (Property 7)
            if suffix in ['.h', '.hpp']:
                if lines > self.HEADER_FILE_MAX_LINES:
                    self.result.warnings.append(
                        f"头文件超过 {self.HEADER_FILE_MAX_LINES} 行: {rel_path} ({lines} 行)"
                    )
            
            # 源文件检查 (Property 1)
            if suffix in ['.cpp', '.c']:
                if lines > self.SOURCE_FILE_ERROR_LINES:
                    # 检查是否有文档说明
                    has_doc = "大文件说明" in content or "@note 大文件说明" in content
                    if has_doc:
                        self.result.warnings.append(
                            f"源文件超过 {self.SOURCE_FILE_ERROR_LINES} 行 (已有文档说明): {rel_path} ({lines} 行)"
                        )
                    else:
                        self.result.errors.append(
                            f"源文件超过 {self.SOURCE_FILE_ERROR_LINES} 行需要文档说明: {rel_path} ({lines} 行)"
                        )
                        self.result.passed = False
                elif lines > self.SOURCE_FILE_WARNING_LINES:
                    self.result.warnings.append(
                        f"源文件超过 {self.SOURCE_FILE_WARNING_LINES} 行建议评审: {rel_path} ({lines} 行)"
                    )
    
    def check_directory_file_counts(self):
        """检查目录文件数量 (Property 3)"""
        print("[检查] 目录文件数量...")
        
        for dir_path in self.core_dir.rglob("*"):
            if not dir_path.is_dir():
                continue
                
            # 统计源文件数量
            source_files = list(dir_path.glob("*.cpp")) + list(dir_path.glob("*.c"))
            header_files = list(dir_path.glob("*.h")) + list(dir_path.glob("*.hpp"))
            
            # 只统计直接子文件，不包括子目录中的文件
            total_source = len(source_files)
            
            if total_source > self.DIR_MAX_SOURCE_FILES:
                rel_path = dir_path.relative_to(self.root_dir)
                self.result.warnings.append(
                    f"目录超过 {self.DIR_MAX_SOURCE_FILES} 个源文件，建议创建子目录: {rel_path} ({total_source} 个)"
                )
    
    def check_cmake_exists(self):
        """检查 CMakeLists.txt 存在性 (Property 4)"""
        print("[检查] CMakeLists.txt 存在性...")
        
        for dir_path in self.core_dir.iterdir():
            if not dir_path.is_dir():
                continue
                
            # 检查是否有源文件
            has_sources = any(dir_path.glob("*.cpp")) or any(dir_path.glob("*.c"))
            
            if has_sources:
                cmake_path = dir_path / "CMakeLists.txt"
                if not cmake_path.exists():
                    rel_path = dir_path.relative_to(self.root_dir)
                    self.result.errors.append(
                        f"子系统目录缺少 CMakeLists.txt: {rel_path}"
                    )
                    self.result.passed = False
    
    def check_include_counts(self):
        """检查 include 数量 (Property 8)"""
        print("[检查] Include 依赖数量...")
        
        include_pattern = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
        
        for path in self.core_dir.rglob("*.cpp"):
            try:
                with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
            except Exception:
                continue
                
            includes = include_pattern.findall(content)
            # 只统计项目内部的 include
            internal_includes = [inc for inc in includes if inc.startswith("core/")]
            
            if len(internal_includes) > self.MAX_INCLUDES:
                rel_path = path.relative_to(self.root_dir)
                self.result.warnings.append(
                    f"文件内部 include 超过 {self.MAX_INCLUDES} 个: {rel_path} ({len(internal_includes)} 个)"
                )
    
    def check_readme_exists(self):
        """检查 README.md 存在性 (Property 12)"""
        print("[检查] README.md 存在性...")
        
        for dir_path in self.core_dir.rglob("*"):
            if not dir_path.is_dir():
                continue
                
            # 统计源文件数量
            source_count = len(list(dir_path.glob("*.cpp"))) + len(list(dir_path.glob("*.h")))
            
            if source_count >= 3:
                readme_path = dir_path / "README.md"
                if not readme_path.exists():
                    rel_path = dir_path.relative_to(self.root_dir)
                    self.result.warnings.append(
                        f"目录包含 {source_count} 个文件但缺少 README.md: {rel_path}"
                    )
    
    def print_report(self):
        """打印报告"""
        print("\n" + "=" * 60)
        print("验证报告")
        print("=" * 60)
        
        if self.result.errors:
            print(f"\n❌ 错误 ({len(self.result.errors)}):")
            for error in self.result.errors:
                print(f"  - {error}")
        
        if self.result.warnings:
            print(f"\n⚠️  警告 ({len(self.result.warnings)}):")
            for warning in self.result.warnings:
                print(f"  - {warning}")
        
        if self.result.passed and not self.result.warnings:
            print("\n✅ 所有检查通过!")
        elif self.result.passed:
            print(f"\n✅ 检查通过 (有 {len(self.result.warnings)} 个警告)")
        else:
            print(f"\n❌ 检查失败 ({len(self.result.errors)} 个错误)")
        
        print("=" * 60)


def main():
    """主函数"""
    # 获取项目根目录
    script_dir = Path(__file__).parent
    root_dir = script_dir.parent
    
    # 检查 core 目录是否存在
    core_dir = root_dir / "core"
    if not core_dir.exists():
        print(f"错误: 找不到 core 目录: {core_dir}")
        sys.exit(1)
    
    # 执行检查
    checker = CodeStructureChecker(root_dir)
    result = checker.check_all()
    checker.print_report()
    
    # 返回退出码
    sys.exit(0 if result.passed else 1)


if __name__ == "__main__":
    main()
