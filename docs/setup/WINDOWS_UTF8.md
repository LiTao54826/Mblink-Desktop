# Windows UTF-8 编码问题解决方案

本文档说明如何解决Windows上的中文乱码问题。

---

## 📋 问题描述

在Windows上运行批处理脚本（.bat）时，可能会出现中文乱码：

```
鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹�
  LightUI 渚濊禆搴撲笅杞借剼鏈�
鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹傗攢鈹�
```

这是因为Windows默认使用GBK编码，而脚本文件使用UTF-8编码。

---

## ✅ 解决方案

### 方案1: 使用PowerShell脚本（推荐）⭐

PowerShell对UTF-8的支持更好，推荐使用PowerShell脚本：

```powershell
# 1. 以管理员身份运行PowerShell

# 2. 允许运行脚本（首次需要）
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# 3. 运行PowerShell脚本
.\scripts\download_deps.ps1
```

**优点**:
- ✅ 完美支持UTF-8
- ✅ 更好的错误处理
- ✅ 彩色输出
- ✅ 更强大的功能

### 方案2: 批处理脚本已修复

批处理脚本已添加UTF-8支持：

```batch
@echo off
REM 设置UTF-8编码，解决中文乱码问题
chcp 65001 >nul 2>&1
```

直接运行即可：

```cmd
.\scripts\download_deps.bat
```

### 方案3: 修改终端编码

如果仍然出现乱码，可以手动设置终端编码：

```cmd
REM 设置为UTF-8
chcp 65001

REM 运行脚本
.\scripts\download_deps.bat
```

### 方案4: 使用Windows Terminal

Windows Terminal对UTF-8的支持更好：

