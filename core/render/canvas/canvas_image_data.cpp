/**
 * @file canvas_image_data.cpp
 * @brief Canvas ImageData类实现
 */

#include "canvas_image_data.h"

namespace mblink {

ImageData::ImageData(unsigned int width, unsigned int height)
    : width_(width)
    , height_(height)
    , data_(width * height * 4, 0) {  // RGBA，初始化为透明黑色
}

ImageData::ImageData(const std::vector<uint8_t>& data, unsigned int width, unsigned int height)
    : width_(width)
    , height_(height)
    , data_(data) {
    // 确保数据大小正确
    size_t expected_size = width * height * 4;
    if (data_.size() < expected_size) {
        data_.resize(expected_size, 0);
    }
}

uint8_t ImageData::GetPixelValue(size_t index) const {
    if (index < data_.size()) {
        return data_[index];
    }
    return 0;
}

void ImageData::SetPixelValue(size_t index, uint8_t value) {
    if (index < data_.size()) {
        data_[index] = value;
    }
}

} // namespace mblink
