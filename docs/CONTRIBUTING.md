# 贡献指南

感谢您对LightUI项目的关注！我们欢迎各种形式的贡献。

## 目录

1. [行为准则](#行为准则)
2. [如何贡献](#如何贡献)
3. [开发环境设置](#开发环境设置)
4. [提交代码](#提交代码)
5. [代码审查](#代码审查)
6. [报告Bug](#报告bug)
7. [功能请求](#功能请求)
8. [文档贡献](#文档贡献)

---

## 行为准则

### 我们的承诺

为了营造一个开放和友好的环境，我们承诺：

- 使用友好和包容的语言
- 尊重不同的观点和经验
- 优雅地接受建设性批评
- 关注对社区最有利的事情
- 对其他社区成员表示同理心

### 不可接受的行为

- 使用性化的语言或图像
- 人身攻击或侮辱性评论
- 公开或私下骚扰
- 未经许可发布他人的私人信息
- 其他不道德或不专业的行为

---

## 如何贡献

### 贡献类型

我们欢迎以下类型的贡献：

1. **代码贡献**
   - 新功能实现
   - Bug修复
   - 性能优化
   - 代码重构

2. **文档贡献**
   - API文档
   - 教程和指南
   - 示例代码
   - 翻译

3. **测试贡献**
   - 单元测试
   - 集成测试
   - 性能测试
   - Bug报告

4. **设计贡献**
   - UI/UX设计
   - Logo和图标
   - 网站设计

5. **社区贡献**
   - 回答问题
   - 代码审查
   - 博客文章
   - 演讲和分享

---

## 开发环境设置

### 1. Fork和Clone

```bash
# Fork项目到你的GitHub账号
# 然后clone你的fork

git clone https://github.com/YOUR_USERNAME/lightui.git
cd lightui

# 添加上游仓库
git remote add upstream https://github.com/lightui/lightui.git
```

### 2. 安装依赖

#### Linux (Ubuntu/Debian)

```bash
# 安装构建工具
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libgl1-mesa-dev \
    libglu1-mesa-dev

# 安装Python开发环境
sudo apt-get install -y python3-dev python3-pip
pip3 install -r requirements-dev.txt
```

#### macOS

```bash
# 安装Homebrew（如果还没有）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake sdl3

# 安装Python依赖
pip3 install -r requirements-dev.txt
```

#### Windows

```powershell
# 安装Visual Studio 2022（包含C++工具）
# 下载并安装CMake

# 安装Python依赖
pip install -r requirements-dev.txt
```

### 3. 初始化子模块

```bash
git submodule update --init --recursive
```

### 4. 构建项目

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### 5. 运行测试

```bash
cd build
ctest --output-on-failure
```

---

## 提交代码

### 1. 创建分支

```bash
# 从main分支创建新分支
git checkout main
git pull upstream main
git checkout -b feature/your-feature-name
```

### 2. 编写代码

- 遵循[编码规范](CODING_STANDARDS.md)
- 编写清晰的注释
- 添加单元测试
- 确保代码通过所有测试

### 3. 提交更改

```bash
# 添加更改
git add .

# 提交（遵循提交信息规范）
git commit -m "feat(dom): add querySelector support

- Implement querySelector method
- Add unit tests
- Update documentation

Closes #123"
```

#### 提交信息规范

格式：`<type>(<scope>): <subject>`

**Type类型：**
- `feat`: 新功能
- `fix`: Bug修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

**Scope范围：**
- `dom`: DOM模块
- `event`: 事件系统
- `layout`: 布局引擎
- `render`: 渲染引擎
- `api`: C API
- `python`: Python绑定
- 等等...

**示例：**
```
feat(dom): add querySelector support
fix(event): fix memory leak in event listeners
docs(api): update C API documentation
test(layout): add layout engine tests
```

### 4. 推送到GitHub

```bash
git push origin feature/your-feature-name
```

### 5. 创建Pull Request

1. 访问你的GitHub fork
2. 点击"New Pull Request"
3. 填写PR描述：
   - 描述你的更改
   - 关联相关Issue
   - 添加截图（如果适用）
   - 列出测试步骤

**PR模板：**

```markdown
## 描述
简要描述你的更改

## 相关Issue
Closes #123

## 更改类型
- [ ] Bug修复
- [ ] 新功能
- [ ] 破坏性变更
- [ ] 文档更新

## 测试
描述你如何测试这些更改

## 截图（如果适用）
添加截图

## 检查清单
- [ ] 代码遵循项目规范
- [ ] 添加了单元测试
- [ ] 所有测试通过
- [ ] 更新了文档
- [ ] 提交信息符合规范
```

---

## 代码审查

### 审查流程

1. **自动检查**
   - CI/CD自动运行测试
   - 代码格式检查
   - 静态分析

2. **人工审查**
   - 至少一位维护者审查
   - 检查代码质量
   - 验证功能正确性

3. **反馈和修改**
   - 根据反馈修改代码
   - 推送新的提交
   - 重新请求审查

4. **合并**
   - 审查通过后合并到main分支
   - 自动关闭相关Issue

### 审查标准

- [ ] 代码功能正确
- [ ] 代码风格一致
- [ ] 有充分的测试
- [ ] 文档完整
- [ ] 无明显性能问题
- [ ] 无安全漏洞

---

## 报告Bug

### 在报告Bug之前

1. 检查[已知问题](https://github.com/lightui/lightui/issues)
2. 确保使用最新版本
3. 尝试在干净环境中重现

### 如何报告

使用[Bug报告模板](https://github.com/lightui/lightui/issues/new?template=bug_report.md)

**包含以下信息：**

1. **环境信息**
   - 操作系统和版本
   - LightUI版本
   - Python/C++版本

2. **重现步骤**
   - 详细的步骤
   - 最小可重现示例

3. **预期行为**
   - 你期望发生什么

4. **实际行为**
   - 实际发生了什么
   - 错误信息
   - 截图或日志

5. **额外信息**
   - 可能的原因
   - 临时解决方案

**示例：**

```markdown
## Bug描述
窗口无法正常关闭

## 环境
- OS: Ubuntu 22.04
- LightUI: 1.0.0
- Python: 3.10

## 重现步骤
1. 创建窗口
2. 加载UI
3. 点击关闭按钮
4. 窗口没有关闭

## 预期行为
窗口应该关闭

## 实际行为
窗口仍然显示，控制台输出错误：
```
Error: Failed to destroy window
```

## 最小可重现示例
```python
import lightui
window = lightui.Window("Test", 800, 600)
window.load_ui("<h1>Test</h1>")
window.close()  # 不工作
```
```

---

## 功能请求

### 在请求功能之前

1. 检查[功能请求列表](https://github.com/lightui/lightui/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement)
2. 考虑是否适合项目范围
3. 思考实现方案

### 如何请求

使用[功能请求模板](https://github.com/lightui/lightui/issues/new?template=feature_request.md)

**包含以下信息：**

1. **功能描述**
   - 清晰描述功能
   - 使用场景

2. **动机**
   - 为什么需要这个功能
   - 解决什么问题

3. **建议实现**
   - 可能的实现方案
   - API设计

4. **替代方案**
   - 其他可能的解决方案
   - 为什么不采用

---

## 文档贡献

### 文档类型

1. **API文档**
   - C API文档
   - Python API文档
   - JavaScript API文档

2. **教程**
   - 快速开始
   - 进阶教程
   - 最佳实践

3. **示例**
   - 完整示例项目
   - 代码片段
   - 使用案例

### 文档规范

- 使用Markdown格式
- 包含代码示例
- 添加截图（如果适用）
- 保持简洁清晰
- 检查拼写和语法

### 提交文档

```bash
# 创建分支
git checkout -b docs/update-api-docs

# 编辑文档
# ...

# 提交
git commit -m "docs(api): update Python API documentation"

# 推送并创建PR
git push origin docs/update-api-docs
```

---

## 开发工作流

### 日常开发

```bash
# 1. 同步上游更改
git checkout main
git pull upstream main

# 2. 创建功能分支
git checkout -b feature/my-feature

# 3. 开发和测试
# 编写代码...
cmake --build build
ctest --test-dir build

# 4. 提交更改
git add .
git commit -m "feat: add new feature"

# 5. 推送并创建PR
git push origin feature/my-feature
```

### 保持分支更新

```bash
# 在功能分支上
git fetch upstream
git rebase upstream/main

# 解决冲突（如果有）
# ...

# 强制推送（如果已经推送过）
git push origin feature/my-feature --force-with-lease
```

---

## 发布流程

### 版本号规范

遵循[语义化版本](https://semver.org/)：

- **主版本号（Major）**: 不兼容的API变更
- **次版本号（Minor）**: 向后兼容的功能新增
- **修订号（Patch）**: 向后兼容的Bug修复

### 发布检查清单

- [ ] 所有测试通过
- [ ] 更新CHANGELOG.md
- [ ] 更新版本号
- [ ] 创建Git标签
- [ ] 构建发布包
- [ ] 发布到GitHub Releases
- [ ] 发布到PyPI（Python包）
- [ ] 更新文档网站
- [ ] 发布公告

---

## 获取帮助

### 联系方式

- **GitHub Issues**: [提问](https://github.com/lightui/lightui/issues/new)
- **讨论区**: [GitHub Discussions](https://github.com/lightui/lightui/discussions)
- **邮件列表**: lightui-dev@googlegroups.com
- **Discord**: [加入服务器](https://discord.gg/lightui)

### 资源

- [文档](https://lightui.dev/docs)
- [API参考](https://lightui.dev/api)
- [示例](https://github.com/lightui/examples)
- [博客](https://lightui.dev/blog)

---

## 致谢

感谢所有贡献者！

查看[贡献者列表](https://github.com/lightui/lightui/graphs/contributors)

---

## 许可证

通过贡献代码，您同意您的贡献将在[MIT许可证](LICENSE)下发布。

