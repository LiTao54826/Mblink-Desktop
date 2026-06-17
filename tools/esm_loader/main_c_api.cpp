#include "core/api/mbink.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeTextFile(const std::string& path, const std::string& value) {
    if (path.empty()) return false;
    fs::path out(path);
    if (!out.parent_path().empty()) {
        fs::create_directories(out.parent_path());
    }
    std::ofstream file(out, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file << value;
    return file.good();
}

void writeOwnedJsonFile(const std::string& path,
                        int (*producer)(MBinkHandle, char**),
                        MBinkHandle handle) {
    if (path.empty() || !producer) return;
    char* json = nullptr;
    if (producer(handle, &json) == MBINK_OK && json) {
        writeTextFile(path, json);
    }
    if (json) mbink_free(json);
}

void printUsage(const char* program_name) {
    std::cout << "MBink Loader - ES module / HTML app loader\n\n";
    std::cout << "Usage: " << program_name << " <entry.js|index.html> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --width <n>\n";
    std::cout << "  --height <n>\n";
    std::cout << "  --title <text>\n";
    std::cout << "  --borderless\n";
    std::cout << "  --transparent\n";
    std::cout << "  --no-gpu\n";
    std::cout << "  --min-width <n>\n";
    std::cout << "  --min-height <n>\n";
    std::cout << "  --max-width <n>\n";
    std::cout << "  --max-height <n>\n";
    std::cout << "  --no-scripts\n";
    std::cout << "  --no-official-preact\n";
    std::cout << "  --devtools\n";
    std::cout << "  --devtools-http-mcp [--devtools-http-port <n>] [--devtools-http-token <token>]\n";
    std::cout << "  --ui-dev-snapshot-file <path>\n";
    std::cout << "  --ui-dev-command-file <path>\n";
    std::cout << "  --ui-dev-response-file <path>\n";
    std::cout << "  --ui-dev-console-file <path>\n";
    std::cout << "  --ui-dev-errors-file <path>\n";
    std::cout << "  --ui-dev-lifecycle-file <path>\n";
    std::cout << "  --ui-dev-runtime-epoch <id>\n";
    std::cout << "  --ui-dev-snapshot-max-nodes <n>\n";
    std::cout << "  --ui-dev-snapshot-max-depth <n>\n";
    std::cout << "  --ui-dev-snapshot-root-selector <selector>\n";
    std::cout << "  --ui-dev-snapshot-include-screenshot\n";
    std::cout << "  --ui-dev-snapshot-inline-screenshot\n";
    std::cout << "  --ui-dev-screenshot-file <path>\n";
    std::cout << "  -q, --quit <seconds>\n";
}

struct Options {
    std::string entry_path;
    int width = 1200;
    int height = 800;
    std::string title = "MBink App";
    bool open_devtools = false;
    bool borderless = false;
    bool transparent = false;
    bool gpu = true;
    int min_width = 0;
    int min_height = 0;
    int max_width = 0;
    int max_height = 0;
    bool execute_scripts = true;
    bool load_official_preact = true;
    bool devtools_http_mcp = false;
    unsigned short devtools_http_port = 0;
    std::string devtools_http_token;
    float quit_after_seconds = 0.0f;
    std::string snapshot_file;
    std::string command_file;
    std::string response_file;
    std::string console_file;
    std::string errors_file;
    std::string lifecycle_file;
    std::string runtime_epoch;
    size_t snapshot_max_nodes = 2000;
    int snapshot_max_depth = 64;
    std::string snapshot_root_selector;
    bool snapshot_include_screenshot = false;
    bool snapshot_inline_screenshot = false;
    std::string snapshot_screenshot_file;
};

struct UiDevRuntimeState {
    std::optional<fs::file_time_type> last_command_write_time;
    std::chrono::steady_clock::time_point next_observability_flush =
        (std::chrono::steady_clock::time_point::min)();
};

bool hasObservabilityFiles(const Options& options) {
    return !options.console_file.empty() ||
           !options.errors_file.empty() ||
           !options.lifecycle_file.empty();
}

bool hasCommandChannel(const Options& options) {
    return !options.command_file.empty() && !options.response_file.empty();
}

