# 代理配置指南

本文档说明如何为LightUI的依赖下载脚本配置代理。

---

## 📋 为什么需要代理？

在某些网络环境下，直接访问GitHub、Google等网站可能会失败或速度很慢。配置代理可以：

- ✅ 加速GitHub仓库克隆
- ✅ 加速文件下载
- ✅ 解决网络访问问题

---

## 🚀 快速配置

### Linux/macOS

#### 方式1: 临时设置（推荐）

```bash
# 设置代理环境变量
export HTTP_PROXY="http://127.0.0.1:7890"
export HTTPS_PROXY="http://127.0.0.1:7890"

# 运行下载脚本
./scripts/download_deps.sh
```

#### 方式2: 一行命令

```bash
HTTP_PROXY="http://127.0.0.1:7890" HTTPS_PROXY="http://127.0.0.1:7890" ./scripts/download_deps.sh
```

#### 方式3: 编辑脚本

编辑 `scripts/download_deps.sh`，找到这一行：

```bash
# PROXY="http://127.0.0.1:7890"
```

取消注释并修改为你的代理地址：

```bash
PROXY="http://127.0.0.1:7890"
```

### Windows

#### 方式1: PowerShell脚本（推荐，更好的UTF-8支持）⭐

```powershell
# 设置代理环境变量
$env:HTTP_PROXY = "http://127.0.0.1:7890"
$env:HTTPS_PROXY = "http://127.0.0.1:7890"

# 运行PowerShell脚本
.\scripts\download_deps.ps1
```

#### 方式2: 批处理脚本

```cmd
REM 设置代理环境变量
set HTTP_PROXY=http://127.0.0.1:7890
set HTTPS_PROXY=http://127.0.0.1:7890

REM 运行批处理脚本
.\scripts\download_deps.bat
```

> **注意**: 批处理脚本可能出现中文乱码，推荐使用PowerShell脚本

#### 方式3: 编辑脚本

**PowerShell脚本** (`scripts/download_deps.ps1`):
```powershell
# $Proxy = "http://127.0.0.1:7890"  # 取消这行的注释
```

**批处理脚本** (`scripts/download_deps.bat`):
```batch
REM set PROXY=http://127.0.0.1:7890  REM 取消这行的注释
```

---

## 🔧 代理类型

### HTTP/HTTPS 代理

最常见的代理类型：

```bash
# Linux/macOS
export HTTP_PROXY="http://127.0.0.1:7890"
export HTTPS_PROXY="http://127.0.0.1:7890"

# Windows
set HTTP_PROXY=http://127.0.0.1:7890
set HTTPS_PROXY=http://127.0.0.1:7890
```

### SOCKS5 代理

某些代理工具使用SOCKS5协议：

```bash
# Linux/macOS
export HTTP_PROXY="socks5://127.0.0.1:7890"
export HTTPS_PROXY="socks5://127.0.0.1:7890"

# Windows
set HTTP_PROXY=socks5://127.0.0.1:7890
set HTTPS_PROXY=socks5://127.0.0.1:7890
```

### 带认证的代理

如果代理需要用户名和密码：

```bash
# Linux/macOS
export HTTP_PROXY="http://username:password@proxy.example.com:8080"
export HTTPS_PROXY="http://username:password@proxy.example.com:8080"

# Windows
set HTTP_PROXY=http://username:password@proxy.example.com:8080
set HTTPS_PROXY=http://username:password@proxy.example.com:8080
```

---

## 🛠️ 常见代理工具配置

### Clash

Clash默认端口通常是 `7890`：

```bash
# Linux/macOS
export HTTP_PROXY="http://127.0.0.1:7890"
export HTTPS_PROXY="http://127.0.0.1:7890"

# Windows
set HTTP_PROXY=http://127.0.0.1:7890
set HTTPS_PROXY=http://127.0.0.1:7890
```

### V2Ray

V2Ray默认端口通常是 `10808` (HTTP) 或 `10809` (SOCKS)：

```bash
# HTTP代理
export HTTP_PROXY="http://127.0.0.1:10808"
export HTTPS_PROXY="http://127.0.0.1:10808"

# SOCKS5代理
export HTTP_PROXY="socks5://127.0.0.1:10809"
export HTTPS_PROXY="socks5://127.0.0.1:10809"
```

### Shadowsocks

Shadowsocks通常使用 `1080` 端口：

```bash
export HTTP_PROXY="socks5://127.0.0.1:1080"
export HTTPS_PROXY="socks5://127.0.0.1:1080"
```

### 企业代理

企业环境通常使用HTTP代理：

```bash
export HTTP_PROXY="http://proxy.company.com:8080"
export HTTPS_PROXY="http://proxy.company.com:8080"
```

---

## 🔍 验证代理配置

### 测试代理连接

#### Linux/macOS

```bash
# 测试HTTP代理
curl -x http://127.0.0.1:7890 https://www.google.com

# 测试环境变量
echo $HTTP_PROXY
echo $HTTPS_PROXY

# 测试Git代理
git config --global --get http.proxy
git config --global --get https.proxy
```

#### Windows

