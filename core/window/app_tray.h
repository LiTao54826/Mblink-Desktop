#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mbink {

enum class AppTrayMenuItemType {
    Action,
    Separator,
    Submenu,
};

struct AppTrayMenuItem {
    std::string id;
    std::string label;
    AppTrayMenuItemType type = AppTrayMenuItemType::Action;
    bool enabled = true;
    bool checked = false;
    std::vector<AppTrayMenuItem> children;
};

struct AppTrayConfig {
    std::string tooltip;
    void* owner_native_window = nullptr;
    bool reserve_custom_panel = true;
};

class AppTray {
public:
    virtual ~AppTray() = default;

    virtual bool Create() = 0;
    virtual void Destroy() = 0;
    virtual void SetTooltip(const std::string& tooltip) = 0;
    virtual void SetMenuItems(const std::vector<AppTrayMenuItem>& items) = 0;
    virtual void SetLeftClickHandler(std::function<void()> handler) = 0;
    virtual void SetMenuItemHandler(std::function<void(const std::string&)> handler) = 0;

    static std::unique_ptr<AppTray> CreateForPlatform(const AppTrayConfig& config);
};

}  // namespace mbink
