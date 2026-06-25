#!/usr/bin/env python3
"""
MBlink 布局系统测试运行脚本

功能：
1. 运行 IFC 单元测试
2. 运行基础布局比较测试
3. 运行高级布局比较测试
4. 生成统一测试报告
5. 检查通过率阈值

用法：
    python scripts/run_layout_tests.py [--build] [--verbose]
"""

import os
import sys
import subprocess
import json
import argparse
from pathlib import Path
from datetime import datetime

# 配置
CONFIG = {
    'ifc_test': 'build/bin/Release/test_ifc.exe',
    'layout_test': 'build/bin/Release/layout_compare_test.exe',
    'compare_script': 'tests/layout_comparison/compare_unified.py',
    'basic_browser_data': 'tests/layout_comparison/browser_reference_data.json',
    'advanced_browser_data': 'tests/layout_comparison/browser_advanced_data.json',
    'basic_output': 'tests/layout_comparison/mblink_output.txt',
    'advanced_output': 'tests/layout_comparison/mblink_advanced_output.txt',
    'pass_rate_threshold': 98.0,  # 最低通过率要求
}

class LayoutTestRunner:
    def __init__(self, project_root: str, verbose: bool = False):
        self.project_root = Path(project_root)
        self.verbose = verbose
        self.results = {
            'timestamp': datetime.now().isoformat(),
            'ifc_tests': None,
            'basic_tests': None,
            'advanced_tests': None,
            'overall_pass_rate': 0.0,
            'status': 'PENDING'
        }

    def run_command(self, cmd: list, cwd: str = None, capture: bool = True) -> tuple:
        """运行命令并返回输出"""
        if self.verbose:
            print(f"  Running: {' '.join(cmd)}")

        try:
            result = subprocess.run(
                cmd,
                cwd=cwd or str(self.project_root),
                capture_output=capture,
                text=True,
                encoding='utf-8',
                errors='replace',
                timeout=300
            )
            return result.returncode, result.stdout or "", result.stderr or ""
        except subprocess.TimeoutExpired:
            return -1, "", "Command timed out"
        except Exception as e:
            return -1, "", str(e)

    def build_tests(self) -> bool:
        """编译测试程序"""
        print("\n📦 Building tests...")
        
        targets = ['test_ifc', 'layout_compare_test']
        for target in targets:
            cmd = ['cmake', '--build', 'build', '--config', 'Release', '--target', target]
            ret, stdout, stderr = self.run_command(cmd)
            if ret != 0:
                print(f"  ❌ Failed to build {target}")
                if self.verbose:
                    print(stderr)
                return False
            print(f"  ✅ Built {target}")
        
        return True

    def run_ifc_tests(self) -> dict:
        """运行 IFC 单元测试"""
        print("\n🧪 Running IFC unit tests...")
        
        test_exe = self.project_root / CONFIG['ifc_test']
        if not test_exe.exists():
            return {'status': 'SKIP', 'message': 'test_ifc.exe not found'}
        
        ret, stdout, stderr = self.run_command([str(test_exe)])
        
        # 解析 GTest 输出
        total = 0
        passed = 0
        for line in stdout.split('\n'):
            if '[==========]' in line and 'tests from' in line:
                import re
                match = re.search(r'Running (\d+) tests', line)
                if match:
                    total = int(match.group(1))
            if '[  PASSED  ]' in line:
                match = re.search(r'(\d+) tests?', line)
                if match:
                    passed = int(match.group(1))
        
        result = {
            'status': 'PASS' if ret == 0 else 'FAIL',
            'total': total,
            'passed': passed,
            'pass_rate': (passed / total * 100) if total > 0 else 0
        }
        
        print(f"  Total: {total}, Passed: {passed}, Rate: {result['pass_rate']:.1f}%")
        print(f"  Status: {'✅ PASS' if result['status'] == 'PASS' else '❌ FAIL'}")
        
        return result

    def run_layout_comparison(self, mode: str) -> dict:
        """运行布局比较测试"""
        is_advanced = mode == 'advanced'
        mode_str = "高级" if is_advanced else "基础"
        print(f"\n📐 Running {mode_str} layout comparison tests...")
        
        # 确定文件路径
        layout_exe = self.project_root / CONFIG['layout_test']
        output_file = self.project_root / (CONFIG['advanced_output'] if is_advanced else CONFIG['basic_output'])
        browser_data = self.project_root / (CONFIG['advanced_browser_data'] if is_advanced else CONFIG['basic_browser_data'])
        compare_script = self.project_root / CONFIG['compare_script']
        
        if not layout_exe.exists():
            return {'status': 'SKIP', 'message': 'layout_compare_test.exe not found'}
        
        # 生成 MBlink 输出
        cmd = [str(layout_exe)]
        if is_advanced:
            cmd.append('--advanced')
        cmd.append('-q')
        
        ret, stdout, stderr = self.run_command(cmd)
        if ret != 0 and self.verbose:
            print(f"  Warning: layout_compare_test returned {ret}")
        
        # 保存输出
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(stdout)
        
        # 运行比较脚本
        cmd = ['python', str(compare_script), str(output_file), str(browser_data)]
        ret, stdout, stderr = self.run_command(cmd)
        
        # 解析比较结果
        result = {
            'status': 'PASS' if ret == 0 else 'FAIL',
            'total': 0,
            'passed': 0,
            'failed': 0,
            'pass_rate': 0.0
        }
        
        for line in stdout.split('\n'):
            if 'Total elements:' in line:
                import re
                match = re.search(r'(\d+)', line)
                if match:
                    result['total'] = int(match.group(1))
            elif '✅ Passed:' in line:
                match = re.search(r'(\d+)', line)
                if match:
                    result['passed'] = int(match.group(1))
            elif '❌ Failed:' in line:
                match = re.search(r'(\d+)', line)
                if match:
                    result['failed'] = int(match.group(1))
            elif 'Pass rate:' in line:
                match = re.search(r'([0-9.]+)%', line)
                if match:
                    result['pass_rate'] = float(match.group(1))
        
        print(f"  Total: {result['total']}, Passed: {result['passed']}, Failed: {result['failed']}")
        print(f"  Pass rate: {result['pass_rate']:.1f}%")
        print(f"  Status: {'✅ PASS' if result['pass_rate'] >= CONFIG['pass_rate_threshold'] else '❌ FAIL'}")
        
        return result

    def run_all_tests(self, build: bool = False) -> bool:
        """运行所有测试"""
        print("=" * 60)
        print("    MBlink Layout System Test Suite")
        print("=" * 60)
        
        # 可选：先编译
        if build:
            if not self.build_tests():
                self.results['status'] = 'BUILD_FAILED'
                return False
        
        # 运行 IFC 单元测试
        self.results['ifc_tests'] = self.run_ifc_tests()
        
        # 运行基础布局测试
        self.results['basic_tests'] = self.run_layout_comparison('basic')
        
        # 运行高级布局测试
        self.results['advanced_tests'] = self.run_layout_comparison('advanced')
        
        # 计算总体通过率
        total = 0
        passed = 0
        for key in ['ifc_tests', 'basic_tests', 'advanced_tests']:
            if self.results[key] and self.results[key].get('total', 0) > 0:
                total += self.results[key]['total']
                passed += self.results[key]['passed']
        
        self.results['overall_pass_rate'] = (passed / total * 100) if total > 0 else 0
        
        # 判断整体状态
        all_pass = all([
            self.results['ifc_tests'] and self.results['ifc_tests']['status'] == 'PASS',
            self.results['basic_tests'] and self.results['basic_tests']['pass_rate'] >= CONFIG['pass_rate_threshold'],
            self.results['advanced_tests'] and self.results['advanced_tests']['pass_rate'] >= CONFIG['pass_rate_threshold'],
        ])
        
        self.results['status'] = 'PASS' if all_pass else 'FAIL'
        
        # 打印总结
        self.print_summary()
        
        return all_pass

    def print_summary(self):
        """打印测试总结"""
        print("\n" + "=" * 60)
        print("    TEST SUMMARY")
        print("=" * 60)
        
        print(f"\n📊 Results:")
        
        if self.results['ifc_tests']:
            ifc = self.results['ifc_tests']
            status = '✅' if ifc['status'] == 'PASS' else '❌'
            print(f"   {status} IFC Unit Tests:     {ifc['passed']}/{ifc['total']} ({ifc['pass_rate']:.1f}%)")
        
        if self.results['basic_tests']:
            basic = self.results['basic_tests']
            status = '✅' if basic['pass_rate'] >= CONFIG['pass_rate_threshold'] else '❌'
            print(f"   {status} Basic Layout Tests: {basic['passed']}/{basic['total']} ({basic['pass_rate']:.1f}%)")
        
        if self.results['advanced_tests']:
            adv = self.results['advanced_tests']
            status = '✅' if adv['pass_rate'] >= CONFIG['pass_rate_threshold'] else '❌'
            print(f"   {status} Advanced Tests:     {adv['passed']}/{adv['total']} ({adv['pass_rate']:.1f}%)")
        
        print(f"\n   📈 Overall Pass Rate: {self.results['overall_pass_rate']:.1f}%")
        print(f"   🏆 Threshold: {CONFIG['pass_rate_threshold']}%")
        
        if self.results['status'] == 'PASS':
            print(f"\n   🎉 ALL TESTS PASSED!")
        else:
            print(f"\n   ⚠️  SOME TESTS FAILED")
        
        print("=" * 60)

    def save_results(self, output_file: str):
        """保存测试结果到 JSON"""
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(self.results, f, indent=2, ensure_ascii=False)
        print(f"\n📁 Results saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(description='MBlink Layout Test Runner')
    parser.add_argument('--build', action='store_true', help='Build tests before running')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--output', '-o', default='layout_test_results.json', help='Output JSON file')
    args = parser.parse_args()
    
    # 确定项目根目录
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    runner = LayoutTestRunner(str(project_root), verbose=args.verbose)
    success = runner.run_all_tests(build=args.build)
    runner.save_results(args.output)
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()

