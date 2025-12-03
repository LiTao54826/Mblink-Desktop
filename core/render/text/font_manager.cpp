/**
 * @file font_manager.cpp
 * @brief 字体管理器实现
 */

#include "font_manager.h"
#include "include/core/SkData.h"
#include "include/ports/SkTypeface_win.h"
#include <sstream>
#include <iostream>

namespace lightui {

// ========== FontDescriptor 实现 ==========

std::string FontDescriptor::GetCacheKey() const {
    std::ostringstream oss;
    oss << family << "_" << size << "_" << static_cast<int>(weight) << "_" << static_cast<int>(style);
    return oss.str();
}

// ========== FontManager 实现 ==========

FontManager::FontManager()
    : font_mgr_(nullptr)
    , font_cache_()
    , typeface_cache_()
    , emoji_typeface_(nullptr)
    , initialized_(false) {
}

FontManager& FontManager::GetInstance() {
    static FontManager instance;
    return instance;
}

void FontManager::Initialize() {
    if (initialized_) {
        return;
    }

    // 创建系统字体管理器
#ifdef _WIN32
    font_mgr_ = SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    font_mgr_ = SkFontMgr_New_CoreText(nullptr);
#elif defined(__linux__)
    font_mgr_ = SkFontMgr_New_FontConfig(nullptr);
#else
    font_mgr_ = SkFontMgr::RefDefault();
#endif

    // 初始化emoji字体
    InitializeEmojiFont();

    // 初始化CJK字体
    InitializeCJKFont();

    initialized_ = true;
}

SkFont FontManager::LoadFont(const FontDescriptor& descriptor) {
    if (!initialized_) {
        Initialize();
    }
    
    // 检查缓存
    std::string cache_key = descriptor.GetCacheKey();
    auto it = font_cache_.find(cache_key);
    if (it != font_cache_.end()) {
        return it->second;
    }
    
    // 创建字体样式
    SkFontStyle font_style = CreateSkFontStyle(descriptor.weight, descriptor.style);
    
    // 查找字体族
    sk_sp<SkTypeface> typeface = FindTypeface(descriptor.family, font_style);
    
    // 创建字体
    SkFont font(typeface, descriptor.size);
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setSubpixel(true);
    
    // 缓存字体
    font_cache_[cache_key] = font;
    
    return font;
}

SkFont FontManager::LoadFontFromFile(const std::string& path, float size) {
    if (!initialized_) {
        Initialize();
    }
    
    // 检查缓存
    std::string cache_key = path + "_" + std::to_string(size);
    auto it = font_cache_.find(cache_key);
    if (it != font_cache_.end()) {
        return it->second;
    }
    
    // 从文件加载字体
    sk_sp<SkTypeface> typeface = nullptr;
    if (font_mgr_) {
        typeface = font_mgr_->makeFromFile(path.c_str());
    }

    if (!typeface) {
        // 加载失败，返回默认字体
        return GetDefaultFont(size);
    }
    
    // 创建字体
    SkFont font(typeface, size);
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setSubpixel(true);
    
    // 缓存字体
    font_cache_[cache_key] = font;
    
    return font;
}

SkFont FontManager::GetDefaultFont(float size) {
    if (!initialized_) {
        Initialize();
    }

    FontDescriptor descriptor;
    descriptor.size = size;
    // 默认字体列表：Arial + 中文字体备选
    // Microsoft YaHei (微软雅黑) 和 SimSun (宋体) 支持中文显示
    descriptor.family = "Arial, Microsoft YaHei, SimSun, SimHei";

    return LoadFont(descriptor);
}

void FontManager::ClearCache() {
    font_cache_.clear();
    typeface_cache_.clear();
}

// ========== 私有辅助方法 ==========

SkFontStyle FontManager::CreateSkFontStyle(FontWeight weight, FontStyle style) const {
    int sk_weight = static_cast<int>(weight);
    int sk_width = SkFontStyle::kNormal_Width;
    SkFontStyle::Slant sk_slant = SkFontStyle::kUpright_Slant;
    
    switch (style) {
        case FontStyle::ITALIC:
            sk_slant = SkFontStyle::kItalic_Slant;
            break;
        case FontStyle::OBLIQUE:
            sk_slant = SkFontStyle::kOblique_Slant;
            break;
        default:
            sk_slant = SkFontStyle::kUpright_Slant;
            break;
    }
    
    return SkFontStyle(sk_weight, sk_width, sk_slant);
}

sk_sp<SkTypeface> FontManager::FindTypeface(const std::string& family, const SkFontStyle& style) {
    // 检查缓存
    std::string cache_key = family + "_" + std::to_string(style.weight()) + "_" + std::to_string(style.slant());
    auto it = typeface_cache_.find(cache_key);
    if (it != typeface_cache_.end()) {
        return it->second;
    }

    // 查找字体族
    sk_sp<SkTypeface> typeface;

    if (font_mgr_) {
        // 解析字体列表（用逗号分隔）
        std::istringstream font_stream(family);
        std::string font_name;

        while (std::getline(font_stream, font_name, ',')) {
            // 去除前后空格
            size_t start = font_name.find_first_not_of(" \t\r\n");
            size_t end = font_name.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) {
                continue;
            }
            font_name = font_name.substr(start, end - start + 1);

            // 去除引号
            if (!font_name.empty() && (font_name.front() == '"' || font_name.front() == '\'')) {
                font_name = font_name.substr(1);
            }
            if (!font_name.empty() && (font_name.back() == '"' || font_name.back() == '\'')) {
                font_name = font_name.substr(0, font_name.size() - 1);
            }

            // 尝试匹配字体
            if (!font_name.empty()) {
                typeface = font_mgr_->matchFamilyStyle(font_name.c_str(), style);
                if (typeface) {
                    break;  // 找到字体，停止搜索
                }
            }
        }
    }

