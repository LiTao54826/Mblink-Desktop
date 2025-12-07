#!/usr/bin/env python3
"""
DOM 渲染比较工具
比较浏览器和 MBink 的渲染结果

用法:
    python compare_render.py browser.json mbink.json
    python compare_render.py browser.json mbink.json --test BASIC-001
    python compare_render.py browser.json mbink.json --category basic
    python compare_render.py browser.json mbink.json --tolerance 2
"""

import argparse
import json
import sys
import re
from typing import Dict, List, Any, Optional, Tuple
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CompareResult:
    """比较结果"""
    passed: bool
    path: str
    field: str
    expected: Any
    actual: Any
    diff: float = 0.0


class RenderComparer:
    """渲染数据比较器"""
    
    def __init__(self, tolerance: float = 2.0, color_tolerance: int = 5):
        self.tolerance = tolerance
        self.color_tolerance = color_tolerance
        self.results: List[CompareResult] = []
    
    def compare(self, browser_data: Dict, mbink_data: Dict) -> List[CompareResult]:
        """比较两个渲染数据"""
        self.results = []
        self._compare_element(browser_data, mbink_data, "root")
        return self.results
    
    def _compare_element(self, expected: Dict, actual: Dict, path: str):
        """递归比较元素"""
        if expected is None or actual is None:
            if expected != actual:
                self.results.append(CompareResult(
                    passed=False, path=path, field="element",
                    expected=expected, actual=actual
                ))
            return
        
        # 比较标签名
        if expected.get('tag') != actual.get('tag'):
            self.results.append(CompareResult(
                passed=False, path=path, field="tag",
                expected=expected.get('tag'), actual=actual.get('tag')
            ))
        
        # 比较布局
        self._compare_layout(expected.get('layout', {}), actual.get('layout', {}), path)
        
        # 比较盒模型
        self._compare_box(expected.get('box', {}), actual.get('box', {}), path)
        
        # 比较样式
        self._compare_style(expected.get('style', {}), actual.get('style', {}), path)
        
        # 递归比较子元素
        expected_children = expected.get('children', [])
        actual_children = actual.get('children', [])
        
        if len(expected_children) != len(actual_children):
            self.results.append(CompareResult(
                passed=False, path=path, field="children.length",
                expected=len(expected_children), actual=len(actual_children)
            ))
        
        for i, (exp_child, act_child) in enumerate(zip(expected_children, actual_children)):
            child_path = f"{path}.children[{i}]"
            self._compare_element(exp_child, act_child, child_path)
    
    def _compare_layout(self, expected: Dict, actual: Dict, path: str):
        """比较布局数据"""
        for field in ['x', 'y', 'width', 'height']:
            exp_val = expected.get(field, 0)
            act_val = actual.get(field, 0)
            diff = abs(exp_val - act_val)
            passed = diff <= self.tolerance
            
            if not passed:
                self.results.append(CompareResult(
                    passed=False, path=path, field=f"layout.{field}",
                    expected=exp_val, actual=act_val, diff=diff
                ))
    
    def _compare_box(self, expected: Dict, actual: Dict, path: str):
        """比较盒模型数据"""
        for field in ['marginTop', 'marginRight', 'marginBottom', 'marginLeft',
                      'paddingTop', 'paddingRight', 'paddingBottom', 'paddingLeft',
                      'borderTop', 'borderRight', 'borderBottom', 'borderLeft']:
            exp_val = expected.get(field, 0)
            act_val = actual.get(field, 0)
            diff = abs(exp_val - act_val)
            passed = diff <= self.tolerance
            
            if not passed:
                self.results.append(CompareResult(
                    passed=False, path=path, field=f"box.{field}",
                    expected=exp_val, actual=act_val, diff=diff
                ))
    
    def _compare_style(self, expected: Dict, actual: Dict, path: str):
        """比较样式数据"""
        for field in ['display', 'position', 'textAlign']:
            exp_val = expected.get(field, '')
            act_val = actual.get(field, '')
            
            if exp_val != act_val:
                self.results.append(CompareResult(
                    passed=False, path=path, field=f"style.{field}",
                    expected=exp_val, actual=act_val
                ))


