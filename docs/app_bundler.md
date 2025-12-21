# MBink App Bundler

将 Preact/JS 应用打包成独立的 Windows 可执行文件。

## 基本用法

```bash
app_bundler <app.js> -o <output.exe> [选项]
```

## 选项

| 选项 | 说明 |
|------|------|
| `-o, --output <file>` | 输出文件路径（必需） |
| `--width <value>` | 窗口宽度（默认: 800） |
| `--height <value>` | 窗口高度（默认: 600） |
| `--title <value>` | 窗口标题（默认: MBink App） |
| `--include <file>` | 包含额外的 JS 文件（可多次使用） |
| `--asset <file>` | 打包单个资源文件（可多次使用） |
| `--assets <dir>` | 打包资源目录（可多次使用） |
| `--template <file>` | 指定模板 exe 路径 |
| `--compress` | 使用 UPX 压缩输出文件 |
| `--debug` | 调试模式（输出 exe 显示控制台窗口） |
| `--verbose` | 显示详细信息 |
| `--no-overwrite` | 不覆盖已存在的输出文件 |

## 示例

### 基本打包

```bash
app_bundler my_app.js -o my_app.exe
```

### 自定义窗口大小

```bash
app_bundler my_app.js -o my_app.exe --width 1024 --height 768
```

### 打包资源目录

```bash
app_bundler my_app.js -o my_app.exe --assets ./assets
```

资源目录会保留目录名作为前缀。例如 `./assets/logo.png` 打包后路径为 `assets/logo.png`。

### 打包单个资源文件

```bash
app_bundler my_app.js -o my_app.exe --asset logo.png --asset config.json
```

### 使用 UPX 压缩

```bash
app_bundler my_app.js -o my_app.exe --compress
```

## 资源访问

### 图片

在 JS 中直接使用相对路径，打包后会自动从嵌入资源加载：

```javascript
h('img', { src: 'assets/logo.png' })
```

### JS API

打包后的 exe 提供以下全局函数：

```javascript
// 列出所有嵌入资源
const assets = listAssets();
// 返回: ['assets/logo.png', 'assets/style.css', ...]

// 检查资源是否存在
const exists = hasAsset('assets/logo.png');
// 返回: true/false

// 加载资源数据
const data = loadAsset('assets/config.json');
// 返回: ArrayBuffer 或 null

// 获取资源的 data URL
const url = getAssetUrl('assets/logo.png');
// 返回: 'data:image/png;base64,...'
```

### 读取文本文件

```javascript
const data = loadAsset('assets/config.json');
if (data) {
    const text = new TextDecoder().decode(data);
    const config = JSON.parse(text);
}
```

## 工作原理

1. 解析 JS 模块依赖
2. 使用 QuickJS 编译为字节码
3. 将字节码和资源打包为 payload
4. 将 payload 追加到 app_loader.exe 模板后面
5. 生成独立的可执行文件

## 注意事项

- 资源路径在打包前后应保持一致
- 使用 `--assets <dir>` 时，目录名会作为路径前缀
- 调试时可使用 `--debug` 选项显示控制台窗口
- 运行打包后的 exe 时可加 `--verbose` 查看详细日志
