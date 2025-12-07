#!/usr/bin/env python3
"""
Unified Layout Comparison Tool
统一的布局比较工具，同时支持基础测试和高级测试

用法:
    python compare_unified.py <mbink_output.txt> <browser_data.json> [--mode basic|advanced]
"""

import json
import sys
import re
import io
from pathlib import Path
from typing import Dict, List, Any, Optional

# 设置 stdout 为 UTF-8 编码
if sys.platform == 'win32':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

# 允许的误差范围（像素）
WIDTH_TOLERANCE = 1.0
HEIGHT_TOLERANCE = 3.0
X_TOLERANCE = 1.0
Y_TOLERANCE = 3.0

def parse_mbink_output(text: str, mode: str = 'auto') -> Dict[str, Dict]:
    """解析 MBink 布局树输出"""
    elements = {}

    # 统一匹配带 ID 的元素行
    # 格式: [div] #XXX-YY x=0.0 y=42.0 w=398.0 h=107.0 (block) ...
    # 注意 display 可以包含连字符如 inline-block
    pattern = r'\[(\w+)\]\s+#([A-Za-z0-9_-]+)\s+x=([0-9.-]+)\s+y=([0-9.-]+)\s+w=([0-9.]+)\s+h=([0-9.]+)\s+\(([a-z-]+)\)'

    for match in re.finditer(pattern, text):
        tag, elem_id, x, y, w, h, display = match.groups()
        elements[elem_id] = {
            'id': elem_id,
            'tag': tag,
            'x': float(x),
            'y': float(y),
            'width': float(w),
            'height': float(h),
            'display': display
        }

    return elements

def load_browser_data(filepath: str) -> tuple:
    """加载浏览器参考数据，返回 (layouts, mode)"""
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    # 检测格式类型
    if 'layouts' in data:
        # 高级测试格式
        return data['layouts'], 'advanced'
    elif '_meta' in data:
        # 基础测试格式 - 需要转换
        layouts = {}
        for test_id, test_data in data.items():
            if test_id == '_meta':
                continue
            if 'elements' in test_data:
                for elem_id, elem_data in test_data['elements'].items():
                    layouts[elem_id] = {
                        'id': elem_id,
                        'width': elem_data.get('width', 0),
                        'height': elem_data.get('height', 0),
                        'x': elem_data.get('x', elem_data.get('relX', 0)),
                        'y': elem_data.get('y', elem_data.get('relY', 0)),
                    }
        return layouts, 'basic'
    else:
        # 尝试直接作为 layouts
        return data, 'unknown'

def compare_value(browser_val: float, mbink_val: float, tolerance: float) -> tuple:
    """比较单个值，返回 (passed, diff)"""
    diff = abs(browser_val - mbink_val)
    return diff <= tolerance, diff

def compare_layouts(browser_data: Dict, mbink_data: Dict, mode: str) -> Dict:
    """比较两个布局数据集"""
    results = {
        'total': 0,
        'passed': 0,
        'failed': 0,
        'missing_in_mbink': 0,
        'details': []
    }

    for elem_id, browser_elem in browser_data.items():
        results['total'] += 1

        if elem_id not in mbink_data:
            results['missing_in_mbink'] += 1
            results['failed'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'MISSING',
                'message': f"Element not found in MBink output"
            })
            continue

        mbink_elem = mbink_data[elem_id]
        diffs = []
        all_pass = True

        # 比较宽度
        b_width = browser_elem.get('width', 0)
        m_width = mbink_elem.get('width', 0)
        passed, diff = compare_value(b_width, m_width, WIDTH_TOLERANCE)
        if not passed:
            all_pass = False
            diffs.append(f"width: {b_width:.1f} vs {m_width:.1f} (diff={diff:.1f})")

        # 比较高度
        b_height = browser_elem.get('height', 0)
        m_height = mbink_elem.get('height', 0)
        passed, diff = compare_value(b_height, m_height, HEIGHT_TOLERANCE)
        if not passed:
            all_pass = False
            diffs.append(f"height: {b_height:.1f} vs {m_height:.1f} (diff={diff:.1f})")

        # 比较 X - 使用 relX 如果存在（相对坐标）
        # MBink 输出的坐标是相对于父元素的
        if 'relX' in browser_elem:
            b_x = browser_elem['relX']
        else:
            b_x = browser_elem.get('x', 0)
        m_x = mbink_elem.get('x', 0)
        passed, diff = compare_value(b_x, m_x, X_TOLERANCE)
        if not passed:
            all_pass = False
            diffs.append(f"x: {b_x:.1f} vs {m_x:.1f} (diff={diff:.1f})")

        # 比较 Y - 使用 relY 如果存在（相对坐标）
        if 'relY' in browser_elem:
            b_y = browser_elem['relY']
        else:
            b_y = browser_elem.get('y', 0)
        m_y = mbink_elem.get('y', 0)
        passed, diff = compare_value(b_y, m_y, Y_TOLERANCE)
        if not passed:
            all_pass = False
            diffs.append(f"y: {b_y:.1f} vs {m_y:.1f} (diff={diff:.1f})")

        if all_pass:
            results['passed'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'PASS'
            })
        else:
            results['failed'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'FAIL',
                'diffs': diffs
            })

    return results

