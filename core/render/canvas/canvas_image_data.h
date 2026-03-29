/**
 * @file canvas_image_data.h
 * @brief Canvas ImageData类
 */

#pragma once

#include <vector>
#include <cstdint>

namespace mbink {

/**
 * @brief Canvas ImageData类
 * 
 * 对应HTML5 ImageData接口，用于像素级操作
 */
class ImageData {
public:
    /**
     * @brief 构造函数
     * @param width 宽度
     * @param height 高度
     */
    ImageData(unsigned int width, unsigned int height);
    
    /**
     * @brief 从现有数据构造
     * @param data RGBA像素数据
     * @param width 宽度
     * @param height 高度
     */
    ImageData(const std::vector<uint8_t>& data, unsigned int width, unsigned int height);
    
    ~ImageData() = default;
    
    /**
     * @brief 获取宽度
     */
    unsigned int GetWidth() const { return width_; }
    
    /**
     * @brief 获取高度
     */
    unsigned int GetHeight() const { return height_; }
    
    /**
     * @brief 获取像素数据（只读）
     */
    const std::vector<uint8_t>& GetData() const { return data_; }
    
    /**
     * @brief 获取像素数据（可写）
     */
    std::vector<uint8_t>& GetData() { return data_; }
    
    /**
     * @brief 获取指定位置的像素值
     * @param index 数据索引
     * @return 像素值（0-255）
     */
    uint8_t GetPixelValue(size_t index) const;
    
    /**
     * @brief 设置指定位置的像素值
     * @param index 数据索引
     * @param value 像素值（0-255）
     */
    void SetPixelValue(size_t index, uint8_t value);
    
    /**
     * @brief 获取数据长度
     */
    size_t GetDataLength() const { return data_.size(); }

private:
    unsigned int width_;
    unsigned int height_;
    std::vector<uint8_t> data_;  // RGBA格式，每像素4字节
};

} // namespace mbink
