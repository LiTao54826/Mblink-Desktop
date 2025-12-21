#!/usr/bin/env python3
"""
将二进制文件转换为 C++ 字节数组
用法: python generate_embedded_binary.py <input_file> <output_file> <var_name>
"""

import sys
import os

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input_file> <output_file> <var_name>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    var_name = sys.argv[3]
    
    # 检查输入文件
    if not os.path.exists(input_file):
        print(f"Warning: Input file not found: {input_file}")
        # 生成空的占位文件
        with open(output_file, 'w') as f:
            f.write(f"// {var_name} not available\n")
            f.write(f"static const unsigned char {var_name}_data[] = {{}};\n")
            f.write(f"static const size_t {var_name}_size = 0;\n")
        return
    
    # 读取二进制文件
    with open(input_file, 'rb') as f:
        data = f.read()
    
    file_size = len(data)
    print(f"Processing {input_file} ({file_size} bytes)...")
    
    # 生成 C++ 代码
    with open(output_file, 'w') as f:
        f.write(f"// Auto-generated from {os.path.basename(input_file)}\n")
        f.write(f"// Size: {file_size} bytes\n\n")
        f.write(f"static const unsigned char {var_name}_data[] = {{\n")
        
        # 每行 16 个字节
        for i in range(0, file_size, 16):
            chunk = data[i:i+16]
            hex_bytes = ', '.join(f'0x{b:02x}' for b in chunk)
            if i + 16 < file_size:
                f.write(f"    {hex_bytes},\n")
            else:
                f.write(f"    {hex_bytes}\n")
        
        f.write("};\n\n")
        f.write(f"static const size_t {var_name}_size = {file_size};\n")
    
    print(f"Generated {output_file}")

if __name__ == '__main__':
    main()