bool usesUiDevRuntime(const Options& options) {
    return options.open_devtools ||
           options.devtools_http_mcp ||
           !options.snapshot_file.empty() ||
           hasCommandChannel(options) ||
           hasObservabilityFiles(options);
}

bool parseArgs(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto needValue = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << name << " requires a value\n";
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        } else if (arg == "--width") {
            if (auto* v = needValue("--width")) options->width = std::stoi(v);
        } else if (arg == "--height") {
            if (auto* v = needValue("--height")) options->height = std::stoi(v);
        } else if (arg == "--title") {
            if (auto* v = needValue("--title")) options->title = v;
        } else if (arg == "--devtools") {
            options->open_devtools = true;
        } else if (arg == "--devtools-http-mcp") {
            options->devtools_http_mcp = true;
        } else if (arg == "--devtools-http-port") {
            if (auto* v = needValue("--devtools-http-port")) options->devtools_http_port = static_cast<unsigned short>(std::stoi(v));
        } else if (arg == "--devtools-http-token") {
            if (auto* v = needValue("--devtools-http-token")) options->devtools_http_token = v;
        } else if (arg == "--borderless") {
            options->borderless = true;
        } else if (arg == "--transparent") {
            options->transparent = true;
        } else if (arg == "--no-gpu") {
            options->gpu = false;
        } else if (arg == "--no-scripts") {
            options->execute_scripts = false;
        } else if (arg == "--no-official-preact") {
            options->load_official_preact = false;
        } else if (arg == "-q" || arg == "--quit") {
            if (auto* v = needValue(arg.c_str())) options->quit_after_seconds = std::stof(v);
        } else if (arg == "--min-width") {
            if (auto* v = needValue("--min-width")) options->min_width = std::stoi(v);
        } else if (arg == "--min-height") {
            if (auto* v = needValue("--min-height")) options->min_height = std::stoi(v);
        } else if (arg == "--max-width") {
            if (auto* v = needValue("--max-width")) options->max_width = std::stoi(v);
        } else if (arg == "--max-height") {
            if (auto* v = needValue("--max-height")) options->max_height = std::stoi(v);
        } else if (arg == "--ui-dev-snapshot-file") {
            if (auto* v = needValue("--ui-dev-snapshot-file")) options->snapshot_file = v;
        } else if (arg == "--ui-dev-command-file") {
            if (auto* v = needValue("--ui-dev-command-file")) options->command_file = v;
        } else if (arg == "--ui-dev-response-file") {
            if (auto* v = needValue("--ui-dev-response-file")) options->response_file = v;
        } else if (arg == "--ui-dev-console-file") {
            if (auto* v = needValue("--ui-dev-console-file")) options->console_file = v;
        } else if (arg == "--ui-dev-errors-file") {
            if (auto* v = needValue("--ui-dev-errors-file")) options->errors_file = v;
        } else if (arg == "--ui-dev-lifecycle-file") {
            if (auto* v = needValue("--ui-dev-lifecycle-file")) options->lifecycle_file = v;
        } else if (arg == "--ui-dev-runtime-epoch") {
            if (auto* v = needValue("--ui-dev-runtime-epoch")) options->runtime_epoch = v;
        } else if (arg == "--ui-dev-snapshot-max-nodes") {
            if (auto* v = needValue("--ui-dev-snapshot-max-nodes")) options->snapshot_max_nodes = static_cast<size_t>(std::stoul(v));
        } else if (arg == "--ui-dev-snapshot-max-depth") {
            if (auto* v = needValue("--ui-dev-snapshot-max-depth")) options->snapshot_max_depth = std::stoi(v);
        } else if (arg == "--ui-dev-snapshot-root-selector") {
            if (auto* v = needValue("--ui-dev-snapshot-root-selector")) options->snapshot_root_selector = v;
        } else if (arg == "--ui-dev-snapshot-include-screenshot") {
            options->snapshot_include_screenshot = true;
        } else if (arg == "--ui-dev-snapshot-inline-screenshot") {
            options->snapshot_inline_screenshot = true;
            options->snapshot_include_screenshot = true;
        } else if (arg == "--ui-dev-screenshot-file") {
            if (auto* v = needValue("--ui-dev-screenshot-file")) options->snapshot_screenshot_file = v;
        } else if (!arg.empty() && arg[0] != '-') {
            options->entry_path = arg;
        }
    }
    return true;
}

