# Implementation Plan

## CSS clip-path Property Implementation

- [x] 1. 创建 ClipPath 数据结构

  - [x] 1.1 创建 css_clip_path.h 头文件
    - 定义 ClipPathType 枚举 (NONE, INSET, CIRCLE, ELLIPSE, POLYGON)
    - 定义 ClipFillRule 枚举 (NONZERO, EVENODD)
    - 定义 ClipPosition 结构（用于 at 参数）
    - 定义 ClipInset, ClipCircle, ClipEllipse, ClipPolygon 结构
    - 定义 CSSClipPath 统一结构
    - Location: `core/render/css_clip_path.h`
    - _Requirements: 1.1-1.5_

  - [x] 1.2 实现 css_clip_path.cpp
    - 实现 CSSClipPath::ToSkPath() 方法
    - 实现各形状到 SkPath 的转换
    - Location: `core/render/css_clip_path.cpp`
    - _Requirements: 6.1-6.5_

  - [x] 1.3 更新 CMakeLists.txt
    - 添加 css_clip_path.cpp 到构建
    - Location: `core/render/CMakeLists.txt`

- [x] 2. 添加 clip-path 到 ComputedStyle

  - [x] 2.1 在 ComputedStyle 中添加 clip_path 字段
    - 添加 `std::optional<CSSClipPath> clip_path`
    - 包含 css_clip_path.h 头文件
    - Location: `core/render/render_object.h`
    - _Requirements: 1.1-1.5_

- [x] 3. 实现 clip-path 解析

  - [x] 3.1 实现 inset() 解析
    - 解析 1-4 个长度/百分比参数
    - 解析可选的 round 参数
    - Location: `core/render/css_clip_path.cpp`
    - _Requirements: 2.1-2.6_

  - [x] 3.2 实现 circle() 解析
    - 解析半径参数（长度、百分比、closest-side、farthest-side）
    - 解析可选的 at 位置参数
    - Location: `core/render/css_clip_path.cpp`
    - _Requirements: 3.1-3.7_

  - [x] 3.3 实现 ellipse() 解析
    - 解析两个半径参数
    - 解析可选的 at 位置参数
    - Location: `core/render/css_clip_path.cpp`
    - _Requirements: 4.1-4.5_

  - [x] 3.4 实现 polygon() 解析
    - 解析坐标点列表
    - 解析可选的填充规则
    - Location: `core/render/css_clip_path.cpp`
    - _Requirements: 5.1-5.6_

  - [x] 3.5 在 StyleResolver 中集成 clip-path 解析
    - 添加 clip-path 属性处理
    - 调用对应的解析函数
    - Location: `core/render/style_resolver.cpp`
    - _Requirements: 1.1-1.5_

- [x] 4. 实现 clip-path 渲染

  - [x] 4.1 实现 ApplyClipPath 方法
    - 在 RenderObject 中添加 ApplyClipPath 方法
    - 将 CSSClipPath 转换为 SkPath 并应用到 canvas
    - Location: `core/render/render_object.cpp`
    - _Requirements: 6.1-6.5_

  - [x] 4.2 在 Paint 方法中集成裁剪
    - 在绘制前检查并应用 clip-path
    - 使用 canvas->save() 和 canvas->restore() 保护状态
    - Location: `core/render/render_object.cpp`
    - _Requirements: 6.1-6.5_

- [x] 5. Checkpoint - 验证基本功能
  - 确保 clip-path 解析和渲染正常工作
  - 如有问题，询问用户

- [x] 6. 编写属性测试

  - [x] 6.1 创建 test_clip_path_properties.cpp
    - **Property 1: ClipPath None 不应用裁剪**
    - **Property 2: Inset 正确计算裁剪区域**
    - **Property 3: Circle 正确计算圆形区域**
    - **Property 4: Ellipse 正确计算椭圆区域**
    - **Property 5: Polygon 正确计算多边形区域**
    - **Property 6: 百分比值正确相对于元素尺寸计算**
    - Location: `tests/property/render/test_clip_path_properties.cpp`
    - _Requirements: 1.1-6.5_

  - [x] 6.2 更新测试 CMakeLists.txt
    - 添加 test_clip_path_properties.cpp 到构建
    - Location: `tests/property/CMakeLists.txt`

- [x] 7. Final Checkpoint - 确保所有测试通过
  - 运行所有属性测试
  - 如有问题，询问用户