    // 如果找不到，使用默认字体
    if (!typeface && font_mgr_) {
        typeface = font_mgr_->matchFamilyStyle(nullptr, SkFontStyle());
    }

    // 如果还是找不到，使用空字体
    if (!typeface) {
        typeface = SkTypeface::MakeEmpty();
    }

    // 缓存字体族
    typeface_cache_[cache_key] = typeface;

    return typeface;
}

// ========== Emoji字体支持 ==========

void FontManager::InitializeEmojiFont() {
    if (!font_mgr_) {
        return;
    }

    // 尝试加载支持emoji的字体
    // Windows上的emoji字体优先级列表
    const std::vector<const char*> emoji_fonts = {
#ifdef _WIN32
        "Segoe UI Emoji",           // Windows 10/11 主要emoji字体
        "Segoe UI Symbol",          // Windows 符号字体
        "Noto Color Emoji",         // Google Noto emoji字体（如果安装）
        "Twemoji Mozilla",          // Mozilla Twemoji（如果安装）
#elif defined(__APPLE__)
        "Apple Color Emoji",        // macOS emoji字体
#else
        "Noto Color Emoji",         // Linux上常用的emoji字体
        "Noto Emoji",               // 黑白emoji
        "EmojiOne",                 // EmojiOne字体
        "Symbola",                  // Symbola字体
#endif
    };

    for (const char* font_name : emoji_fonts) {
        emoji_typeface_ = font_mgr_->matchFamilyStyle(font_name, SkFontStyle());
        if (emoji_typeface_) {
            std::cout << "[FontManager] Emoji font loaded: " << font_name << std::endl;
            break;
        }
    }

    if (!emoji_typeface_) {
        std::cerr << "[FontManager] Warning: No emoji font found, emoji may not display correctly" << std::endl;
    }
}

SkFont FontManager::GetEmojiFont(float size) {
    if (!initialized_) {
        Initialize();
    }

    // 检查缓存
    std::string cache_key = "emoji_" + std::to_string(size);
    auto it = font_cache_.find(cache_key);
    if (it != font_cache_.end()) {
        return it->second;
    }

    sk_sp<SkTypeface> typeface = GetEmojiTypeface();

    // 创建字体
    SkFont font(typeface, size);
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setSubpixel(true);

    // 缓存字体
    font_cache_[cache_key] = font;

    return font;
}

sk_sp<SkTypeface> FontManager::GetEmojiTypeface() {
    if (!initialized_) {
        Initialize();
    }

    if (emoji_typeface_) {
        return emoji_typeface_;
    }

    // 如果没有emoji字体，返回默认字体
    if (font_mgr_) {
        return font_mgr_->matchFamilyStyle(nullptr, SkFontStyle());
    }

    return SkTypeface::MakeEmpty();
}

// ========== CJK(中日韩)字体支持 ==========

