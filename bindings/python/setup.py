"""
LightUI Python绑定安装脚本

功能：
- 编译C扩展模块
- 安装Python包
- 配置依赖

使用方法：
    pip install .
    或
    python setup.py install

TODO:
- [ ] 配置C扩展编译
- [ ] 添加依赖库链接
- [ ] 配置包数据
- [ ] 添加测试命令
"""

from setuptools import setup, Extension
import os
import sys

# 项目根目录
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# 版本信息
VERSION = "0.1.0"

# 扩展模块配置
# TODO: 配置C扩展
# lightui_extension = Extension(
#     'lightui._lightui',
#     sources=['lightui/_lightui.c'],
#     include_dirs=[
#         os.path.join(ROOT_DIR, 'core'),
#         os.path.join(ROOT_DIR, 'core/api'),
#     ],
#     library_dirs=[
#         os.path.join(ROOT_DIR, 'build/lib'),
#     ],
#     libraries=['lightui'],
#     extra_compile_args=['-std=c11'],
# )

setup(
    name='lightui',
    version=VERSION,
    description='Lightweight cross-language UI framework',
    long_description=open(os.path.join(ROOT_DIR, 'README.md'), encoding='utf-8').read(),
    long_description_content_type='text/markdown',
    author='LightUI Team',
    author_email='team@lightui.dev',
    url='https://github.com/lightui/lightui',
    license='MIT',
    
    packages=['lightui'],
    package_dir={'lightui': 'lightui'},
    
    # TODO: 添加C扩展
    # ext_modules=[lightui_extension],
    
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
        ],
    },
    
    python_requires='>=3.7',
    
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Topic :: Software Development :: User Interfaces',
    ],
    
    keywords='ui gui desktop framework',
)