1. 安装 [Windows Terminal](https://aka.ms/terminal)
2. 在Windows Terminal中运行脚本

---

## 🔧 PowerShell执行策略

### 问题: 无法运行PowerShell脚本

**错误信息**:
```
.\download_deps.ps1 : 无法加载文件，因为在此系统上禁止运行脚本。
```

### 解决方案

#### 方式1: 临时允许（推荐）

```powershell
# 仅对当前用户允许运行脚本
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

#### 方式2: 绕过执行策略

```powershell
# 临时绕过执行策略运行脚本
PowerShell -ExecutionPolicy Bypass -File .\scripts\download_deps.ps1
```

#### 方式3: 查看当前策略

```powershell
# 查看当前执行策略
Get-ExecutionPolicy

# 查看所有作用域的执行策略
Get-ExecutionPolicy -List
```

---

## 📊 编码对比

| 编码 | 说明 | 优点 | 缺点 |
|------|------|------|------|
| **UTF-8** | 国际标准 | 支持所有语言 | Windows默认不支持 |
| **GBK** | 中文编码 | Windows默认 | 只支持中文 |
| **UTF-8 BOM** | 带BOM的UTF-8 | Windows兼容性好 | 某些工具不支持BOM |

---

## 🛠️ 脚本对比

### PowerShell脚本 (download_deps.ps1)

**优点**:
- ✅ 原生UTF-8支持
- ✅ 更好的错误处理
- ✅ 彩色输出（Green/Yellow/Red/Cyan）
- ✅ 更强大的功能
- ✅ 更好的代理支持
- ✅ 更详细的进度提示

**缺点**:
- ❌ 需要设置执行策略
- ❌ 某些旧系统可能不支持

**使用场景**: 推荐用于Windows 10/11

### 批处理脚本 (download_deps.bat)

**优点**:
- ✅ 无需额外配置
- ✅ 兼容所有Windows版本
- ✅ 简单易用

**缺点**:
- ❌ UTF-8支持需要额外配置
- ❌ 错误处理较弱
- ❌ 功能相对简单

**使用场景**: 用于旧版Windows或无法使用PowerShell的环境

---

## 🔍 验证编码

### 检查文件编码

#### 使用PowerShell

```powershell
# 检查文件编码
Get-Content .\scripts\download_deps.bat -Encoding Byte -TotalCount 3
```

#### 使用记事本

1. 用记事本打开文件
2. 点击"文件" -> "另存为"
3. 查看"编码"下拉框

### 检查终端编码

```cmd
REM 查看当前代码页
chcp

REM 常见代码页：
REM 936  - GBK (简体中文)
REM 65001 - UTF-8
```

---

## 🐛 常见问题

### 问题1: PowerShell脚本无法运行

**错误信息**:
```
无法加载文件，因为在此系统上禁止运行脚本
```

**解决方案**:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### 问题2: 批处理脚本中文乱码

**解决方案1**: 使用PowerShell脚本
```powershell
.\scripts\download_deps.ps1
```

**解决方案2**: 手动设置编码
```cmd
chcp 65001
.\scripts\download_deps.bat
```

### 问题3: Git输出乱码

**解决方案**:
```bash
# 设置Git使用UTF-8
git config --global core.quotepath false
git config --global gui.encoding utf-8
git config --global i18n.commit.encoding utf-8
git config --global i18n.logoutput.encoding utf-8
```

### 问题4: CMake输出乱码

**解决方案**:
```cmd
REM 在运行CMake前设置编码
chcp 65001
cmake ..
```

---

## 💡 最佳实践

### 1. 使用Windows Terminal

Windows Terminal对UTF-8的支持最好：

1. 安装Windows Terminal
2. 设置默认配置文件为PowerShell
3. 在Windows Terminal中运行所有命令

### 2. 统一使用UTF-8

在项目中统一使用UTF-8编码：

- 源代码文件: UTF-8
- 脚本文件: UTF-8
- 文档文件: UTF-8

### 3. 配置编辑器

#### Visual Studio Code

在 `.vscode/settings.json` 中添加：

```json
{
    "files.encoding": "utf8",
    "files.autoGuessEncoding": false
}
```

#### Visual Studio

工具 -> 选项 -> 环境 -> 文档 -> 默认编码: UTF-8

### 4. Git配置

```bash
# 设置Git使用UTF-8
git config --global core.quotepath false
git config --global gui.encoding utf-8
git config --global i18n.commit.encoding utf-8
git config --global i18n.logoutput.encoding utf-8
```

---

## 📚 参考资料

### Windows代码页

| 代码页 | 编码 | 说明 |
|--------|------|------|
| 936 | GBK | 简体中文 |
| 950 | Big5 | 繁体中文 |
| 65001 | UTF-8 | 国际标准 |
| 437 | OEM | 美国英语 |

### PowerShell执行策略

| 策略 | 说明 |
|------|------|
| **Restricted** | 默认策略，不允许运行脚本 |
| **AllSigned** | 只运行签名的脚本 |
| **RemoteSigned** | 本地脚本可运行，远程脚本需签名（推荐） |
| **Unrestricted** | 所有脚本都可运行（不推荐） |
| **Bypass** | 不阻止任何脚本，不显示警告 |

### 相关命令

```powershell
# PowerShell
Get-ExecutionPolicy                    # 查看执行策略
Set-ExecutionPolicy RemoteSigned       # 设置执行策略
[Console]::OutputEncoding              # 查看输出编码
$OutputEncoding                        # 查看输出编码变量

# CMD
chcp                                   # 查看代码页
chcp 65001                            # 设置为UTF-8
chcp 936                              # 设置为GBK
```

---

## 🚀 快速开始

### 推荐流程（Windows 10/11）

```powershell
# 1. 打开Windows Terminal或PowerShell

# 2. 设置执行策略（首次需要）
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# 3. 设置代理（如果需要）
$env:HTTP_PROXY = "http://127.0.0.1:7890"
$env:HTTPS_PROXY = "http://127.0.0.1:7890"

# 4. 运行PowerShell脚本
.\scripts\download_deps.ps1

# 5. 编译QuickJS
cd third_party\quickjs
nmake
cd ..\..

# 6. 配置CMake
mkdir build
cd build
cmake ..

# 7. 编译项目
cmake --build . --config Release
```

### 兼容流程（所有Windows版本）

```cmd
REM 1. 打开命令提示符

REM 2. 设置UTF-8编码
chcp 65001

REM 3. 设置代理（如果需要）
set HTTP_PROXY=http://127.0.0.1:7890
set HTTPS_PROXY=http://127.0.0.1:7890

REM 4. 运行批处理脚本
.\scripts\download_deps.bat

REM 5. 编译QuickJS
cd third_party\quickjs
nmake
cd ..\..

REM 6. 配置CMake
mkdir build
cd build
cmake ..

REM 7. 编译项目
cmake --build . --config Release
```

---

<div align="center">

**UTF-8编码问题已解决！** ✅

**推荐使用PowerShell脚本获得最佳体验** 🚀

[返回构建指南](../BUILD.md) • [返回主页](../README.md)

</div>

