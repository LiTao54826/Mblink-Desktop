# 贡献指南

当前仓库仍处于开源整理阶段，欢迎以“小步、可验证、少承诺”的方式贡献。

## 贡献方向

优先欢迎以下类型的贡献：

- 修复可复现 bug
- 改善构建稳定性
- 补充或修正测试
- 清理仓库中的历史噪音
- 根据代码真实状态修正文档

## 提交原则

- 先看代码和构建脚本，再动文档
- 不要根据旧文档推断当前能力
- 避免一次性大范围重构
- 优先小改动、可验证改动
- 文档只写能从代码中确认的事实

## 开发前建议

建议先确认：

1. 修改涉及哪个模块
2. 是否影响构建入口
3. 是否影响公开 API 或绑定层
4. 是否需要同步更新测试或示例

## 基本开发流程

```bash
git checkout -b feature/your-change
cmake -B build
cmake --build build --config Release
```

如果修改了测试相关内容：

```bash
cmake -B build -DLIGHTUI_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## 文档修改要求

文档改动请遵守：

- 不写完成率、百分比、宣传性口号
- 不把历史计划稿当成现状
- 优先补充 README、BUILD、TESTING、BINDINGS、KNOWN_LIMITATIONS
- 如果某能力尚未验证，请明确写“未确认”或“仍需验证”

## Pull Request 建议

PR 描述建议包含：

- 改了什么
- 为什么改
- 如何验证
- 是否影响构建、测试、绑定、示例或公开接口

## 当前最需要的贡献

- Windows 以外平台验证
- Python binding 稳定性验证
- 示例可运行性筛选
- 测试通过率收口
- 命名统一（MBink / LightUI）方案整理