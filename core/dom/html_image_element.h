/**
 * @file html_image_element.h
 * @brief HTML Image元素类
 * 
 * 功能：
 * - 实现<img>元素
 * - 支持src, alt, width, height属性
 * - 图片加载和渲染（支持本地文件和网络URL）
 * - 符合WHATWG HTML标准
 */

#pragma once

#include "element.h"
#include <string>
#include <memory>
#include <functional>
#include "include/core/SkImage.h"

namespace lightui {

/**
 * @brief 图片加载状态
 */
enum class ImageLoadState {
    IDLE,       ///< 空闲（未开始加载）
    LOADING,    ///< 加载中
    COMPLETE,   ///< 加载完成
    ERROR       ///< 加载失败
};

/**
 * @brief HTML Image元素类
 * 
 * 对应HTML标准：
 * https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element
 * 
 * IDL定义：
 * interface HTMLImageElement : HTMLElement {
 *   attribute DOMString src;
 *   attribute DOMString alt;
 *   attribute DOMString crossOrigin;
 *   attribute unsigned long width;
 *   attribute unsigned long height;
 *   readonly attribute unsigned long naturalWidth;
 *   readonly attribute unsigned long naturalHeight;
 *   readonly attribute boolean complete;
 * };
 */
class HTMLImageElement : public Element {
public:
    /**
     * @brief 构造函数
     */
    HTMLImageElement();

    /**
     * @brief 析构函数
     */
    ~HTMLImageElement() override = default;

    // ========== IDL属性 ==========

    /**
     * @brief 获取src属性（图片URL）
     * @return src属性值
     */
    std::string GetSrc() const { return src_; }

    /**
     * @brief 设置src属性
     * @param src 图片URL
     */
    void SetSrc(const std::string& src);

    /**
     * @brief 获取alt属性（替代文本）
     * @return alt属性值
     */
    std::string GetAlt() const { return alt_; }

    /**
     * @brief 设置alt属性
     * @param alt 替代文本
     */
    void SetAlt(const std::string& alt);

    /**
     * @brief 获取width属性（显示宽度）
     * @return width属性值，0表示未设置
     */
    unsigned long GetWidth() const { return width_; }

    /**
     * @brief 设置width属性
     * @param width 显示宽度
     */
    void SetWidth(unsigned long width);

    /**
     * @brief 获取height属性（显示高度）
     * @return height属性值，0表示未设置
     */
    unsigned long GetHeight() const { return height_; }

    /**
     * @brief 设置height属性
     * @param height 显示高度
     */
    void SetHeight(unsigned long height);

    /**
     * @brief 获取naturalWidth（图片原始宽度）
     * @return 图片原始宽度，如果图片未加载则返回0
     */
    unsigned long GetNaturalWidth() const { return natural_width_; }

    /**
     * @brief 获取naturalHeight（图片原始高度）
     * @return 图片原始高度，如果图片未加载则返回0
     */
    unsigned long GetNaturalHeight() const { return natural_height_; }

    /**
     * @brief 获取complete状态（图片是否加载完成）
     * @return true表示加载完成
     */
    bool GetComplete() const { return complete_; }

    /**
     * @brief 获取crossOrigin属性
     * @return crossOrigin属性值
     */
    std::string GetCrossOrigin() const { return cross_origin_; }

    /**
     * @brief 设置crossOrigin属性
     * @param cross_origin crossOrigin属性值
     */
    void SetCrossOrigin(const std::string& cross_origin);

    // ========== 重写方法 ==========

    /**
     * @brief 设置属性
     * @param name 属性名
     * @param value 属性值
     */
    void SetAttribute(const std::string& name, const std::string& value) override;

    /**
     * @brief 移除属性
     * @param name 属性名
     */
    void RemoveAttribute(const std::string& name) override;

    // ========== 图片加载 ==========

    /**
     * @brief 加载图片
     * 
     * 触发图片加载流程：
     * 1. 触发loadstart事件
     * 2. 异步加载图片数据
     * 3. 加载成功触发load事件，失败触发error事件
     * 4. 触发loadend事件
     */
    void LoadImage();
    
    /**
     * @brief 同步加载图片（阻塞）
     * @return 是否加载成功
     */
    bool LoadImageSync();

    /**
     * @brief 获取图片数据（用于渲染）
     * @return SkImage 智能指针，如果未加载则返回nullptr
     */
    sk_sp<SkImage> GetSkImage() const { return sk_image_; }

    /**
     * @brief 获取图片数据（用于渲染，旧接口兼容）
     * @return 图片数据指针，如果未加载则返回nullptr
     */
    void* GetImageData() const { return sk_image_.get(); }

    /**
     * @brief 设置图片数据（用于测试或手动加载）
     * @param data 图片数据指针
     * @param width 图片宽度
     * @param height 图片高度
     */
    void SetImageData(void* data, unsigned long width, unsigned long height);
    
    /**
     * @brief 设置 SkImage（用于直接设置图片）
     * @param image SkImage 智能指针
     */
    void SetSkImage(sk_sp<SkImage> image);
    
    /**
     * @brief 获取加载状态
     * @return 当前加载状态
     */
    ImageLoadState GetLoadState() const { return load_state_; }
    
    /**
     * @brief 获取加载错误信息
     * @return 错误信息，如果没有错误则为空
     */
    std::string GetError() const { return error_message_; }

private:
    /**
     * @brief 触发加载开始事件
     */
    void TriggerLoadStartEvent();

    /**
     * @brief 触发加载成功事件
     */
    void TriggerLoadEvent();

    /**
     * @brief 触发加载失败事件
     */
    void TriggerErrorEvent();

    /**
     * @brief 触发loadend事件
     */
    void TriggerLoadEndEvent();

    /**
     * @brief 解析width/height属性值
     * @param value 属性值字符串
     * @return 解析后的数值，解析失败返回0
     */
    unsigned long ParseDimension(const std::string& value) const;
    
    /**
     * @brief 处理图片加载完成
     * @param image 加载的图片
     * @param error 错误信息（如果有）
     */
    void OnImageLoaded(sk_sp<SkImage> image, const std::string& error);

private:
    std::string src_;               ///< 图片URL
    std::string alt_;               ///< 替代文本
    std::string cross_origin_;      ///< CORS设置
    unsigned long width_;           ///< 显示宽度（0表示未设置）
    unsigned long height_;          ///< 显示高度（0表示未设置）
    unsigned long natural_width_;   ///< 图片原始宽度
    unsigned long natural_height_;  ///< 图片原始高度
    bool complete_;                 ///< 是否加载完成
    sk_sp<SkImage> sk_image_;       ///< Skia 图片对象
    ImageLoadState load_state_;     ///< 加载状态
    std::string error_message_;     ///< 错误信息
};

} // namespace lightui

