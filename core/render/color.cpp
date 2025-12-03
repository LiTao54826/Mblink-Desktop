/**
 * @file color.cpp
 * @brief 颜色管理模块实现
 */

#include "color.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace lightui {

// 静态成员初始化
std::unordered_map<std::string, SkColor> Color::named_colors_;
bool Color::named_colors_initialized_ = false;

// ========== 颜色创建 ==========

SkColor Color::FromRGB(int r, int g, int b) {
    return SkColorSetRGB(
        std::clamp(r, 0, 255),
        std::clamp(g, 0, 255),
        std::clamp(b, 0, 255)
    );
}

SkColor Color::FromRGBA(int r, int g, int b, int a) {
    return SkColorSetARGB(
        std::clamp(a, 0, 255),
        std::clamp(r, 0, 255),
        std::clamp(g, 0, 255),
        std::clamp(b, 0, 255)
    );
}

SkColor Color::FromHex(const std::string& hex) {
    std::string h = hex;
    
    // 移除 # 前缀
    if (!h.empty() && h[0] == '#') {
        h = h.substr(1);
    }
    
    // 转换为大写
    std::transform(h.begin(), h.end(), h.begin(), ::toupper);
    
    // 解析不同长度的 HEX
    if (h.length() == 3) {
        // #RGB -> #RRGGBB
        int r = std::stoi(h.substr(0, 1), nullptr, 16) * 17;
        int g = std::stoi(h.substr(1, 1), nullptr, 16) * 17;
        int b = std::stoi(h.substr(2, 1), nullptr, 16) * 17;
        return FromRGB(r, g, b);
    } else if (h.length() == 6) {
        // #RRGGBB
        int r = std::stoi(h.substr(0, 2), nullptr, 16);
        int g = std::stoi(h.substr(2, 2), nullptr, 16);
        int b = std::stoi(h.substr(4, 2), nullptr, 16);
        return FromRGB(r, g, b);
    } else if (h.length() == 8) {
        // #RRGGBBAA
        int r = std::stoi(h.substr(0, 2), nullptr, 16);
        int g = std::stoi(h.substr(2, 2), nullptr, 16);
        int b = std::stoi(h.substr(4, 2), nullptr, 16);
        int a = std::stoi(h.substr(6, 2), nullptr, 16);
        return FromRGBA(r, g, b, a);
    }
    
    // 默认返回黑色
    return SK_ColorBLACK;
}

SkColor Color::FromName(const std::string& name) {
    if (!named_colors_initialized_) {
        InitNamedColors();
    }
    
    // 转换为小写
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    auto it = named_colors_.find(lower_name);
    if (it != named_colors_.end()) {
        return it->second;
    }
    
    // 默认返回黑色
    return SK_ColorBLACK;
}

SkColor Color::Parse(const std::string& str) {
    if (str.empty()) {
        return SK_ColorBLACK;
    }
    
    // 去除首尾空格
    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    // HEX 格式
    if (trimmed[0] == '#') {
        return FromHex(trimmed);
    }
    
    // rgb() 或 rgba() 格式
    if (trimmed.substr(0, 4) == "rgb(" || trimmed.substr(0, 5) == "rgba(") {
        return ParseRgbString(trimmed);
    }
    
    // 命名颜色
    return FromName(trimmed);
}

// ========== 颜色分量获取 ==========

int Color::GetRed(SkColor color) {
    return SkColorGetR(color);
}

int Color::GetGreen(SkColor color) {
    return SkColorGetG(color);
}

int Color::GetBlue(SkColor color) {
    return SkColorGetB(color);
}

int Color::GetAlpha(SkColor color) {
    return SkColorGetA(color);
}

// ========== 颜色转换 ==========

