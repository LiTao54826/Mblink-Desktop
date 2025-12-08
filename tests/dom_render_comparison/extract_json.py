#!/usr/bin/env python3
"""从 MBink 输出中提取 JSON 数据"""

import json
import re
import sys
import subprocess

def main():
    # 运行 dom_render_test.exe 并捕获输出
    result = subprocess.run(
        [r'.\build\bin\Release\Debug\dom_render_test.exe', '-q'],
        capture_output=True,
        text=True,
        cwd=r'c:\Users\Administrator\Desktop\code\MBink'
    )
    
    output = result.stdout + result.stderr
    
    # 查找 __RENDER_DATA__ 后面的 JSON
    match = re.search(r'__RENDER_DATA__({.*})', output)
    if match:
        json_str = match.group(1)
        data = json.loads(json_str)
        
        # 保存到文件
        with open(r'tests\dom_render_comparison\reference_data\mbink_new.json', 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        
        print(f"Saved MBink data with {len(data.get('elements', {}))} elements")
    else:
        print("Could not find __RENDER_DATA__ in output")
        print("Output preview:")
        print(output[:500])

if __name__ == '__main__':
    main()

