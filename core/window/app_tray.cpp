#include "app_tray.h"

namespace mblink {

#ifdef _WIN32
std::unique_ptr<AppTray> CreateWin32AppTray(const AppTrayConfig& config);
#endif

std::unique_ptr<AppTray> AppTray::CreateForPlatform(const AppTrayConfig& config) {
#ifdef _WIN32
    return CreateWin32AppTray(config);
#else
    (void)config;
    return nullptr;
#endif
}

}  // namespace mblink
