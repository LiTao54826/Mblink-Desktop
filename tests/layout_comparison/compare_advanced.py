#!/usr/bin/env python3
"""
Advanced Layout Comparison Tool
比较浏览器和 MBink 的高级布局测试数据

用法:
    python compare_advanced.py <mbink_output.txt> <browser_data.json>
"""

import json
import sys
import re
from pathlib import Path
from typing import Dict, List, Tuple, Any

# 允许的误差范围（像素）
WIDTH_TOLERANCE = 1.0
HEIGHT_TOLERANCE = 3.0
X_TOLERANCE = 1.0
Y_TOLERANCE = 3.0

def parse_mbink_output(text: str) -> Dict[str, Dict]:
    """解析 MBink 布局树输出"""
    elements = {}
    
    # 匹配带 ID 的元素行
    # 格式: [div] #ADV-1-01 x=0.0 y=42.0 w=398.0 h=107.0 (block) ...
    pattern = r'\[(\w+)\]\s+#(ADV-[^\s]+)\s+x=([0-9.]+)\s+y=([0-9.]+)\s+w=([0-9.]+)\s+h=([0-9.]+)\s+\((\w+)\)'
    
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

def compare_layouts(browser_data: Dict, mbink_data: Dict) -> Dict:
    """比较两个布局数据集"""
    browser_layouts = browser_data.get('layouts', browser_data)
    
    results = {
        'total': 0,
        'passed': 0,
        'failed': 0,
        'missing_in_mbink': 0,
        'missing_in_browser': 0,
        'details': []
    }
    
    # 遍历浏览器中的所有元素
    for elem_id, browser_elem in browser_layouts.items():
        results['total'] += 1
        
        if elem_id not in mbink_data:
            results['missing_in_mbink'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'MISSING_IN_MBINK',
                'message': f"Element not found in MBink output"
            })
            continue
        
        mbink_elem = mbink_data[elem_id]
        diffs = []
        
        # 比较宽度
        b_width = browser_elem.get('width', 0)
        m_width = mbink_elem.get('width', 0)
        if abs(b_width - m_width) > WIDTH_TOLERANCE:
            diffs.append(f"width: browser={b_width:.1f}, mbink={m_width:.1f}, diff={abs(b_width - m_width):.1f}")
        
        # 比较高度
        b_height = browser_elem.get('height', 0)
        m_height = mbink_elem.get('height', 0)
        if abs(b_height - m_height) > HEIGHT_TOLERANCE:
            diffs.append(f"height: browser={b_height:.1f}, mbink={m_height:.1f}, diff={abs(b_height - m_height):.1f}")
        
        # 比较相对 X (relX)
        b_relx = browser_elem.get('relX', 0)
        m_x = mbink_elem.get('x', 0)  # MBink 输出的是相对于父元素的坐标
        if abs(b_relx - m_x) > X_TOLERANCE:
            diffs.append(f"relX: browser={b_relx:.1f}, mbink={m_x:.1f}, diff={abs(b_relx - m_x):.1f}")
        
        # 比较相对 Y (relY)
        b_rely = browser_elem.get('relY', 0)
        m_y = mbink_elem.get('y', 0)
        if abs(b_rely - m_y) > Y_TOLERANCE:
            diffs.append(f"relY: browser={b_rely:.1f}, mbink={m_y:.1f}, diff={abs(b_rely - m_y):.1f}")
        
        if diffs:
            results['failed'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'FAILED',
                'diffs': diffs
            })
        else:
            results['passed'] += 1
            results['details'].append({
                'id': elem_id,
                'status': 'PASSED'
            })
    
    # 检查 MBink 中有但浏览器中没有的元素
    for elem_id in mbink_data:
        if elem_id not in browser_layouts:
            results['missing_in_browser'] += 1
    
    return results

def print_report(results: Dict):
    """打印比较报告"""
    print("\n" + "=" * 70)
    print("    ADVANCED LAYOUT COMPARISON REPORT")
    print("=" * 70)
    
    total = results['total']
    passed = results['passed']
    failed = results['failed']
    missing = results['missing_in_mbink']
    
    pass_rate = (passed / total * 100) if total > 0 else 0
    
    print(f"\n📊 Summary:")
    print(f"   Total elements:      {total}")
    print(f"   ✅ Passed:           {passed}")
    print(f"   ❌ Failed:           {failed}")
    print(f"   ⚠️  Missing in MBink: {missing}")
    print(f"   📈 Pass rate:        {pass_rate:.1f}%")
    print(f"\n   Tolerances: width/x ±{WIDTH_TOLERANCE}px, height/y ±{HEIGHT_TOLERANCE}px")
    
    # 确定等级
    if pass_rate >= 95:
        grade = "A级 (优秀) ✨"
    elif pass_rate >= 85:
        grade = "B级 (良好) 👍"
    elif pass_rate >= 70:
        grade = "C级 (及格) 📝"
    else:
        grade = "D级 (不合格) ⚠️"
    
    print(f"\n   🏆 Grade: {grade}")
    
    # 打印失败的详情
    failed_items = [d for d in results['details'] if d['status'] == 'FAILED']
    if failed_items:
        print(f"\n❌ Failed Tests ({len(failed_items)}):")
        print("-" * 70)
        for item in failed_items:
            print(f"\n   [{item['id']}]")
            for diff in item.get('diffs', []):
                print(f"      - {diff}")
    
    # 打印缺失的元素
    missing_items = [d for d in results['details'] if d['status'] == 'MISSING_IN_MBINK']
    if missing_items:
        print(f"\n⚠️  Missing in MBink ({len(missing_items)}):")
        print("-" * 70)
        for item in missing_items[:10]:  # 只显示前10个
            print(f"   - {item['id']}")
        if len(missing_items) > 10:
            print(f"   ... and {len(missing_items) - 10} more")
    
    print("\n" + "=" * 70)
    
    return pass_rate >= 70  # 返回是否及格

def main():
    if len(sys.argv) < 3:
        print("Usage: python compare_advanced.py <mbink_output.txt> <browser_data.json>")
        sys.exit(1)
    
    mbink_file = sys.argv[1]
    browser_file = sys.argv[2]
    
    # 读取 MBink 输出
    with open(mbink_file, 'r', encoding='utf-8') as f:
        mbink_text = f.read()
    
    # 读取浏览器数据
    with open(browser_file, 'r', encoding='utf-8') as f:
        browser_data = json.load(f)
    
    # 解析 MBink 输出
    mbink_data = parse_mbink_output(mbink_text)
    print(f"Parsed {len(mbink_data)} elements from MBink output")
    
    # 比较布局
    results = compare_layouts(browser_data, mbink_data)
    
    # 打印报告
    passed = print_report(results)
    
    # 保存详细报告
    report_file = Path("advanced_comparison_report.json")
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(results, f, indent=2, ensure_ascii=False)
    print(f"\nDetailed report saved to: {report_file}")
    
    sys.exit(0 if passed else 1)

if __name__ == "__main__":
    main()

