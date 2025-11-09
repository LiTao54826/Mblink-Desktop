# LightUI 第三方库

本目录包含LightUI所需的所有第三方库。

---

## 📦 依赖库列表

| 库名 | 版本 | 大小 | 用途 | 许可证 |
|------|------|------|------|--------|
| **QuickJS** | 2024-01-13 | ~600KB | JavaScript引擎 | MIT |
| **Skia** | latest | ~5-8MB | 2D图形渲染 | BSD |
| **SDL3** | latest | ~1-2MB | 窗口和输入 | Zlib |
| **Yoga** | latest | ~1-2MB | Flexbox布局 | MIT |

---

## 🔧 安装方法

### 方式1: 自动下载（推荐）

```bash
# Linux/macOS
./download_deps.sh

# Windows
.\download_deps.bat
```

### 方式2: 手动下载

#### QuickJS

```bash
cd third_party/
wget https://bellard.org/quickjs/quickjs-2024-01-13.tar.xz
tar xf quickjs-2024-01-13.tar.xz
mv quickjs-2024-01-13 quickjs
cd quickjs
make
```

#### SDL3

```bash
cd third_party/
git clone https://github.com/libsdl-org/SDL SDL3
cd SDL3
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

#### Skia

```bash
cd third_party/
git clone https://github.com/google/skia.git
cd skia
python3 tools/git-sync-deps
bin/gn gen out/Release --args='is_official_build=true'
ninja -C out/Release
```

#### Yoga

```bash
cd third_party/
git clone https://github.com/facebook/yoga.git
cd yoga
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

---

## 📋 构建状态

- [ ] QuickJS - 待下载
- [ ] SDL3 - 待下载
- [ ] Skia - 待下载
- [ ] Yoga - 待下载

---

## 🔍 版本信息

更新此文件以跟踪实际使用的版本：

```
QuickJS: 2024-01-13
SDL3: commit xxxxxxx
Skia: commit xxxxxxx
Yoga: v2.0.1
```

---

## 📝 注意事项

1. **Skia编译时间较长**（30分钟-2小时），建议使用预编译版本
2. **Windows用户**需要安装Visual Studio 2019+
3. **macOS用户**需要安装Xcode Command Line Tools
4. **Linux用户**需要安装基础开发工具：
   ```bash
   sudo apt-get install build-essential cmake git python3
   ```

---

## 🚀 快速开始（最小依赖）

如果只想快速测试，可以先只安装SDL3：

```bash
cd third_party/
git clone https://github.com/libsdl-org/SDL SDL3
cd SDL3
mkdir build && cd build
cmake ..
cmake --build .
```

然后修改根目录的CMakeLists.txt，暂时注释掉其他依赖。

---

## 📞 获取帮助

如果遇到编译问题，请查看：
- [SDL3文档](https://wiki.libsdl.org/SDL3)
- [Skia编译指南](https://skia.org/docs/user/build/)
- [QuickJS文档](https://bellard.org/quickjs/)
- [Yoga文档](https://yogalayout.com/)