void FontManager::InitializeCJKFont() {
    if (!font_mgr_) {
        return;
    }

    // CJK字体优先级列表
    const std::vector<const char*> cjk_fonts = {
#ifdef _WIN32
        "Microsoft YaHei",          // 微软雅黑 (简体中文)
        "Microsoft YaHei UI",       // 微软雅黑UI
        "SimSun",                   // 宋体
        "SimHei",                   // 黑体
        "KaiTi",                    // 楷体
        "DengXian",                 // 等线
        "FangSong",                 // 仿宋
        "NSimSun",                  // 新宋体
#elif defined(__APPLE__)
        "PingFang SC",              // 苹方简体
        "PingFang TC",              // 苹方繁体
        "Hiragino Sans GB",         // 冬青黑体
        "STHeiti",                  // 华文黑体
        "STSong",                   // 华文宋体
#else
        "Noto Sans CJK SC",         // Google Noto CJK
        "Noto Sans SC",             // Google Noto简体
        "Source Han Sans SC",       // 思源黑体
        "WenQuanYi Micro Hei",      // 文泉驿微米黑
        "Droid Sans Fallback",      // Android字体
#endif
    };

    for (const char* font_name : cjk_fonts) {
        cjk_typeface_ = font_mgr_->matchFamilyStyle(font_name, SkFontStyle());
        if (cjk_typeface_) {
            std::cout << "[FontManager] CJK font loaded: " << font_name << std::endl;
            break;
        }
    }

    if (!cjk_typeface_) {
        std::cerr << "[FontManager] Warning: No CJK font found, Chinese characters may not display correctly" << std::endl;
    }
}

sk_sp<SkTypeface> FontManager::GetCJKTypeface() {
    if (!initialized_) {
        Initialize();
    }

    if (cjk_typeface_) {
        return cjk_typeface_;
    }

    // 如果没有CJK字体，返回默认字体
    if (font_mgr_) {
        return font_mgr_->matchFamilyStyle(nullptr, SkFontStyle());
    }

    return SkTypeface::MakeEmpty();
}

sk_sp<SkTypeface> FontManager::GetCJKTypeface(const SkFontStyle& style) {
    if (!initialized_) {
        Initialize();
    }

    // 如果请求的是普通样式，返回缓存的 typeface
    if (style.weight() == SkFontStyle::kNormal_Weight &&
        style.slant() == SkFontStyle::kUpright_Slant) {
        return GetCJKTypeface();
    }

    // CJK 字体列表
    const char* cjk_fonts[] = {
#ifdef _WIN32
        "Microsoft YaHei",      // 微软雅黑
        "SimHei",               // 黑体
        "SimSun",               // 宋体
        "KaiTi",                // 楷体
        "DengXian",             // 等线
#elif defined(__APPLE__)
        "PingFang SC",          // 苹方简体
        "PingFang TC",          // 苹方繁体
        "Hiragino Sans GB",     // 冬青黑体
        "STHeiti",              // 华文黑体
#else
        "Noto Sans CJK SC",     // Google Noto CJK
        "Noto Sans SC",         // Google Noto简体
        "Source Han Sans SC",   // 思源黑体
        "WenQuanYi Micro Hei",  // 文泉驿微米黑
#endif
    };

    // 尝试用指定样式匹配 CJK 字体
    if (font_mgr_) {
        for (const char* font_name : cjk_fonts) {
            auto typeface = font_mgr_->matchFamilyStyle(font_name, style);
            if (typeface) {
                return typeface;
            }
        }
    }

    // 如果找不到带样式的字体，返回普通 CJK 字体
    return GetCJKTypeface();
}

bool FontManager::IsCJK(uint32_t codepoint) {
    // CJK Unified Ideographs (U+4E00–U+9FFF) - 基本汉字
    if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) return true;

    // CJK Unified Ideographs Extension A (U+3400–U+4DBF)
    if (codepoint >= 0x3400 && codepoint <= 0x4DBF) return true;

    // CJK Unified Ideographs Extension B-F (U+20000–U+2A6DF, U+2A700–U+2CEAF, etc.)
    if (codepoint >= 0x20000 && codepoint <= 0x2A6DF) return true;
    if (codepoint >= 0x2A700 && codepoint <= 0x2B73F) return true;
    if (codepoint >= 0x2B740 && codepoint <= 0x2B81F) return true;
    if (codepoint >= 0x2B820 && codepoint <= 0x2CEAF) return true;
    if (codepoint >= 0x2CEB0 && codepoint <= 0x2EBEF) return true;
    if (codepoint >= 0x30000 && codepoint <= 0x3134F) return true;

    // CJK Compatibility Ideographs (U+F900–U+FAFF)
    if (codepoint >= 0xF900 && codepoint <= 0xFAFF) return true;

    // CJK Compatibility Ideographs Supplement (U+2F800–U+2FA1F)
    if (codepoint >= 0x2F800 && codepoint <= 0x2FA1F) return true;

    // CJK Radicals Supplement (U+2E80–U+2EFF)
    if (codepoint >= 0x2E80 && codepoint <= 0x2EFF) return true;

    // Kangxi Radicals (U+2F00–U+2FDF)
    if (codepoint >= 0x2F00 && codepoint <= 0x2FDF) return true;

    // CJK Symbols and Punctuation (U+3000–U+303F)
    if (codepoint >= 0x3000 && codepoint <= 0x303F) return true;

    // Hiragana (U+3040–U+309F)
    if (codepoint >= 0x3040 && codepoint <= 0x309F) return true;

    // Katakana (U+30A0–U+30FF)
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return true;

    // Hangul Syllables (U+AC00–U+D7AF) - 韩文
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return true;

    // Hangul Jamo (U+1100–U+11FF)
    if (codepoint >= 0x1100 && codepoint <= 0x11FF) return true;

    // Bopomofo (U+3100–U+312F) - 注音符号
    if (codepoint >= 0x3100 && codepoint <= 0x312F) return true;

    // Halfwidth and Fullwidth Forms (U+FF00–U+FFEF)
    if (codepoint >= 0xFF00 && codepoint <= 0xFFEF) return true;

    return false;
}

