# Contributing | 贡献

## Accepted Contributions | 接受的贡献类型

- Bug fixes with reproducible cases
  可复现问题的 bug 修复
- Build stability improvements
  构建稳定性改进
- Test additions or fixes
  测试补充或修复
- Documentation corrections based on current source state
  基于当前源码状态的文档修正
- Example validation and cleanup
  示例验证与清理

## Before You Start | 开始前检查

Check whether your change affects:
先确认你的改动是否影响：

1. module boundaries / 模块边界
2. build entry points or dependencies / 构建入口或依赖
3. public API or bindings / 公开 API 或绑定
4. examples or tests / 示例或测试
5. user-visible behavior / 用户可见行为

## Basic Workflow | 基本流程

```bash
git checkout -b feature/your-change
cmake -B build
cmake --build build --config Release
```

For test-related changes | 涉及测试的改动：

```bash
cmake -B build -DMBINK_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Pull Request Checklist | PR 清单

- describe what changed / 说明改了什么
- explain why the change is needed / 说明为什么要改
- include validation steps / 写明验证步骤
- mention impact on build, tests, bindings, examples, or public interfaces / 说明影响范围

## Documentation Requirements | 文档要求

- do not describe placeholder directories as supported features
  不要把占位目录写成已支持功能
- do not treat historical plans as current project state
  不要把历史计划当成当前状态
- mark unverified capabilities clearly
  未验证能力要明确标注
- keep statements aligned with the current source tree and build scripts
  文档表述必须与当前源码和构建脚本一致
