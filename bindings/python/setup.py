"""
LightUI Python绑定安装脚本

功能：
- 安装Python包
- 配置依赖
- 包含预编译扩展（.pyd/.so）

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

# 版本信息（与 lightui/__init__.py 及 lightui_core 保持一致）
VERSION = "0.5.0"

# 本目录的 README（Python 绑定专用文档）
_HERE = os.path.dirname(os.path.abspath(__file__))
_readme_path = os.path.join(_HERE, 'README.md')
_long_desc = open(_readme_path, encoding='utf-8').read() if os.path.exists(_readme_path) else ""


setup(
    name='lightui',
    version=VERSION,
    description='Lightweight Python desktop UI framework (ctypes + C ABI)',
    long_description=_long_desc,
    long_description_content_type='text/markdown',
    author='LightUI Team',
    author_email='team@lightui.dev',
    url='https://github.com/lightui/lightui',
    license='MIT',

    packages=['lightui'],
    package_dir={'lightui': 'lightui'},
    # 包含预编译扩展和 DLL（Windows: .pyd + .dll，Linux/macOS: .so）
    package_data={'lightui': ['bin/*.pyd', 'bin/*.so', 'bin/*.dll']},
    
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

