---
inclusion: always
---

# 代码结构规范引导

在进行代码开发时，必须遵循 `docs/CODE_STRUCTURE_STANDARDS.md` 中定义的规范。

## 核心规则

### 文件大小
- 源文件超过 1500 行需要评审说明
- 源文件超过 2000 行必须有文档说明理由
- 头文件不应超过 300 行
- 单个函数不应超过 150 行

### 目录结构
- 目录超过 15 个源文件必须创建子目录
- 每个子系统目录必须有 CMakeLists.txt
- 包含 3+ 文件的目录应有 README.md

### 命名规范
- 文件名：snake_case（如 `render_object.cpp`）
- 类名：PascalCase（如 `RenderObject`）
- 成员变量：snake_case 带下划线后缀（如 `member_variable_`）

### Include 顺序
1. 对应的头文件
2. 项目内部头文件（按字母顺序）
3. 第三方库头文件
4. 系统头文件

### 依赖管理
- 优先使用前向声明减少依赖
- 禁止循环依赖
- 单文件 include 不应超过 15 个

## 新文件创建检查清单

创建新文件时，确保：
- [ ] 文件名符合 snake_case 规范
- [ ] 类名符合 PascalCase 规范
- [ ] 有适当的文件头注释
- [ ] 公共类有 Doxygen 注释
- [ ] Include 顺序正确
- [ ] 已更新 CMakeLists.txt
- [ ] 如果是新目录，已创建 README.md

## 重构时的注意事项

当修改现有代码时：
- 检查文件大小是否接近阈值
- 考虑是否可以提取独立的类/函数
- 更新所有受影响的 include 路径
- 确保不引入循环依赖

## 参考文档

详细规范请参阅：#[[file:docs/CODE_STRUCTURE_STANDARDS.md]]
