#!/usr/bin/env python3
"""
Layout Comparison Tool
比较浏览器和 MBink 的布局数据

用法:
    python compare_layouts.py browser_layout_data.json mbink_layout_data.json
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Any

# 允许的误差范围（像素）
TOLERANCE = 2.0

class LayoutComparator:
    def __init__(self, tolerance: float = TOLERANCE):
        self.tolerance = tolerance
        self.total_elements = 0
        self.matched_elements = 0
        self.differences: List[Dict[str, Any]] = []
    
    def load_json(self, filepath: str) -> Dict:
        """加载 JSON 文件"""
        with open(filepath, 'r', encoding='utf-8') as f:
            return json.load(f)
    
    def compare_value(self, name: str, browser_val: float, mbink_val: float) -> Tuple[bool, str]:
        """比较单个值"""
        diff = abs(browser_val - mbink_val)
        if diff > self.tolerance:
            return False, f"{name}: browser={browser_val:.2f}, mbink={mbink_val:.2f}, diff={diff:.2f}"
        return True, ""
    
    def compare_element(self, test_id: str, browser_elem: Dict, mbink_elem: Dict) -> List[str]:
        """比较单个元素的布局"""
        diffs = []
        
        browser_vp = browser_elem.get('viewport', {})
        mbink_vp = mbink_elem.get('viewport', {})
        
        # 比较 x, y, width, height
        for key in ['x', 'y', 'width', 'height']:
            browser_val = browser_vp.get(key, 0)
            mbink_val = mbink_vp.get(key, 0)
            
            match, diff_msg = self.compare_value(key, browser_val, mbink_val)
            if not match:
                diffs.append(diff_msg)
        
        return diffs
    
    def compare(self, browser_data: Dict, mbink_data: Dict) -> Dict:
        """比较两个布局数据集"""
        browser_elements = browser_data.get('elements', {})
        mbink_elements = mbink_data.get('elements', {})
        
        self.total_elements = 0
        self.matched_elements = 0
        self.differences = []
        
        # 遍历浏览器中的所有元素
        for test_id, browser_elem in browser_elements.items():
            self.total_elements += 1
            
            if test_id not in mbink_elements:
                self.differences.append({
                    'testId': test_id,
                    'type': 'missing',
                    'message': f"Element '{test_id}' not found in MBink data"
                })
                continue
            
            mbink_elem = mbink_elements[test_id]
            diffs = self.compare_element(test_id, browser_elem, mbink_elem)
            
            if diffs:
                self.differences.append({
                    'testId': test_id,
                    'type': 'mismatch',
                    'details': diffs
                })
            else:
                self.matched_elements += 1
        
        # 检查 MBink 中有但浏览器中没有的元素
        for test_id in mbink_elements:
            if test_id not in browser_elements:
                self.differences.append({
                    'testId': test_id,
                    'type': 'extra',
                    'message': f"Element '{test_id}' exists in MBink but not in browser"
                })
        
        # 生成报告
        match_rate = (self.matched_elements / self.total_elements * 100) if self.total_elements > 0 else 0
        
        return {
            'summary': {
                'totalElements': self.total_elements,
                'matchedElements': self.matched_elements,
                'mismatchedElements': len([d for d in self.differences if d['type'] == 'mismatch']),
                'missingElements': len([d for d in self.differences if d['type'] == 'missing']),
                'extraElements': len([d for d in self.differences if d['type'] == 'extra']),
                'matchRate': f"{match_rate:.2f}%",
                'tolerance': self.tolerance
            },
            'differences': self.differences
        }
    
    def print_report(self, report: Dict):
        """打印比较报告"""
        summary = report['summary']
        
        print("\n" + "=" * 60)
        print("    LAYOUT COMPARISON REPORT")
        print("=" * 60)
        
        print(f"\n📊 Summary:")
        print(f"   Total elements:    {summary['totalElements']}")
        print(f"   Matched:          {summary['matchedElements']}")
        print(f"   Mismatched:       {summary['mismatchedElements']}")
        print(f"   Missing in MBink: {summary['missingElements']}")
        print(f"   Extra in MBink:   {summary['extraElements']}")
        print(f"   Match rate:       {summary['matchRate']}")
        print(f"   Tolerance:        {summary['tolerance']}px")
        
        if report['differences']:
            print(f"\n❌ Differences ({len(report['differences'])}):")
            print("-" * 60)
            
            for diff in report['differences']:
                test_id = diff['testId']
                diff_type = diff['type']
                
                if diff_type == 'missing':
                    print(f"   🔴 [{test_id}] MISSING - {diff['message']}")
                elif diff_type == 'extra':
                    print(f"   🟡 [{test_id}] EXTRA - {diff['message']}")
                elif diff_type == 'mismatch':
                    print(f"   🟠 [{test_id}] MISMATCH:")
                    for detail in diff['details']:
                        print(f"      - {detail}")
        else:
            print(f"\n✅ All elements match within {summary['tolerance']}px tolerance!")
        
        print("\n" + "=" * 60)
        
        # 返回是否全部匹配
        return len(report['differences']) == 0

def main():
    if len(sys.argv) < 3:
        print("Usage: python compare_layouts.py <browser_data.json> <mbink_data.json>")
        print("\nExample:")
        print("  python compare_layouts.py browser_layout_data.json mbink_layout_data.json")
        sys.exit(1)
    
    browser_file = sys.argv[1]
    mbink_file = sys.argv[2]
    
    # 可选的容差参数
    tolerance = TOLERANCE
    if len(sys.argv) > 3:
        try:
            tolerance = float(sys.argv[3])
        except ValueError:
            print(f"Warning: Invalid tolerance value '{sys.argv[3]}', using default {TOLERANCE}")
    
    comparator = LayoutComparator(tolerance)
    
    try:
        browser_data = comparator.load_json(browser_file)
        mbink_data = comparator.load_json(mbink_file)
    except FileNotFoundError as e:
        print(f"Error: File not found - {e.filename}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON - {e}")
        sys.exit(1)
    
    report = comparator.compare(browser_data, mbink_data)
    
    # 保存报告
    report_file = Path("layout_comparison_report.json")
    with open(report_file, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=2, ensure_ascii=False)
    print(f"Report saved to: {report_file}")
    
    # 打印报告
    all_match = comparator.print_report(report)
    
    # 返回退出码
    sys.exit(0 if all_match else 1)

if __name__ == "__main__":
    main()

