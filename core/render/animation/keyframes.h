/**
 * @file keyframes.h
 * @brief CSS @keyframes 规则定义和解析
 * @author MBlink Development Team
 * @date 2025-11-14
 */

#ifndef MBLINK_CORE_RENDER_KEYFRAMES_H_
#define MBLINK_CORE_RENDER_KEYFRAMES_H_

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace mblink {

/**
 * @brief 关键帧
 * 
 * 表示动画中的一个关键帧，包含时间偏移和该时刻的属性值。
 * 
 * 示例:
 * @code
 * Keyframe kf(0.5f);  // 50% 位置
 * kf.properties["opacity"] = "0.5";
 * kf.properties["transform"] = "translateX(100px)";
 * @endcode
 */
struct Keyframe {
    float offset;  ///< 时间偏移 (0.0 到 1.0, 对应 0% 到 100%)
    std::map<std::string, std::string> properties;  ///< 属性名 -> 属性值
    
    /**
     * @brief 默认构造函数
     */
    Keyframe() : offset(0.0f) {}
    
    /**
     * @brief 构造函数
     * @param offset 时间偏移 (0.0 到 1.0)
     */
    explicit Keyframe(float offset) : offset(offset) {}
    
    /**
     * @brief 比较运算符 (用于排序)
     */
    bool operator<(const Keyframe& other) const {
        return offset < other.offset;
    }
};

/**
 * @brief @keyframes 规则
 * 
 * 表示一个完整的 @keyframes 规则，包含动画名称和所有关键帧。
 * 
 * 支持的 CSS 语法:
 * @code{.css}
 * @keyframes slide-in {
 *     from { transform: translateX(-100%); }
 *     to { transform: translateX(0); }
 * }
 * 
 * @keyframes bounce {
 *     0% { transform: translateY(0); }
 *     50% { transform: translateY(-20px); }
 *     100% { transform: translateY(0); }
 * }
 * 
 * @keyframes fade {
 *     0%, 100% { opacity: 0; }
 *     50% { opacity: 1; }
 * }
 * @endcode
 */
struct KeyframesRule {
    std::string name;  ///< 动画名称
    std::vector<Keyframe> keyframes;  ///< 关键帧列表 (按 offset 排序)
    
    /**
     * @brief 默认构造函数
     */
    KeyframesRule() = default;
    
    /**
     * @brief 构造函数
     * @param name 动画名称
     */
    explicit KeyframesRule(const std::string& name) : name(name) {}
    
    /**
     * @brief 解析 @keyframes 规则
     * 
     * 解析 CSS @keyframes 规则字符串，提取动画名称和所有关键帧。
     * 
     * 支持的语法:
     * - from/to 关键字
     * - 百分比 (0%, 50%, 100%)
     * - 复合选择器 (0%, 100%)
     * - 多个属性
     * 
     * @param css CSS @keyframes 规则字符串
     * @return 解析后的 KeyframesRule 对象
     * 
     * @note 如果解析失败，返回空的 KeyframesRule
     * 
     * 示例:
     * @code
     * std::string css = R"(
     *     @keyframes slide-in {
     *         from { transform: translateX(-100%); opacity: 0; }
     *         to { transform: translateX(0); opacity: 1; }
     *     }
     * )";
     * KeyframesRule rule = KeyframesRule::Parse(css);
     * @endcode
     */
    static KeyframesRule Parse(const std::string& css);
    
    /**
     * @brief 获取指定进度的关键帧
     * 
     * 根据动画进度 (0.0 到 1.0)，找到前后两个关键帧和插值因子。
     * 
     * @param progress 动画进度 (0.0 到 1.0)
     * @return tuple<前一个关键帧, 后一个关键帧, 插值因子>
     *         - 如果 progress <= 第一个关键帧，返回 {&first, &first, 0.0}
     *         - 如果 progress >= 最后一个关键帧，返回 {&last, &last, 1.0}
     *         - 否则返回 {&prev, &next, factor}
     * 
     * @note 插值因子 factor 是在 [prev.offset, next.offset] 区间内的归一化值
     * 
     * 示例:
     * @code
     * KeyframesRule rule = ...;
     * auto [prev, next, factor] = rule.GetKeyframesAt(0.75f);
     * // 在 prev 和 next 之间插值，权重为 factor
     * @endcode
     */
    std::tuple<const Keyframe*, const Keyframe*, float> 
        GetKeyframesAt(float progress) const;
    
    /**
     * @brief 添加关键帧
     * 
     * 添加一个关键帧到规则中，并自动按 offset 排序。
     * 
     * @param keyframe 要添加的关键帧
     */
    void AddKeyframe(const Keyframe& keyframe);
    
    /**
     * @brief 检查规则是否有效
     * 
     * @return true 如果规则有效 (有名称且至少有一个关键帧)
     */
    bool IsValid() const {
        return !name.empty() && !keyframes.empty();
    }
    
private:
    /**
     * @brief 解析关键帧选择器 (0%, 50%, from, to)
     * @param selector 选择器字符串
     * @return 时间偏移列表 (0.0 到 1.0)
     */
    static std::vector<float> ParseSelector(const std::string& selector);
    
    /**
     * @brief 解析属性块 { property: value; ... }
     * @param block 属性块字符串
     * @return 属性名 -> 属性值的映射
     */
    static std::map<std::string, std::string> ParseProperties(const std::string& block);
    
    /**
     * @brief 去除字符串首尾空白
     */
    static std::string Trim(const std::string& str);
};

/**
 * @brief Keyframes 管理器
 * 
 * 管理所有注册的 @keyframes 规则。
 */
class KeyframesManager {
public:
    /**
     * @brief 获取单例实例
     */
    static KeyframesManager& Instance();
    
    /**
     * @brief 注册 @keyframes 规则
     * @param rule 要注册的规则
     */
    void RegisterKeyframes(const KeyframesRule& rule);
    
    /**
     * @brief 获取 @keyframes 规则
     * @param name 动画名称
     * @return 规则指针，如果不存在返回 nullptr
     */
    const KeyframesRule* GetKeyframes(const std::string& name) const;
    
    /**
     * @brief 移除 @keyframes 规则
     * @param name 动画名称
     */
    void RemoveKeyframes(const std::string& name);
    
    /**
     * @brief 清空所有规则
     */
    void Clear();
    
    /**
     * @brief 获取所有规则名称
     */
    std::vector<std::string> GetAllNames() const;
    
private:
    KeyframesManager() = default;
    ~KeyframesManager() = default;
    
    // 禁止拷贝和赋值
    KeyframesManager(const KeyframesManager&) = delete;
    KeyframesManager& operator=(const KeyframesManager&) = delete;
    
    std::map<std::string, KeyframesRule> rules_;
};

} // namespace mblink

#endif // MBLINK_CORE_RENDER_KEYFRAMES_H_

