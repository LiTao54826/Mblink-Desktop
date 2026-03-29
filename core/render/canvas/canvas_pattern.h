/**
 * @file canvas_pattern.h
 * @brief Canvas图案对象
 */

#pragma once

#include "include/core/SkShader.h"
#include "include/core/SkImage.h"
#include <string>

namespace mbink {

/**
 * @brief Canvas图案重复模式
 */
enum class PatternRepetition {
    REPEAT,       // "repeat" - 水平和垂直重复
    REPEAT_X,     // "repeat-x" - 仅水平重复
    REPEAT_Y,     // "repeat-y" - 仅垂直重复
    NO_REPEAT     // "no-repeat" - 不重复
};

/**
 * @brief Canvas图案对象
 * 
 * 对应HTML5 CanvasPattern接口
 * 使用Skia的SkShader实现图案填充
 */
class CanvasPattern {
public:
    /**
     * @brief 创建图案
     * @param image 图像指针（SkImage*）
     * @param repetition 重复模式字符串
     * @return CanvasPattern指针
     */
    static CanvasPattern* Create(void* image, const std::string& repetition);
    
    /**
     * @brief 获取Skia Shader
     * @return Skia Shader指针
     */
    sk_sp<SkShader> GetShader() const { return shader_; }
    
    /**
     * @brief 获取重复模式
     */
    PatternRepetition GetRepetition() const { return repetition_; }

private:
    /**
     * @brief 私有构造函数
     */
    CanvasPattern(sk_sp<SkImage> image, PatternRepetition repetition);
    
    /**
     * @brief 解析重复模式字符串
     */
    static PatternRepetition ParseRepetition(const std::string& repetition);
    
    sk_sp<SkImage> image_;           ///< 图像
    PatternRepetition repetition_;   ///< 重复模式
    sk_sp<SkShader> shader_;         ///< Skia Shader
};

} // namespace mbink