def print_report(results: Dict, mode: str):
    """打印测试报告"""
    total = results['total']
    passed = results['passed']
    failed = results['failed']
    missing = results['missing_in_mbink']
    
    pass_rate = (passed / total * 100) if total > 0 else 0
    
    # 确定等级
    if pass_rate >= 98:
        grade = "A级 (优秀) ✨"
    elif pass_rate >= 85:
        grade = "B级 (良好) 👍"
    elif pass_rate >= 70:
        grade = "C级 (及格) ⚠️"
    else:
        grade = "D级 (不及格) ❌"
    
    print("\n" + "=" * 70)
    print(f"    {'BASIC' if mode == 'basic' else 'ADVANCED'} LAYOUT COMPARISON REPORT")
    print("=" * 70)
    print(f"\n📊 Summary:")
    print(f"   Total elements:      {total}")
    print(f"   ✅ Passed:           {passed}")
    print(f"   ❌ Failed:           {failed}")
    print(f"   ⚠️  Missing in MBink: {missing}")
    print(f"   📈 Pass rate:        {pass_rate:.1f}%")
    print(f"\n   Tolerances: width/x ±{WIDTH_TOLERANCE}px, height/y ±{HEIGHT_TOLERANCE}px")
    print(f"\n   🏆 Grade: {grade}")
    print("\n" + "=" * 70)
    
    # 打印失败详情
    failures = [d for d in results['details'] if d['status'] != 'PASS']
    if failures and len(failures) <= 20:
        print("\n❌ Failed elements:")
        for detail in failures:
            print(f"\n  [{detail['id']}]")
            if 'message' in detail:
                print(f"    {detail['message']}")
            if 'diffs' in detail:
                for diff in detail['diffs']:
                    print(f"    - {diff}")
    elif failures:
        print(f"\n❌ {len(failures)} elements failed (too many to list)")
        # 只打印前5个
        for detail in failures[:5]:
            print(f"\n  [{detail['id']}]")
            if 'diffs' in detail:
                for diff in detail['diffs']:
                    print(f"    - {diff}")
        print(f"\n  ... and {len(failures) - 5} more")
    
    print()

def main():
    if len(sys.argv) < 3:
        print("Usage: python compare_unified.py <mbink_output.txt> <browser_data.json>")
        sys.exit(1)
    
    mbink_file = sys.argv[1]
    browser_file = sys.argv[2]
    
    # 读取 MBink 输出
    with open(mbink_file, 'r', encoding='utf-8') as f:
        mbink_text = f.read()
    
    # 解析 MBink 数据
    mbink_data = parse_mbink_output(mbink_text)
    print(f"Parsed {len(mbink_data)} elements from MBink output")
    
    # 加载浏览器数据
    browser_data, mode = load_browser_data(browser_file)
    print(f"Loaded {len(browser_data)} elements from browser data ({mode} mode)")
    
    # 比较布局
    results = compare_layouts(browser_data, mbink_data, mode)
    
    # 打印报告
    print_report(results, mode)
    
    # 保存详细报告
    report_file = f"comparison_report_{mode}.json"
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(results, f, ensure_ascii=False, indent=2)
    print(f"Detailed report saved to: {report_file}")
    
    # 返回状态码
    sys.exit(0 if results['failed'] == 0 else 1)

if __name__ == '__main__':
    main()

