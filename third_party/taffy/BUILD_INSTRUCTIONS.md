# Taffy C Bindings 编译指南

本文档说明如何编译 Taffy C bindings 并集成到 MBink 项目中。

---

## 📋 前提条件

### 必需
- **Rust 1.65+** - 从 https://rustup.rs/ 安装
- **Cargo** - 随 Rust 一起安装
- **Git** - 用于克隆仓库

### Windows 额外要求
- **Visual Studio 2019+** 或 **Build Tools for Visual Studio**
- **MSVC 工具链** - Rust 会自动检测

### Linux 额外要求
```bash
sudo apt-get install build-essential
```

### macOS 额外要求
```bash
xcode-select --install
```

---

## 🚀 编译步骤

### 步骤 1: 克隆 Taffy 仓库（c-bindings 分支）

```bash
cd third_party/taffy
git clone https://github.com/DioxusLabs/taffy.git src
cd src
git fetch origin pull/404/head:c-bindings
git checkout c-bindings
```

### 步骤 2: 编译 C bindings

```bash
cd bindings/c
cargo build --release
```

编译时间：约 2-5 分钟（首次编译会下载依赖）

### 步骤 3: 复制编译产物

#### Windows

```bash
# 复制库文件
cp target/release/taffy.lib ../../lib/windows/

# 复制头文件
cp include/taffy.h ../../include/
```

#### Linux

```bash
# 复制库文件
cp target/release/libtaffy.a ../../lib/linux/

# 复制头文件
cp include/taffy.h ../../include/
```

#### macOS

```bash
# 复制库文件
cp target/release/libtaffy.a ../../lib/macos/

# 复制头文件
cp include/taffy.h ../../include/
```

### 步骤 4: 验证文件结构

编译完成后，`third_party/taffy` 目录应该如下：

```
taffy/
├── README.md
├── BUILD_INSTRUCTIONS.md
├── CMakeLists.txt
├── include/
│   └── taffy.h              ✅ 已复制
├── lib/
│   ├── windows/
│   │   └── taffy.lib        ✅ 已复制 (Windows)
│   ├── linux/
│   │   └── libtaffy.a       ✅ 已复制 (Linux)
│   └── macos/
│       └── libtaffy.a       ✅ 已复制 (macOS)
└── src/                     (Taffy 源码，可选保留)
```

### 步骤 5: 清理（可选）

如果不需要保留源码，可以删除 `src` 目录：

```bash
cd third_party/taffy
rm -rf src
```

**注意**：删除后如果需要重新编译，需要重新克隆仓库。

---

## 🔧 重新编译

如果需要更新 Taffy 或重新编译：

```bash
cd third_party/taffy/src/bindings/c
git pull origin c-bindings
cargo clean
cargo build --release
# 然后重复步骤 3
```

---

## 📝 编译脚本（自动化）

为了方便，可以使用以下脚本自动化编译过程：

### Windows (PowerShell)

创建 `third_party/taffy/build.ps1`:

```powershell
# 克隆仓库
if (-not (Test-Path "src")) {
    git clone https://github.com/DioxusLabs/taffy.git src
    cd src
    git fetch origin pull/404/head:c-bindings
    git checkout c-bindings
    cd ..
}

# 编译
cd src/bindings/c
cargo build --release

# 复制文件
Copy-Item target/release/taffy.lib ../../../lib/windows/ -Force
Copy-Item include/taffy.h ../../../include/ -Force

Write-Host "✅ Taffy compiled successfully!"
```

运行：
```powershell
cd third_party/taffy
.\build.ps1
```

### Linux/macOS (Bash)

创建 `third_party/taffy/build.sh`:

```bash
#!/bin/bash
set -e

# 克隆仓库
if [ ! -d "src" ]; then
    git clone https://github.com/DioxusLabs/taffy.git src
    cd src
    git fetch origin pull/404/head:c-bindings
    git checkout c-bindings
    cd ..
fi

# 编译
cd src/bindings/c
cargo build --release

# 检测平台
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    LIB_NAME="libtaffy.a"
else
    PLATFORM="linux"
    LIB_NAME="libtaffy.a"
fi

# 复制文件
cp target/release/$LIB_NAME ../../../lib/$PLATFORM/
cp include/taffy.h ../../../include/

echo "✅ Taffy compiled successfully for $PLATFORM!"
```

运行：
```bash
cd third_party/taffy
chmod +x build.sh
./build.sh
```

---

## ❓ 常见问题

### Q: 编译失败，提示 "cargo: command not found"

**A**: 需要安装 Rust。访问 https://rustup.rs/ 并按照说明安装。

### Q: Windows 上编译失败，提示找不到 MSVC

**A**: 需要安装 Visual Studio 或 Build Tools for Visual Studio。Rust 需要 MSVC 工具链。

### Q: 编译很慢

**A**: 首次编译会下载所有依赖，需要 2-5 分钟。后续编译会快很多。

### Q: 是否需要保留 src 目录？

**A**: 不需要。编译完成后可以删除 `src` 目录，只保留 `lib` 和 `include`。

### Q: 如何更新 Taffy 版本？

**A**: 
```bash
cd third_party/taffy/src
git pull origin c-bindings
cd bindings/c
cargo build --release
# 重新复制文件
```

---

## 📞 获取帮助

- **Taffy 官方仓库**: https://github.com/DioxusLabs/taffy
- **C Bindings PR**: https://github.com/DioxusLabs/taffy/pull/404
- **Rust 安装**: https://rustup.rs/

---

## ✅ 验证编译成功

编译完成后，运行以下命令验证：

### Windows
```powershell
Test-Path third_party/taffy/lib/windows/taffy.lib
Test-Path third_party/taffy/include/taffy.h
```

### Linux/macOS
```bash
ls -lh third_party/taffy/lib/*/libtaffy.a
ls -lh third_party/taffy/include/taffy.h
```

如果文件存在，说明编译成功！

---

## 🎯 下一步

编译完成后：
1. 返回 MBink 根目录
2. 重新运行 CMake 配置
3. 编译 MBink 项目

```bash
cd ../../..  # 返回 MBink 根目录
cmake -B build
cmake --build build --config Debug
```