void writeSnapshotIfRequested(MBinkHandle handle, const Options& options) {
    if (options.snapshot_file.empty()) return;
    auto snapshot_options = mbink_ui_dev_default_snapshot_options();
    snapshot_options.runtime_epoch = options.runtime_epoch.empty() ? nullptr : options.runtime_epoch.c_str();
    snapshot_options.max_nodes = options.snapshot_max_nodes;
    snapshot_options.max_depth = options.snapshot_max_depth;
    snapshot_options.root_selector = options.snapshot_root_selector.empty() ? nullptr : options.snapshot_root_selector.c_str();
    snapshot_options.include_screenshot = options.snapshot_include_screenshot;
    snapshot_options.inline_screenshot = options.snapshot_inline_screenshot;
    snapshot_options.screenshot_file = options.snapshot_screenshot_file.empty() ? nullptr : options.snapshot_screenshot_file.c_str();
    if (mbink_ui_dev_snapshot_file(handle, options.snapshot_file.c_str(), &snapshot_options) != MBINK_OK) {
        std::cerr << "snapshot failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
    }
}

void writeObservabilityFiles(MBinkHandle handle, const Options& options) {
    writeOwnedJsonFile(options.console_file, mbink_observe_console_json, handle);
    writeOwnedJsonFile(options.errors_file, mbink_observe_errors_json, handle);
    writeOwnedJsonFile(options.lifecycle_file, mbink_observe_lifecycle_json, handle);
}

void maybeWriteObservabilityFiles(MBinkHandle handle,
                                  const Options& options,
                                  UiDevRuntimeState* state,
                                  bool force = false) {
    if (!hasObservabilityFiles(options) || !state) return;
    const auto now = std::chrono::steady_clock::now();
    if (!force && now < state->next_observability_flush) return;
    writeObservabilityFiles(handle, options);
    state->next_observability_flush = now + std::chrono::milliseconds(250);
}

void handleCommandFile(MBinkHandle handle,
                       const Options& options,
                       std::string* last_command_id,
                       UiDevRuntimeState* state) {
    if (!hasCommandChannel(options) || !state) return;
    std::error_code ec;
    if (!fs::exists(options.command_file, ec) || ec) return;

    const auto write_time = fs::last_write_time(options.command_file, ec);
    if (ec) return;
    if (state->last_command_write_time && *state->last_command_write_time == write_time) {
        return;
    }
    state->last_command_write_time = write_time;

    const auto content = readFile(options.command_file);
    if (content.empty()) return;

    std::string command_id;
    try {
        auto parsed = nlohmann::json::parse(content);
        command_id = parsed.value("id", "");
    } catch (...) {
        return;
    }
    if (command_id.empty() || command_id == *last_command_id) return;

    char* response = nullptr;
    if (mbink_ui_dev_command_json(handle, content.c_str(), &response) == MBINK_OK && response) {
        writeTextFile(options.response_file, response);
        *last_command_id = command_id;
        std::error_code ec;
        fs::remove(options.command_file, ec);
    } else {
        nlohmann::json error{{"ok", false},
                             {"id", command_id},
                             {"error", {{"code", "ui_dev_command_failed"},
                                         {"message", mbink_last_error() ? mbink_last_error() : ""}}}};
        writeTextFile(options.response_file, error.dump(2));
    }
    if (response) mbink_free(response);
}

