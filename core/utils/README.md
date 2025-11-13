# Utils 模块

## 📋 概述

Utils 模块提供了 MBink 框架中通用的工具函数和辅助类，包括日志系统、JSON 处理、字符串操作、文件 I/O 等基础功能。这些工具被所有其他模块广泛使用。

## 🎯 主要功能

- **日志系统**: 多级别日志记录（Debug, Info, Warn, Error）
- **JSON 处理**: 基于 nlohmann/json 的 JSON 解析和序列化
- **字符串工具**: 字符串分割、修剪、格式化等
- **文件操作**: 文件读写、路径处理
- **时间工具**: 时间戳、格式化、计时器
- **颜色转换**: CSS 颜色到 SkColor 的转换
- **URL 解析**: URL 解析和处理

## 📁 文件结构

```
utils/
├── CMakeLists.txt        # 构建配置
├── logger.h/cpp          # 日志系统
├── json_utils.h/cpp      # JSON 工具
├── string_utils.h/cpp    # 字符串工具
├── file_utils.h/cpp      # 文件工具
├── time_utils.h/cpp      # 时间工具
├── color_utils.h/cpp     # 颜色工具
└── url_utils.h/cpp       # URL 工具
```

## 🔌 核心类和函数

### Logger (日志系统)

```cpp
class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
    
    // 设置日志级别
    static void SetLevel(Level level);
    
    // 设置日志输出
    static void SetOutput(std::ostream* output);
    
    // 日志记录
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warn(const std::string& message);
    static void Error(const std::string& message);
    
    // 格式化日志
    template<typename... Args>
    static void Debug(const std::string& format, Args... args);
    
    template<typename... Args>
    static void Info(const std::string& format, Args... args);
    
    template<typename... Args>
    static void Warn(const std::string& format, Args... args);
    
    template<typename... Args>
    static void Error(const std::string& format, Args... args);
};
```

### JSON 工具

```cpp
namespace JsonUtils {
    // 解析 JSON
    nlohmann::json Parse(const std::string& json_str);
    nlohmann::json ParseFile(const std::string& filepath);
    
    // 序列化 JSON
    std::string Stringify(const nlohmann::json& json, int indent = -1);
    bool WriteFile(const std::string& filepath, const nlohmann::json& json);
    
    // 类型检查
    bool IsObject(const nlohmann::json& json);
    bool IsArray(const nlohmann::json& json);
    bool IsString(const nlohmann::json& json);
    bool IsNumber(const nlohmann::json& json);
    bool IsBool(const nlohmann::json& json);
    
    // 安全访问
    std::optional<std::string> GetString(const nlohmann::json& json, 
                                        const std::string& key);
    std::optional<int> GetInt(const nlohmann::json& json, 
                             const std::string& key);
    std::optional<double> GetDouble(const nlohmann::json& json, 
                                   const std::string& key);
    std::optional<bool> GetBool(const nlohmann::json& json, 
                               const std::string& key);
}
```

### 字符串工具

```cpp
namespace StringUtils {
    // 修剪空白
    std::string Trim(const std::string& str);
    std::string TrimLeft(const std::string& str);
    std::string TrimRight(const std::string& str);
    
    // 分割和连接
    std::vector<std::string> Split(const std::string& str, char delimiter);
    std::string Join(const std::vector<std::string>& parts, 
                    const std::string& separator);
    
    // 大小写转换
    std::string ToLower(const std::string& str);
    std::string ToUpper(const std::string& str);
    
    // 查找和替换
    bool StartsWith(const std::string& str, const std::string& prefix);
    bool EndsWith(const std::string& str, const std::string& suffix);
    bool Contains(const std::string& str, const std::string& substr);
    std::string Replace(const std::string& str, 
                       const std::string& from,
                       const std::string& to);
    
    // 格式化
    template<typename... Args>
    std::string Format(const std::string& format, Args... args);
}
```

### 文件工具

```cpp
namespace FileUtils {
    // 读写文件
    std::string ReadFile(const std::string& filepath);
    bool WriteFile(const std::string& filepath, const std::string& content);
    
    // 文件信息
    bool Exists(const std::string& filepath);
    bool IsFile(const std::string& filepath);
    bool IsDirectory(const std::string& filepath);
    size_t GetFileSize(const std::string& filepath);
    
    // 路径操作
    std::string GetDirectory(const std::string& filepath);
    std::string GetFilename(const std::string& filepath);
    std::string GetExtension(const std::string& filepath);
    std::string JoinPath(const std::string& path1, const std::string& path2);
    std::string NormalizePath(const std::string& path);
    
    // 目录操作
    bool CreateDirectory(const std::string& path);
    bool RemoveFile(const std::string& filepath);
    std::vector<std::string> ListDirectory(const std::string& path);
}
```

