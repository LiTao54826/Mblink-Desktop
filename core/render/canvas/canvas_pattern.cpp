/**
 * @file canvas_pattern.cpp
 * @brief Canvas图案对象实现
 */

#include "canvas_pattern.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkSamplingOptions.h"
#include <stdexcept>

namespace mbink {

CanvasPattern::CanvasPattern(sk_sp<SkImage> image, PatternRepetition repetition)
    : image_(image)
    , repetition_(repetition) {
    
    if (!image_) {
        throw std::invalid_argument("Invalid image for pattern");
    }
    
    // 根据重复模式设置TileMode
    SkTileMode tileModeX = SkTileMode::kRepeat;
    SkTileMode tileModeY = SkTileMode::kRepeat;
    
    switch (repetition_) {
        case PatternRepetition::REPEAT:
            tileModeX = SkTileMode::kRepeat;
            tileModeY = SkTileMode::kRepeat;
            break;
        case PatternRepetition::REPEAT_X:
            tileModeX = SkTileMode::kRepeat;
            tileModeY = SkTileMode::kDecal;  // 不重复
            break;
        case PatternRepetition::REPEAT_Y:
            tileModeX = SkTileMode::kDecal;
            tileModeY = SkTileMode::kRepeat;
            break;
        case PatternRepetition::NO_REPEAT:
            tileModeX = SkTileMode::kDecal;
            tileModeY = SkTileMode::kDecal;
            break;
    }
    
    // 创建图像shader
    shader_ = image_->makeShader(tileModeX, tileModeY, SkSamplingOptions(), nullptr);
}

CanvasPattern* CanvasPattern::Create(void* image, const std::string& repetition) {
    if (!image) {
        throw std::invalid_argument("Image cannot be null");
    }
    
    SkImage* sk_image = static_cast<SkImage*>(image);
    PatternRepetition rep = ParseRepetition(repetition);
    
    return new CanvasPattern(sk_sp<SkImage>(sk_image), rep);
}

PatternRepetition CanvasPattern::ParseRepetition(const std::string& repetition) {
    if (repetition == "repeat" || repetition.empty()) {
        return PatternRepetition::REPEAT;
    } else if (repetition == "repeat-x") {
        return PatternRepetition::REPEAT_X;
    } else if (repetition == "repeat-y") {
        return PatternRepetition::REPEAT_Y;
    } else if (repetition == "no-repeat") {
        return PatternRepetition::NO_REPEAT;
    } else {
        // 默认为repeat
        return PatternRepetition::REPEAT;
    }
}

} // namespace mbink
