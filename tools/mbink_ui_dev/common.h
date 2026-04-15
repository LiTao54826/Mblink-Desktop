#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "third_party/nlohmann/json.hpp"

namespace mbink::ui_dev {

struct ProjectConfig {
    std::string name = "unnamed-project";
    std::string entry = "src/index.html";
    std::string src_dir = "src";
    std::string out_dir = ".dist";
    std::string template_name = "unknown";
    std::string build_builder = "esbuild";
    std::string build_jsx_factory = "Preact.h";
    std::string build_jsx_fragment = "Preact.Fragment";
    std::vector<std::string> build_external = {"preact", "preact/hooks"};
    bool build_sourcemap = true;
    bool build_minify = false;
    int width = 1280;
    int height = 800;
    std::string title = "MBink UI Dev";
};

struct DaemonState {
    bool running = false;
    int pid = 0;
    int runtime_pid = 0;
    std::string started_at;
    std::string project_root;
    ProjectConfig project;
    nlohmann::json last_build;
};

std::filesystem::path GetStateFilePath();
std::string CurrentTimestampIso8601();
int CurrentProcessId();

bool SaveState(const DaemonState& state, std::string* error = nullptr);
std::optional<DaemonState> LoadState(std::string* error = nullptr);
bool RemoveState(std::string* error = nullptr);

ProjectConfig LoadProjectConfig(const std::filesystem::path& project_root, std::string* error = nullptr);

nlohmann::json OkResponse();
nlohmann::json ErrorResponse(const std::string& code, const std::string& message);
nlohmann::json StateToJson(const DaemonState& state);
nlohmann::json ProjectToJson(const std::filesystem::path& root, const ProjectConfig& config);

}  // namespace mbink::ui_dev