bool FontManager::IsEmoji(uint32_t codepoint) {
    // 检查常见的emoji范围
    // 参考: https://unicode.org/emoji/charts/full-emoji-list.html

    // Miscellaneous Symbols and Pictographs (U+1F300–U+1F5FF)
    if (codepoint >= 0x1F300 && codepoint <= 0x1F5FF) return true;

    // Emoticons (U+1F600–U+1F64F)
    if (codepoint >= 0x1F600 && codepoint <= 0x1F64F) return true;

    // Transport and Map Symbols (U+1F680–U+1F6FF)
    // 包含🚀 (U+1F680)
    if (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) return true;

    // Supplemental Symbols and Pictographs (U+1F900–U+1F9FF)
    if (codepoint >= 0x1F900 && codepoint <= 0x1F9FF) return true;

    // Symbols and Pictographs Extended-A (U+1FA00–U+1FA6F)
    if (codepoint >= 0x1FA00 && codepoint <= 0x1FA6F) return true;

    // Symbols and Pictographs Extended-B (U+1FA70–U+1FAFF)
    if (codepoint >= 0x1FA70 && codepoint <= 0x1FAFF) return true;

    // Dingbats (U+2700–U+27BF) - 包含✓ (U+2713)
    if (codepoint >= 0x2700 && codepoint <= 0x27BF) return true;

    // Miscellaneous Symbols (U+2600–U+26FF) - 包含☀ ☁ ☂等
    if (codepoint >= 0x2600 && codepoint <= 0x26FF) return true;

    // Miscellaneous Technical (U+2300–U+23FF) - 包含⏳ (U+231B), ⌚ (U+231A) 等
    if (codepoint >= 0x2300 && codepoint <= 0x23FF) return true;

    // Regional Indicator Symbols (U+1F1E0–U+1F1FF) - 国旗emoji
    if (codepoint >= 0x1F1E0 && codepoint <= 0x1F1FF) return true;

    // Enclosed Alphanumeric Supplement (U+1F100–U+1F1FF)
    if (codepoint >= 0x1F100 && codepoint <= 0x1F1FF) return true;

    // Mahjong Tiles (U+1F000–U+1F02F)
    if (codepoint >= 0x1F000 && codepoint <= 0x1F02F) return true;

    // Playing Cards (U+1F0A0–U+1F0FF)
    if (codepoint >= 0x1F0A0 && codepoint <= 0x1F0FF) return true;

    // 手势emoji (U+1F44x, U+1F91x等) - 👋 (U+1F44B)
    if (codepoint >= 0x1F440 && codepoint <= 0x1F4FF) return true;

    // 一些常见的特殊符号
    // ❤ (U+2764)
    if (codepoint == 0x2764) return true;

    // Variation Selectors (emoji style) - 用于指定显示为emoji样式
    if (codepoint == 0xFE0F) return true;

    return false;
}

bool FontManager::TypefaceContainsChar(const sk_sp<SkTypeface>& typeface, uint32_t codepoint) {
    if (!typeface) {
        return false;
    }

    // 使用SkTypeface的unicharToGlyph检查字符是否有对应的glyph
    SkGlyphID glyph = typeface->unicharToGlyph(static_cast<SkUnichar>(codepoint));
    return glyph != 0;
}

} // namespace lightui

