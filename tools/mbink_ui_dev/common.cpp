#include "common.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace mbink::ui_dev {

std::filesystem::path GetStateFilePath() {
    // 最小可用版本：使用 tmp 目录存放状态，避免引入平台特定的配置目录逻辑。
    // 后续 P0/P1 再迁移到更正式的路径（如 %LOCALAPPDATA%）。
    return std::filesystem::temp_directory_path() / "mbink-ui-dev.state.json";
}

std::string CurrentTimestampIso8601() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    char buf[64] = {0};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

int CurrentProcessId() {
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

static void ToJson(nlohmann::json& j, const ProjectConfig& p) {
    j = nlohmann::json{
        {"name", p.name},
        {"template", p.template_name},
        {"entry", p.entry},
        {"src_dir", p.src_dir},
        {"out_dir", p.out_dir},
        {"build", {{"builder", p.build_builder},
                   {"jsx_factory", p.build_jsx_factory},
                   {"jsx_fragment", p.build_jsx_fragment},
                   {"external", p.build_external},
                   {"sourcemap", p.build_sourcemap},
                   {"minify", p.build_minify}}},
        {"window", {{"width", p.width}, {"height", p.height}, {"title", p.title}}},
    };
}

static void FromJson(const nlohmann::json& j, ProjectConfig& p) {
    p.name = j.value("name", p.name);
    p.template_name = j.value("template", p.template_name);
    p.entry = j.value("entry", p.entry);
    p.src_dir = j.value("src_dir", p.src_dir);
    p.out_dir = j.value("out_dir", p.out_dir);
    if (j.contains("build")) {
        const auto& b = j.at("build");
        p.build_builder = b.value("builder", p.build_builder);
        p.build_jsx_factory = b.value("jsx_factory", p.build_jsx_factory);
        p.build_jsx_fragment = b.value("jsx_fragment", p.build_jsx_fragment);
        p.build_sourcemap = b.value("sourcemap", p.build_sourcemap);
        p.build_minify = b.value("minify", p.build_minify);
        if (b.contains("external") && b.at("external").is_array()) {
            p.build_external.clear();
            for (const auto& item : b.at("external")) {
                if (item.is_string()) p.build_external.push_back(item.get<std::string>());
            }
        }
    }
    if (j.contains("window")) {
        const auto& w = j.at("window");
        p.width = w.value("width", p.width);
        p.height = w.value("height", p.height);
        p.title = w.value("title", p.title);
    }
}

bool SaveState(const DaemonState& state, std::string* error) {
    try {
        nlohmann::json j;
        j["running"] = state.running;
        j["pid"] = state.pid;
        j["runtime_pid"] = state.runtime_pid;
        j["started_at"] = state.started_at;
        j["project_root"] = state.project_root;
        ToJson(j["project"], state.project);
        j["last_build"] = state.last_build.is_null()
                              ? nlohmann::json{{"ok", true}, {"status", "not_built"}}
                              : state.last_build;

        std::ofstream ofs(GetStateFilePath(), std::ios::binary | std::ios::trunc);
        ofs << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::optional<DaemonState> LoadState(std::string* error) {
    try {
        const auto path = GetStateFilePath();
        if (!std::filesystem::exists(path)) return std::nullopt;

        std::ifstream ifs(path, std::ios::binary);
        std::stringstream ss;
        ss << ifs.rdbuf();

        const auto j = nlohmann::json::parse(ss.str());
        DaemonState state;
        state.running = j.value("running", false);
        state.pid = j.value("pid", 0);
        state.runtime_pid = j.value("runtime_pid", 0);
        state.started_at = j.value("started_at", std::string{});
        state.project_root = j.value("project_root", std::string{});
        if (j.contains("project")) FromJson(j.at("project"), state.project);
        state.last_build = j.value("last_build", nlohmann::json{{"ok", true}, {"status", "not_built"}});
        return state;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

bool RemoveState(std::string* error) {
    try {
        const auto path = GetStateFilePath();
        if (!std::filesystem::exists(path)) return true;
        std::filesystem::remove(path);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

ProjectConfig LoadProjectConfig(const std::filesystem::path& project_root, std::string* error) {
    // P0 最小实现：优先解析 mbink.config.json，缺失时回退到常见入口探测。
    ProjectConfig cfg;
    cfg.name = project_root.filename().string();
    cfg.title = cfg.name.empty() ? cfg.title : cfg.name;

    const auto detect_entry = [&project_root]() -> std::string {
        static const std::vector<std::string> candidates = {
            "index.html", "app.html", "main.html",
            "app.js", "index.js", "main.js",
            "app.mjs", "index.mjs", "main.mjs",
            "src/index.html", "src/app.html", "src/main.html",
            "src/app.js", "src/index.js", "src/main.js",
            "src/app.mjs", "src/index.mjs", "src/main.mjs"
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(project_root / candidate)) return candidate;
        }
        return std::string{"src/index.html"};
    };

    cfg.entry = detect_entry();

    const auto config_path = project_root / "mbink.config.json";
    if (!std::filesystem::exists(config_path)) {
        return cfg;
    }

    try {
        std::ifstream ifs(config_path, std::ios::binary);
        std::stringstream ss;
        ss << ifs.rdbuf();

        const auto j = nlohmann::json::parse(ss.str());
        cfg.name = j.value("name", cfg.name);
        cfg.template_name = j.value("template", cfg.template_name);
        cfg.entry = j.value("entry", cfg.entry);
        cfg.src_dir = j.value("src_dir", cfg.src_dir);
        cfg.out_dir = j.value("out_dir", cfg.out_dir);
        if (j.contains("build")) {
            const auto& b = j.at("build");
            cfg.build_builder = b.value("builder", cfg.build_builder);
            cfg.build_jsx_factory = b.value("jsx_factory", cfg.build_jsx_factory);
            cfg.build_jsx_fragment = b.value("jsx_fragment", cfg.build_jsx_fragment);
            cfg.build_sourcemap = b.value("sourcemap", cfg.build_sourcemap);
            cfg.build_minify = b.value("minify", cfg.build_minify);
            if (b.contains("external") && b.at("external").is_array()) {
                cfg.build_external.clear();
                for (const auto& item : b.at("external")) {
                    if (item.is_string()) cfg.build_external.push_back(item.get<std::string>());
                }
            }
        }
        if (j.contains("window")) {
            const auto& w = j.at("window");
            cfg.title = w.value("title", cfg.title);
            cfg.width = w.value("width", cfg.width);
            cfg.height = w.value("height", cfg.height);
        }
        return cfg;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return cfg;
    }
}

nlohmann::json OkResponse() {
    return nlohmann::json{{"ok", true}};
}

nlohmann::json ErrorResponse(const std::string& code, const std::string& message) {
    return nlohmann::json{{"ok", false}, {"error", {{"code", code}, {"message", message}}}};
}

nlohmann::json StateToJson(const DaemonState& state) {
    nlohmann::json j;
    j["running"] = state.running;
    j["pid"] = state.pid;
    j["runtime_pid"] = state.runtime_pid;
    j["started_at"] = state.started_at;
    j["project_root"] = state.project_root;
    ToJson(j["project"], state.project);
    j["last_build"] = state.last_build.is_null()
                          ? nlohmann::json{{"ok", true}, {"status", "not_built"}}
                          : state.last_build;
    return j;
}

nlohmann::json ProjectToJson(const std::filesystem::path& root, const ProjectConfig& config) {
    nlohmann::json j;
    j["name"] = config.name;
    j["root"] = root.string();
    ToJson(j["config"], config);
    j["entry"] = config.entry;
    j["src_dir"] = config.src_dir;
    j["out_dir"] = config.out_dir;
    j["template"] = config.template_name;
    j["window"] = {{"width", config.width}, {"height", config.height}, {"title", config.title}};
    return j;
}

}  // namespace mbink::ui_dev