### 时间工具

```cpp
namespace TimeUtils {
    // 获取时间戳
    uint64_t GetTimestampMs();  // 毫秒
    uint64_t GetTimestampUs();  // 微秒
    
    // 格式化时间
    std::string FormatTime(const std::string& format = "%Y-%m-%d %H:%M:%S");
    
    // 计时器
    class Timer {
    public:
        Timer();
        void Start();
        void Stop();
        void Reset();
        uint64_t ElapsedMs() const;
        uint64_t ElapsedUs() const;
    };
    
    // 延迟
    void SleepMs(uint64_t milliseconds);
}
```

### 颜色工具

```cpp
namespace ColorUtils {
    // CSS 颜色到 SkColor
    SkColor ParseColor(const std::string& color_str);
    
    // 支持的格式:
    // - 命名颜色: "red", "blue", "green"
    // - 十六进制: "#FF0000", "#F00"
    // - RGB: "rgb(255, 0, 0)"
    // - RGBA: "rgba(255, 0, 0, 0.5)"
    
    // SkColor 到 CSS
    std::string ToHex(SkColor color);
    std::string ToRgb(SkColor color);
    std::string ToRgba(SkColor color);
    
    // 颜色操作
    SkColor Lighten(SkColor color, float amount);
    SkColor Darken(SkColor color, float amount);
    SkColor SetAlpha(SkColor color, uint8_t alpha);
}
```

### URL 工具

```cpp
namespace UrlUtils {
    struct URL {
        std::string protocol;  // "http", "https", "file"
        std::string host;      // "example.com"
        int port;              // 80, 443
        std::string path;      // "/path/to/resource"
        std::string query;     // "key=value&foo=bar"
        std::string fragment;  // "section"
    };
    
    // 解析 URL
    URL Parse(const std::string& url_str);
    
    // 构建 URL
    std::string Build(const URL& url);
    
    // URL 编码/解码
    std::string Encode(const std::string& str);
    std::string Decode(const std::string& str);
    
    // 查询参数
    std::map<std::string, std::string> ParseQuery(const std::string& query);
    std::string BuildQuery(const std::map<std::string, std::string>& params);
}
```

## 💡 使用示例

### 日志系统

```cpp
// 设置日志级别
Logger::SetLevel(Logger::DEBUG);

// 简单日志
Logger::Info("Application started");
Logger::Warn("This is a warning");
Logger::Error("An error occurred");

// 格式化日志
int count = 42;
std::string name = "MBink";
Logger::Info("Processing {} items for {}", count, name);

// 输出到文件
std::ofstream log_file("app.log");
Logger::SetOutput(&log_file);
```

### JSON 处理

```cpp
// 解析 JSON
std::string json_str = R"({
    "name": "MBink",
    "version": "1.0.0",
    "features": ["DOM", "CSS", "JavaScript"]
})";

auto json = JsonUtils::Parse(json_str);

// 访问数据
auto name = JsonUtils::GetString(json, "name");
if (name.has_value()) {
    std::cout << "Name: " << name.value() << std::endl;
}

// 序列化 JSON
nlohmann::json data;
data["title"] = "Hello";
data["count"] = 42;
std::string output = JsonUtils::Stringify(data, 2);  // 缩进2空格
```

### 字符串操作

```cpp
// 修剪空白
std::string str = "  hello world  ";
std::string trimmed = StringUtils::Trim(str);  // "hello world"

// 分割字符串
std::string path = "a/b/c/d";
auto parts = StringUtils::Split(path, '/');  // ["a", "b", "c", "d"]

// 连接字符串
std::vector<std::string> words = {"hello", "world"};
std::string joined = StringUtils::Join(words, " ");  // "hello world"

// 格式化
std::string msg = StringUtils::Format("User {} has {} points", "Alice", 100);
```

### 文件操作

```cpp
// 读取文件
std::string content = FileUtils::ReadFile("config.json");

// 写入文件
FileUtils::WriteFile("output.txt", "Hello World");

// 检查文件
if (FileUtils::Exists("data.json")) {
    size_t size = FileUtils::GetFileSize("data.json");
    Logger::Info("File size: {} bytes", size);
}

// 路径操作
std::string dir = FileUtils::GetDirectory("/path/to/file.txt");  // "/path/to"
std::string name = FileUtils::GetFilename("/path/to/file.txt");  // "file.txt"
std::string ext = FileUtils::GetExtension("/path/to/file.txt");  // ".txt"
```

