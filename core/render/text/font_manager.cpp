/**
 * @file font_manager.cpp
 * @brief 字体管理器实现
 */

#include "font_manager.h"
#include "include/core/SkData.h"
#include "include/ports/SkTypeface_win.h"
#include <sstream>

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
    descriptor.family = "Arial";  // 默认使用 Arial
    
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
        typeface = font_mgr_->matchFamilyStyle(family.c_str(), style);
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

} // namespace lightui

