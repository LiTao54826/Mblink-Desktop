"""
MBlink Python绑定安装脚本

功能：
- 安装Python包
- 配置依赖
- 包含运行时动态库（.dll/.so/.dylib）

使用方法：
    pip install .
    或
    python setup.py install
"""

from setuptools import setup
import os
import sys

# 项目根目录
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# 版本信息（与 mblink/__init__.py / mblink/__init__.py 保持一致）
VERSION = "0.5.0"

# 本目录的 README（Python 绑定专用文档）
_HERE = os.path.dirname(os.path.abspath(__file__))
_readme_path = os.path.join(_HERE, 'README.md')
_long_desc = open(_readme_path, encoding='utf-8').read() if os.path.exists(_readme_path) else ""


setup(
    name='mblink',
    version=VERSION,
    description='MBlink Python desktop UI framework (ctypes + C ABI)',
    long_description=_long_desc,
    long_description_content_type='text/markdown',
    author='MBlink Team',
    author_email='team@mblink.dev',
    url='https://github.com/mblink/mblink',
    license='MIT',

    packages=['mblink'],
    package_dir={'mblink': 'mblink'},
    # 包含运行时动态库（Windows: .dll，Linux: .so，macOS: .dylib）
    package_data={'mblink': ['bin/*.dll', 'bin/*.so', 'bin/*.dylib']},
    
    install_requires=[
        # 运行时依赖
    ],
    
    extras_require={
        'dev': [
            'pytest>=7.0.0',
            'pytest-cov>=3.0.0',
            'black>=22.0.0',
            'flake8>=4.0.0',
            'mypy>=0.950',
            'hypothesis>=6.0.0',
        ],
    },
    
    python_requires='>=3.8',
    
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Topic :: Software Development :: User Interfaces',
    ],
    
    keywords='ui gui desktop framework',
)

