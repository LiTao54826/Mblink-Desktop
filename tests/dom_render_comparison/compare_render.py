#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DOM 渲染比较工具（基于 data-test 属性的扁平化结构）
比较浏览器和 MBink 的渲染结果

用法:
    python compare_render.py browser.json mbink.json
    python compare_render.py browser.json mbink.json --test BASIC-001
    python compare_render.py browser.json mbink.json --tolerance 2
"""

import argparse
import json
import sys
import re
import io
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from pathlib import Path

# 设置 stdout 编码为 utf-8
if sys.platform == 'win32':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')


@dataclass
class CompareResult:
    """比较结果"""
    passed: bool
    test_id: str
    field: str
    expected: Any
    actual: Any
    diff: float = 0.0


class LayoutComparator:
    """布局数据比较器（基于 data-test 属性）"""

    def __init__(self, tolerance: float = 2.0, use_relative: bool = False):
        self.tolerance = tolerance
        self.use_relative = use_relative  # 是否使用相对坐标模式
        self.results: List[CompareResult] = []
        self.total_elements = 0
        self.matched_elements = 0
        # 用于相对坐标计算的父子关系映射
        self.browser_elements: Dict = {}
        self.mbink_elements: Dict = {}

    def _find_parent_id(self, test_id: str, elements: Dict) -> Optional[str]:
        """根据命名约定找到父元素 ID
        例如: BASIC-001-content 的父元素是 case-BASIC-001
              BASIC-002-inner 的父元素可能是 BASIC-002-outer
        """
        # 尝试找 case-XXX 作为父元素
        parts = test_id.split('-')
        if len(parts) >= 2:
            # 如果是 case-XXX 或 test-container，不计算相对坐标（跳过）
            if parts[0] == 'case' or test_id == 'test-container':
                return None

            # 否则，尝试找 case-XXX 作为父元素
            case_id = f"case-{parts[0]}-{parts[1]}"
            if case_id in elements:
                return case_id

        return None

    def _get_relative_coords(self, test_id: str, elem: Dict, elements: Dict) -> Dict:
        """计算相对于父元素的坐标"""
        vp = elem.get('viewport', {})
        x = vp.get('x', 0)
        y = vp.get('y', 0)

        parent_id = self._find_parent_id(test_id, elements)
        if parent_id and parent_id in elements:
            parent_vp = elements[parent_id].get('viewport', {})
            parent_x = parent_vp.get('x', 0)
            parent_y = parent_vp.get('y', 0)
            return {
                'rel_x': x - parent_x,
                'rel_y': y - parent_y,
                'width': vp.get('width', 0),
                'height': vp.get('height', 0)
            }

        # 没有父元素，返回原始坐标
        return {
            'rel_x': x,
            'rel_y': y,
            'width': vp.get('width', 0),
            'height': vp.get('height', 0)
        }

    def compare(self, browser_data: Dict, mbink_data: Dict) -> List[CompareResult]:
        """比较两个渲染数据"""
        self.results = []
        self.total_elements = 0
        self.matched_elements = 0

        self.browser_elements = browser_data.get('elements', {})
        self.mbink_elements = mbink_data.get('elements', {})

        # 遍历浏览器中的所有元素
        for test_id, browser_elem in self.browser_elements.items():
            self.total_elements += 1

            if test_id not in self.mbink_elements:
                self.results.append(CompareResult(
                    passed=False, test_id=test_id, field="existence",
                    expected="present", actual="missing"
                ))
                continue

            mbink_elem = self.mbink_elements[test_id]
            element_passed = self._compare_element(test_id, browser_elem, mbink_elem)

            if element_passed:
                self.matched_elements += 1

        # 检查 MBink 中有但浏览器中没有的元素
        for test_id in self.mbink_elements:
            if test_id not in self.browser_elements:
                self.results.append(CompareResult(
                    passed=False, test_id=test_id, field="existence",
                    expected="missing", actual="present (extra)"
                ))

        return self.results

    def _compare_element(self, test_id: str, browser_elem: Dict, mbink_elem: Dict) -> bool:
        """比较单个元素的布局"""
        element_passed = True

        if self.use_relative:
            # 使用相对坐标模式
            browser_coords = self._get_relative_coords(test_id, browser_elem, self.browser_elements)
            mbink_coords = self._get_relative_coords(test_id, mbink_elem, self.mbink_elements)

            # 对于 case-XXX 和 test-container，只比较尺寸（跳过坐标）
            # 因为这些是测试用例容器，它们的 y 坐标会累积误差
            is_case_container = test_id.startswith('case-') or test_id == 'test-container'

            if is_case_container:
                # 只比较 width 和 height
                fields = [('width', 'width'), ('height', 'height')]
            else:
                # 比较相对坐标和尺寸
                fields = [('rel_x', 'rel_x'), ('rel_y', 'rel_y'), ('width', 'width'), ('height', 'height')]

            for field_name, key in fields:
                browser_val = browser_coords.get(key, 0)
                mbink_val = mbink_coords.get(key, 0)
                diff = abs(browser_val - mbink_val)

                if diff > self.tolerance:
                    element_passed = False
                    self.results.append(CompareResult(
                        passed=False, test_id=test_id, field=field_name,
                        expected=browser_val, actual=mbink_val, diff=diff
                    ))
        else:
            # 使用全局坐标模式（原有行为）
            browser_vp = browser_elem.get('viewport', {})
            mbink_vp = mbink_elem.get('viewport', {})

            # 比较 x, y, width, height
            for field in ['x', 'y', 'width', 'height']:
                browser_val = browser_vp.get(field, 0)
                mbink_val = mbink_vp.get(field, 0)
                diff = abs(browser_val - mbink_val)

                if diff > self.tolerance:
                    element_passed = False
                    self.results.append(CompareResult(
                        passed=False, test_id=test_id, field=field,
                        expected=browser_val, actual=mbink_val, diff=diff
                    ))

        return element_passed

    def get_summary(self) -> Dict:
        """获取比较摘要"""
        failures = [r for r in self.results if not r.passed]
        match_rate = (self.matched_elements / self.total_elements * 100) if self.total_elements > 0 else 0

        return {
            'totalElements': self.total_elements,
            'matchedElements': self.matched_elements,
            'differences': len(failures),
            'matchRate': f"{match_rate:.2f}%",
            'tolerance': self.tolerance
        }


def extract_from_output(filepath: str) -> Dict:
    """从文件中提取 JSON 数据"""
    # 使用 utf-8-sig 编码来处理可能的 BOM
    with open(filepath, 'r', encoding='utf-8-sig') as f:
        content = f.read()

    # 查找 __RENDER_DATA__ 标记
    match = re.search(r'__RENDER_DATA__(.+)', content)
    if match:
        json_str = match.group(1).strip()
        return json.loads(json_str)

    # 尝试直接解析整个文件
    return json.loads(content)


def print_results(comparator: LayoutComparator, verbose: bool = False):
    """打印比较结果"""
    summary = comparator.get_summary()
    failures = [r for r in comparator.results if not r.passed]

    print("\n" + "=" * 60)
    print("    DOM RENDER COMPARISON REPORT")
    print("=" * 60)

    print(f"\n📊 Summary:")
    print(f"   Total elements:    {summary['totalElements']}")
    print(f"   Matched:           {summary['matchedElements']}")
    print(f"   Differences:       {summary['differences']}")
    print(f"   Match rate:        {summary['matchRate']}")
    print(f"   Tolerance:         {summary['tolerance']}px")

    if failures:
        print(f"\n❌ Differences ({len(failures)}):")
        print("-" * 60)

        # 按 test_id 分组显示
        current_test_id = None
        for r in sorted(failures, key=lambda x: x.test_id):
            if r.test_id != current_test_id:
                current_test_id = r.test_id
                print(f"\n   [{r.test_id}]")

            if r.field == "existence":
                print(f"      🔴 Element {r.actual}")
            else:
                print(f"      {r.field}: browser={r.expected}, mbink={r.actual}, diff={r.diff:.2f}px")
    else:
        print(f"\n✅ All elements match within {summary['tolerance']}px tolerance!")

    print("\n" + "=" * 60)

    return len(failures) == 0


def main():
    parser = argparse.ArgumentParser(description='Compare DOM render data (data-test based)')
    parser.add_argument('browser_json', help='Browser reference JSON file')
    parser.add_argument('mbink_json', help='MBink output JSON file')
    parser.add_argument('--test', '-t', help='Filter by test ID pattern (e.g., BASIC-001)')
    parser.add_argument('--tolerance', type=float, default=2.0, help='Position/size tolerance in pixels')
    parser.add_argument('--relative', '-r', action='store_true',
                        help='Use relative coordinates (relative to parent) instead of global coordinates')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--report', choices=['text', 'json'], default='text')
    parser.add_argument('-o', '--output', help='Output file')

    args = parser.parse_args()

    try:
        # 加载数据
        browser_data = extract_from_output(args.browser_json)
        mbink_data = extract_from_output(args.mbink_json)

        # 筛选测试用例
        if args.test:
            pattern = args.test.upper()
            browser_elements = browser_data.get('elements', {})
            mbink_elements = mbink_data.get('elements', {})

            browser_data['elements'] = {k: v for k, v in browser_elements.items() if pattern in k.upper()}
            mbink_data['elements'] = {k: v for k, v in mbink_elements.items() if pattern in k.upper()}

        # 比较
        comparator = LayoutComparator(tolerance=args.tolerance, use_relative=args.relative)
        results = comparator.compare(browser_data, mbink_data)

        # 输出结果
        if args.report == 'json':
            output = json.dumps({
                'summary': comparator.get_summary(),
                'differences': [{
                    'testId': r.test_id,
                    'field': r.field,
                    'expected': r.expected,
                    'actual': r.actual,
                    'diff': r.diff
                } for r in results if not r.passed]
            }, indent=2)

            if args.output:
                with open(args.output, 'w') as f:
                    f.write(output)
            else:
                print(output)
        else:
            success = print_results(comparator, args.verbose)
            return 0 if success else 1

    except FileNotFoundError as e:
        print(f"Error: File not found - {e}")
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON - {e}")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())