```cmd
REM 测试环境变量
echo %HTTP_PROXY%
echo %HTTPS_PROXY%

REM 测试Git代理
git config --global --get http.proxy
git config --global --get https.proxy
```

### 测试GitHub连接

```bash
# 测试GitHub SSH
ssh -T git@github.com

# 测试GitHub HTTPS
git ls-remote https://github.com/libsdl-org/SDL.git
```

---

## 🐛 常见问题

### 问题1: 代理设置后仍然无法下载

**可能原因**:
- 代理地址或端口错误
- 代理服务未启动
- 防火墙阻止连接

**解决方案**:
```bash
# 1. 检查代理是否运行
curl -x http://127.0.0.1:7890 https://www.google.com

# 2. 检查端口是否正确
netstat -an | grep 7890  # Linux/macOS
netstat -an | findstr 7890  # Windows

# 3. 尝试不同的代理协议
# HTTP -> SOCKS5 或 SOCKS5 -> HTTP
```

### 问题2: Git克隆失败

**错误信息**:
```
fatal: unable to access 'https://github.com/...': Failed to connect to 127.0.0.1 port 7890
```

**解决方案**:
```bash
# 检查Git代理配置
git config --global --get http.proxy

# 重新设置Git代理
git config --global http.proxy "http://127.0.0.1:7890"
git config --global https.proxy "http://127.0.0.1:7890"

# 或者取消Git代理
git config --global --unset http.proxy
git config --global --unset https.proxy
```

### 问题3: PowerShell下载失败（Windows）

**错误信息**:
```
Invoke-WebRequest : Unable to connect to the remote server
```

**解决方案**:

编辑 `scripts/download_deps.bat`，确保PowerShell命令包含代理配置：

```batch
powershell -Command "$proxy = New-Object System.Net.WebProxy('http://127.0.0.1:7890'); [System.Net.WebRequest]::DefaultWebProxy = $proxy; Invoke-WebRequest ..."
```

### 问题4: SOCKS5代理不工作

**解决方案**:

某些工具不支持SOCKS5，尝试使用HTTP代理：

```bash
# 如果你的代理工具支持，启用HTTP代理端口
# 例如Clash的HTTP端口是7890，SOCKS5端口是7891

# 使用HTTP代理
export HTTP_PROXY="http://127.0.0.1:7890"
export HTTPS_PROXY="http://127.0.0.1:7890"
```

---

## 🔐 安全建议

### 1. 不要在脚本中硬编码敏感信息

❌ **不推荐**:
```bash
PROXY="http://username:password@proxy.com:8080"
```

✅ **推荐**:
```bash
# 使用环境变量
export HTTP_PROXY="http://username:password@proxy.com:8080"
```

### 2. 使用完后清除代理配置

```bash
# Linux/macOS
unset HTTP_PROXY
unset HTTPS_PROXY

# Windows
set HTTP_PROXY=
set HTTPS_PROXY=
```

### 3. 仅在需要时使用代理

```bash
# 临时使用代理（不影响全局）
HTTP_PROXY="http://127.0.0.1:7890" ./scripts/download_deps.sh
```

---

## 📚 参考资料

### 环境变量

- `HTTP_PROXY` - HTTP协议代理
- `HTTPS_PROXY` - HTTPS协议代理
- `ALL_PROXY` - 所有协议代理
- `NO_PROXY` - 不使用代理的地址列表

### Git代理配置

```bash
# 设置代理
git config --global http.proxy "http://127.0.0.1:7890"
git config --global https.proxy "http://127.0.0.1:7890"

# 查看代理
git config --global --get http.proxy
git config --global --get https.proxy

# 取消代理
git config --global --unset http.proxy
git config --global --unset https.proxy

# 仅对特定仓库设置代理
git config http.proxy "http://127.0.0.1:7890"
```

---

## 💡 最佳实践

### 1. 使用环境变量（推荐）

```bash
# 在 ~/.bashrc 或 ~/.zshrc 中添加（Linux/macOS）
export HTTP_PROXY="http://127.0.0.1:7890"
export HTTPS_PROXY="http://127.0.0.1:7890"

# 在系统环境变量中添加（Windows）
# 控制面板 -> 系统 -> 高级系统设置 -> 环境变量
```

### 2. 创建代理切换脚本

**Linux/macOS** (`~/.proxy.sh`):
```bash
#!/bin/bash
# 启用代理
proxy_on() {
    export HTTP_PROXY="http://127.0.0.1:7890"
    export HTTPS_PROXY="http://127.0.0.1:7890"
    git config --global http.proxy "http://127.0.0.1:7890"
    git config --global https.proxy "http://127.0.0.1:7890"
    echo "代理已启用"
}

# 禁用代理
proxy_off() {
    unset HTTP_PROXY
    unset HTTPS_PROXY
    git config --global --unset http.proxy
    git config --global --unset https.proxy
    echo "代理已禁用"
}
```

使用：
```bash
source ~/.proxy.sh
proxy_on   # 启用代理
proxy_off  # 禁用代理
```

---

<div align="center">

**代理配置完成！现在可以顺利下载依赖了** 🎉

[返回构建指南](../BUILD.md) • [返回主页](../README.md)

</div>