bool startDevToolsHttpMcp(MBinkHandle handle,
                          const Options& options,
                          MBinkDevToolsHttpInfo* info) {
    if (!options.devtools_http_mcp || !info) return true;
    auto http_options = mbink_devtools_default_http_options();
    http_options.port = options.devtools_http_port;
    http_options.auth_token = options.devtools_http_token.empty() ? nullptr : options.devtools_http_token.c_str();
    http_options.require_auth = true;
    if (mbink_devtools_http_start(handle, &http_options, info) != MBINK_OK) {
        std::cerr << "devtools HTTP MCP failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        return false;
    }
    std::cout << nlohmann::json{{"event", "devtools_http_mcp"},
                                {"url", info->url ? info->url : ""},
                                {"port", info->port},
                                {"auth_token", info->auth_token ? info->auth_token : ""}}.dump()
              << std::endl;
    return true;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    Options options;
    if (!parseArgs(argc, argv, &options)) return 0;
    if (options.entry_path.empty()) {
        std::cerr << "error: missing entry file\n";
        printUsage(argv[0]);
        return 1;
    }
    if (!fs::exists(options.entry_path)) {
        std::cerr << "error: file not found: " << options.entry_path << "\n";
        return 1;
    }
    if ((!options.command_file.empty() || !options.response_file.empty()) && !hasCommandChannel(options)) {
        std::cerr << "error: --ui-dev-command-file and --ui-dev-response-file must be used together\n";
        return 1;
    }

    if (mbink_init() != MBINK_OK) {
        std::cerr << "mbink_init failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        return 1;
    }

    MBinkConfig config = mbink_default_config();
    config.title = options.title.c_str();
    config.width = options.width;
    config.height = options.height;
    config.resizable = true;
    config.borderless = options.borderless || options.transparent;
    config.transparent = options.transparent;
    config.gpu = options.gpu;
    config.min_width = options.min_width;
    config.min_height = options.min_height;
    config.max_width = options.max_width;
    config.max_height = options.max_height;

    MBinkHandle app = mbink_create_ex(&config);
    if (!app) {
        std::cerr << "mbink_create_ex failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        mbink_cleanup();
        return 1;
    }

    MBinkRuntimeOptions runtime_options = mbink_default_runtime_options();
    runtime_options.runtime_epoch = options.runtime_epoch.empty() ? nullptr : options.runtime_epoch.c_str();
    runtime_options.load_embedded_runtime = true;
    runtime_options.load_official_preact = options.load_official_preact;
    if (mbink_configure_runtime(app, &runtime_options) != MBINK_OK) {
        std::cerr << "runtime configure failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        mbink_destroy(app);
        mbink_cleanup();
        return 1;
    }

    if (mbink_load_entry_file(app, options.entry_path.c_str(), options.execute_scripts) != MBINK_OK) {
        std::cerr << "load failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        mbink_destroy(app);
        mbink_cleanup();
        return 1;
    }

    const bool ui_dev_enabled = usesUiDevRuntime(options);
    if (options.open_devtools) {
        if (mbink_devtools_open(app) != MBINK_OK) {
            std::cerr << "devtools failed: " << (mbink_last_error() ? mbink_last_error() : "") << "\n";
        }
    }

    mbink_show(app);
    mbink_render_frame(app, 2);
    mbink_poll_events(app);
    writeSnapshotIfRequested(app, options);
    UiDevRuntimeState ui_dev_state;
    MBinkDevToolsHttpInfo http_info{};
    if (!startDevToolsHttpMcp(app, options, &http_info)) {
        mbink_destroy(app);
        mbink_cleanup();
        return 1;
    }
    maybeWriteObservabilityFiles(app, options, &ui_dev_state, true);

    auto start = std::chrono::steady_clock::now();
    std::string last_command_id;
    while (mbink_poll_events(app)) {
        if (ui_dev_enabled) {
            handleCommandFile(app, options, &last_command_id, &ui_dev_state);
            maybeWriteObservabilityFiles(app, options, &ui_dev_state);
        }

        if (options.quit_after_seconds > 0.0f) {
            const auto elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= options.quit_after_seconds) {
                mbink_stop(app);
                break;
            }
        }
        const auto sleep_ms = ui_dev_enabled ? 2 : 8;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    maybeWriteObservabilityFiles(app, options, &ui_dev_state, true);
    if (options.devtools_http_mcp) {
        mbink_devtools_http_stop(app);
        mbink_devtools_http_info_free(&http_info);
    }
    mbink_destroy(app);
    mbink_cleanup();
    return 0;
}