def load_json_file(filepath: str) -> Dict:
    """加载 JSON 文件"""
    with open(filepath, 'r', encoding='utf-8') as f:
        return json.load(f)


def extract_from_output(filepath: str) -> Dict:
    """从 MBink 输出中提取 JSON 数据"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 查找 __RENDER_DATA__ 标记
    match = re.search(r'__RENDER_DATA__(.+)', content)
    if match:
        json_str = match.group(1).strip()
        return json.loads(json_str)
    
    # 尝试直接解析整个文件
    return json.loads(content)


def filter_by_test_id(data: Dict, test_id: str) -> Optional[Dict]:
    """根据测试 ID 筛选数据"""
    def find_element(element: Dict, target_id: str) -> Optional[Dict]:
        element_id = element.get('id', '')
        if element_id and target_id in element_id:
            return element
        
        for child in element.get('children', []):
            result = find_element(child, target_id)
            if result:
                return result
        return None
    
    return find_element(data, f"test-{test_id}")


def print_results(results: List[CompareResult], verbose: bool = False):
    """打印比较结果"""
    failures = [r for r in results if not r.passed]
    
    if not failures:
        print("✅ All comparisons passed!")
        return True
    
    print(f"❌ {len(failures)} differences found:\n")
    
    for r in failures:
        print(f"  [{r.path}] {r.field}")
        print(f"    Expected: {r.expected}")
        print(f"    Actual:   {r.actual}")
        if r.diff > 0:
            print(f"    Diff:     {r.diff:.2f}px")
        print()
    
    return False


def main():
    parser = argparse.ArgumentParser(description='Compare DOM render data')
    parser.add_argument('browser_json', help='Browser reference JSON file')
    parser.add_argument('mbink_json', help='MBink output JSON file')
    parser.add_argument('--test', '-t', help='Filter by test ID (e.g., BASIC-001)')
    parser.add_argument('--category', '-c', help='Filter by category (e.g., basic, layout)')
    parser.add_argument('--tolerance', type=float, default=2.0, help='Position/size tolerance in pixels')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--quiet', '-q', action='store_true', help='Quiet output')
    parser.add_argument('--report', choices=['text', 'json', 'html'], default='text')
    parser.add_argument('-o', '--output', help='Output file')
    
    args = parser.parse_args()
    
    try:
        # 加载数据
        browser_data = extract_from_output(args.browser_json)
        mbink_data = extract_from_output(args.mbink_json)
        
        # 筛选测试用例
        if args.test:
            browser_data = filter_by_test_id(browser_data, args.test)
            mbink_data = filter_by_test_id(mbink_data, args.test)
            
            if not browser_data:
                print(f"Error: Test {args.test} not found in browser data")
                return 1
            if not mbink_data:
                print(f"Error: Test {args.test} not found in MBink data")
                return 1
        
        # 比较
        comparer = RenderComparer(tolerance=args.tolerance)
        results = comparer.compare(browser_data, mbink_data)
        
        # 输出结果
        if args.report == 'json':
            output = json.dumps([{
                'passed': r.passed,
                'path': r.path,
                'field': r.field,
                'expected': r.expected,
                'actual': r.actual,
                'diff': r.diff
            } for r in results], indent=2)
            
            if args.output:
                with open(args.output, 'w') as f:
                    f.write(output)
            else:
                print(output)
        else:
            success = print_results(results, args.verbose)
            
            # 汇总
            failures = len([r for r in results if not r.passed])
            if not args.quiet:
                print(f"\nSummary: {len(results) - failures}/{len(results)} checks passed")
            
            return 0 if success else 1
    
    except FileNotFoundError as e:
        print(f"Error: File not found - {e}")
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON - {e}")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())

