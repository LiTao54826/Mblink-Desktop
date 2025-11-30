/**
 * @file font_manager.h
 * @brief 字体管理器
 *
 * 功能：
 * - 管理字体加载、缓存、查找
 * - 支持字体回退机制（包括emoji字体）
 * - 集成系统字体
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "include/core/SkTypeface.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"

namespace lightui {

/**
 * @brief 字体样式
 */
enum class FontStyle {
    NORMAL,     ///< 正常
    ITALIC,     ///< 斜体
    OBLIQUE     ///< 倾斜
};

/**
 * @brief 字体粗细
 */
enum class FontWeight {
    THIN = 100,
    EXTRA_LIGHT = 200,
    LIGHT = 300,
    NORMAL = 400,
    MEDIUM = 500,
    SEMI_BOLD = 600,
    BOLD = 700,
    EXTRA_BOLD = 800,
    BLACK = 900
};

/**
 * @brief 字体描述符
 */
struct FontDescriptor {
    std::string family;         ///< 字体族名
    float size;                 ///< 字体大小
    FontWeight weight;          ///< 字体粗细
    FontStyle style;            ///< 字体样式
    
    FontDescriptor()
        : family("Arial")
        , size(16.0f)
        , weight(FontWeight::NORMAL)
        , style(FontStyle::NORMAL) {}
    
    /**
     * @brief 生成缓存键
     */
    std::string GetCacheKey() const;
};

/**
 * @brief 字体管理器类
 * 
 * 单例模式，管理所有字体资源
 */
class FontManager {
public:
    /**
     * @brief 获取单例实例
     */
    static FontManager& GetInstance();
    
    /**
     * @brief 初始化字体管理器
     */
    void Initialize();
    
    /**
     * @brief 加载字体
     * @param descriptor 字体描述符
     * @return SkFont 对象
     */
    SkFont LoadFont(const FontDescriptor& descriptor);
    
    /**
     * @brief 从文件加载字体
     * @param path 字体文件路径
     * @param size 字体大小
     * @return SkFont 对象
     */
    SkFont LoadFontFromFile(const std::string& path, float size);
    
    /**
     * @brief 获取默认字体
     * @param size 字体大小
     * @return SkFont 对象
     */
    SkFont GetDefaultFont(float size = 16.0f);
    
    /**
     * @brief 清除字体缓存
     */
    void ClearCache();
    
    /**
     * @brief 获取系统字体管理器
     */
    sk_sp<SkFontMgr> GetSystemFontManager() const { return font_mgr_; }

    /**
     * @brief 获取emoji字体
     * @param size 字体大小
     * @return 支持emoji的字体，如果不可用则返回默认字体
     */
    SkFont GetEmojiFont(float size = 16.0f);

    /**
     * @brief 获取emoji字体的typeface
     * @return emoji字体的typeface
     */
    sk_sp<SkTypeface> GetEmojiTypeface();

    /**
     * @brief 获取CJK(中日韩)字体的typeface
     * @return CJK字体的typeface
     */
    sk_sp<SkTypeface> GetCJKTypeface();

    /**
     * @brief 检查字符是否是emoji
     * @param codepoint Unicode码点
     * @return true如果是emoji字符
     */
    static bool IsEmoji(uint32_t codepoint);

    /**
     * @brief 检查字符是否是CJK字符（中日韩文字）
     * @param codepoint Unicode码点
     * @return true如果是CJK字符
     */
    static bool IsCJK(uint32_t codepoint);

    /**
     * @brief 检查typeface是否包含指定字符
     * @param typeface 字体
     * @param codepoint Unicode码点
     * @return true如果字体包含该字符
     */
    static bool TypefaceContainsChar(const sk_sp<SkTypeface>& typeface, uint32_t codepoint);

private:
    FontManager();
    ~FontManager() = default;

    // 禁止拷贝和赋值
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    /**
     * @brief 创建 Skia 字体样式
     */
    SkFontStyle CreateSkFontStyle(FontWeight weight, FontStyle style) const;

    /**
     * @brief 查找字体族
     */
    sk_sp<SkTypeface> FindTypeface(const std::string& family, const SkFontStyle& style);

    /**
     * @brief 初始化emoji字体
     */
    void InitializeEmojiFont();

    /**
     * @brief 初始化CJK字体
     */
    void InitializeCJKFont();

private:
    sk_sp<SkFontMgr> font_mgr_;                                     ///< Skia 字体管理器
    std::unordered_map<std::string, SkFont> font_cache_;            ///< 字体缓存
    std::unordered_map<std::string, sk_sp<SkTypeface>> typeface_cache_;  ///< 字体族缓存
    sk_sp<SkTypeface> emoji_typeface_;                              ///< Emoji字体
    sk_sp<SkTypeface> cjk_typeface_;                                ///< CJK(中日韩)字体
    bool initialized_;                                              ///< 是否已初始化
};

} // namespace lightui