std::string Color::ToHex(SkColor color, bool includeAlpha) {
    std::ostringstream oss;
    oss << "#";
    oss << std::hex << std::setfill('0');
    oss << std::setw(2) << GetRed(color);
    oss << std::setw(2) << GetGreen(color);
    oss << std::setw(2) << GetBlue(color);
    
    if (includeAlpha) {
        oss << std::setw(2) << GetAlpha(color);
    }
    
    std::string result = oss.str();
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ========== 私有辅助方法 ==========

void Color::InitNamedColors() {
    if (named_colors_initialized_) {
        return;
    }
    
    // CSS 命名颜色（使用 CSS 标准颜色值）
    named_colors_["black"] = SK_ColorBLACK;           // #000000
    named_colors_["white"] = SK_ColorWHITE;           // #FFFFFF
    named_colors_["red"] = SkColorSetRGB(255, 0, 0);  // #FF0000
    named_colors_["green"] = SkColorSetRGB(0, 128, 0); // #008000 (CSS 标准 green，不是亮绿)
    named_colors_["blue"] = SkColorSetRGB(0, 0, 255); // #0000FF
    named_colors_["yellow"] = SK_ColorYELLOW;         // #FFFF00
    named_colors_["cyan"] = SK_ColorCYAN;             // #00FFFF
    named_colors_["magenta"] = SK_ColorMAGENTA;       // #FF00FF (同 fuchsia)
    named_colors_["gray"] = SkColorSetRGB(128, 128, 128);   // #808080
    named_colors_["grey"] = SkColorSetRGB(128, 128, 128);   // #808080
    named_colors_["darkgray"] = SkColorSetRGB(169, 169, 169);   // #A9A9A9
    named_colors_["darkgrey"] = SkColorSetRGB(169, 169, 169);   // #A9A9A9
    named_colors_["lightgray"] = SkColorSetRGB(211, 211, 211);  // #D3D3D3
    named_colors_["lightgrey"] = SkColorSetRGB(211, 211, 211);  // #D3D3D3
    named_colors_["transparent"] = SK_ColorTRANSPARENT;

    // 更多 CSS 标准颜色
    named_colors_["orange"] = SkColorSetRGB(255, 165, 0);   // #FFA500
    named_colors_["purple"] = SkColorSetRGB(128, 0, 128);   // #800080
    named_colors_["pink"] = SkColorSetRGB(255, 192, 203);   // #FFC0CB
    named_colors_["brown"] = SkColorSetRGB(165, 42, 42);    // #A52A2A
    named_colors_["navy"] = SkColorSetRGB(0, 0, 128);       // #000080
    named_colors_["teal"] = SkColorSetRGB(0, 128, 128);     // #008080
    named_colors_["olive"] = SkColorSetRGB(128, 128, 0);    // #808000
    named_colors_["lime"] = SkColorSetRGB(0, 255, 0);       // #00FF00 (亮绿色)
    named_colors_["aqua"] = SkColorSetRGB(0, 255, 255);     // #00FFFF (同 cyan)
    named_colors_["maroon"] = SkColorSetRGB(128, 0, 0);     // #800000
    named_colors_["silver"] = SkColorSetRGB(192, 192, 192); // #C0C0C0
    named_colors_["fuchsia"] = SkColorSetRGB(255, 0, 255);  // #FF00FF (同 magenta)
    
    named_colors_initialized_ = true;
}

SkColor Color::ParseRgbString(const std::string& str) {
    // 查找括号
    size_t start = str.find('(');
    size_t end = str.find(')');
    
    if (start == std::string::npos || end == std::string::npos) {
        return SK_ColorBLACK;
    }
    
    // 提取括号内的内容
    std::string content = str.substr(start + 1, end - start - 1);
    
    // 分割逗号
    std::vector<int> values;
    std::istringstream iss(content);
    std::string token;
    
    while (std::getline(iss, token, ',')) {
        // 去除空格
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        
        // 解析数值
        try {
            // 处理百分比
            if (token.find('%') != std::string::npos) {
                float percent = std::stof(token);
                values.push_back(static_cast<int>(percent * 255.0f / 100.0f));
            } else if (token.find('.') != std::string::npos) {
                // 浮点数 (0.0-1.0)
                float val = std::stof(token);
                values.push_back(static_cast<int>(val * 255.0f));
            } else {
                // 整数
                values.push_back(std::stoi(token));
            }
        } catch (...) {
            values.push_back(0);
        }
    }
    
    // 根据值的数量创建颜色
    if (values.size() == 3) {
        return FromRGB(values[0], values[1], values[2]);
    } else if (values.size() == 4) {
        return FromRGBA(values[0], values[1], values[2], values[3]);
    }
    
    return SK_ColorBLACK;
}

} // namespace lightui