### 计时器

```cpp
TimeUtils::Timer timer;

timer.Start();
// 执行一些操作...
timer.Stop();

Logger::Info("Operation took {} ms", timer.ElapsedMs());
```

### 颜色转换

```cpp
// 解析 CSS 颜色
SkColor red = ColorUtils::ParseColor("red");
SkColor hex = ColorUtils::ParseColor("#FF0000");
SkColor rgb = ColorUtils::ParseColor("rgb(255, 0, 0)");
SkColor rgba = ColorUtils::ParseColor("rgba(255, 0, 0, 0.5)");

// 转换为 CSS
std::string hex_str = ColorUtils::ToHex(red);      // "#FF0000"
std::string rgb_str = ColorUtils::ToRgb(red);      // "rgb(255, 0, 0)"
std::string rgba_str = ColorUtils::ToRgba(rgba);   // "rgba(255, 0, 0, 128)"

// 颜色操作
SkColor lighter = ColorUtils::Lighten(red, 0.2f);
SkColor darker = ColorUtils::Darken(red, 0.2f);
```

### URL 处理

```cpp
// 解析 URL
auto url = UrlUtils::Parse("https://example.com:8080/path?key=value#section");

std::cout << "Protocol: " << url.protocol << std::endl;  // "https"
std::cout << "Host: " << url.host << std::endl;          // "example.com"
std::cout << "Port: " << url.port << std::endl;          // 8080
std::cout << "Path: " << url.path << std::endl;          // "/path"

// 解析查询参数
auto params = UrlUtils::ParseQuery(url.query);
std::cout << "key = " << params["key"] << std::endl;     // "value"

// URL 编码
std::string encoded = UrlUtils::Encode("hello world");   // "hello%20world"
std::string decoded = UrlUtils::Decode(encoded);         // "hello world"
```

## 🔗 依赖关系

### 依赖的模块

- `third_party/nlohmann/json` - JSON 库
- `third_party/skia` - Skia (用于颜色类型)

### 被依赖的模块

- **所有模块** - Utils 是基础模块，被所有其他模块使用

## 🏗️ 架构说明

Utils 模块在架构中的位置：

```
┌─────────────────────────────────────────┐
│  All Other Modules                      │
│  (api, dom, event, render, etc.)        │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  Utils Module (core/utils) ← 当前模块    │
│  Logger, JSON, String, File, etc.       │
└─────────────────────────────────────────┘
```

## ⚠️ 注意事项

1. **线程安全**: Logger 是线程安全的，其他工具函数大多不是
2. **异常处理**: 文件操作可能抛出异常，需要适当处理
3. **性能**: 字符串操作频繁时考虑使用 `std::string_view`
4. **编码**: 所有字符串假定为 UTF-8 编码

## 🚀 性能优化

### 避免不必要的字符串拷贝

```cpp
// ❌ 慢：多次拷贝
std::string result = StringUtils::Trim(StringUtils::ToLower(str));

// ✅ 快：减少拷贝
std::string lower = StringUtils::ToLower(str);
std::string result = StringUtils::Trim(lower);
```

### 使用 string_view

```cpp
// 对于只读操作，使用 string_view
bool CheckPrefix(std::string_view str, std::string_view prefix) {
    return str.substr(0, prefix.size()) == prefix;
}
```

### 预分配容器

```cpp
// 预分配 vector 容量
std::vector<std::string> parts;
parts.reserve(10);  // 如果知道大概大小
```

## 📚 相关文档

- [nlohmann/json 文档](https://json.nlohmann.me/)
- [C++ 字符串处理最佳实践](../../docs/CODING_STANDARDS.md)
- [日志系统设计](../../docs/ARCHITECTURE.md#日志系统)

## 🔧 扩展工具

### 自定义日志格式

```cpp
class CustomLogger : public Logger {
public:
    static void Log(Level level, const std::string& message) {
        std::string timestamp = TimeUtils::FormatTime();
        std::string level_str = LevelToString(level);
        std::cout << "[" << timestamp << "] [" << level_str << "] " 
                  << message << std::endl;
    }
};
```

### 自定义 JSON 序列化

```cpp
struct Config {
    std::string name;
    int version;
    
    nlohmann::json ToJson() const {
        return {
            {"name", name},
            {"version", version}
        };
    }
    
    static Config FromJson(const nlohmann::json& json) {
        Config config;
        config.name = json["name"];
        config.version = json["version"];
        return config;
    }
};
```

---

**维护者**: MBink Team  
**最后更新**: 2025-11-12

