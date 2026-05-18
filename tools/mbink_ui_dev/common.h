#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "third_party/nlohmann/json.hpp"


namespace mbink::ui_dev {

struct ProjectConfig {
    std::string name = "unnamed-project";
    std::string entry = "src/app.js";
    std::string src_dir = "src";
    std::string out_dir = ".dist";
    std::string template_name = "unknown";
    std::string purpose = "minimal";
    std::string runtime = "tool";
    std::string build_builder = "esbuild";
    std::string build_jsx_factory = "h";
    std::string build_jsx_fragment = "Fragment";
    std::vector<std::string> build_external = {"preact", "preact/hooks"};
    bool build_sourcemap = true;
    bool build_minify = false;
    bool build_hide_console = true;
    int width = 800;
    int height = 600;
    std::string title = "MBink UI Dev";
    bool borderless = false;
    bool resizable = true;
};

struct InitProjectResult {
    std::filesystem::path project_root;
    std::string project_name;
    std::string template_name;
    std::string purpose = "minimal";
    std::string runtime = "tool";
    std::string canonical_key = "minimal/tool";
    std::string requested_purpose;
    std::string requested_runtime;
    std::string legacy_template;
    bool used_default = false;
    std::vector<std::string> layers;
    std::vector<std::string> warnings;
    std::vector<std::string> files_created;
    std::string next_step;
};

struct InitProjectOptions {
    std::string purpose;
    std::string runtime;
    std::string legacy_template;
};

struct ProjectIdentity {
    std::string project_id;
    std::string runtime_id;
    std::filesystem::path project_root;
};

struct DaemonState {
    bool running = false;
    int pid = 0;
    int runtime_pid = 0;
    std::string started_at;
    std::string project_id;
    std::string runtime_id;
    std::string project_root;
    std::string runtime_epoch;
    std::string runtime_status = "stopped";
    std::string runtime_stop_reason = "not_started";
    bool stopping = false;
    ProjectConfig project;
    nlohmann::json last_build;
    nlohmann::json watch;
};

std::filesystem::path GetUiDevFilePath(const std::filesystem::path& project_root);
std::filesystem::path GetProjectsDataRoot();
std::filesystem::path GetProjectDataPath(const std::string& project_id);
std::filesystem::path GetProjectMetadataPath(const std::string& project_id);
std::filesystem::path GetStateFilePath(const std::string& project_id = "");
std::filesystem::path GetRuntimeFilePath(const std::string& project_id, const std::string& file_name);
std::string GetDaemonPipeName(const std::string& project_id);

std::string CurrentTimestampIso8601();
int CurrentProcessId();

bool SaveState(const DaemonState& state, std::string* error = nullptr);
std::optional<DaemonState> LoadState(const std::string& project_id = "", std::string* error = nullptr);
bool RemoveState(const std::string& project_id = "", std::string* error = nullptr);

std::optional<ProjectIdentity> LoadProjectIdentity(const std::filesystem::path& project_root,
                                                   std::string* error = nullptr);
bool EnsureProjectIdentity(const std::filesystem::path& project_root,
                           ProjectIdentity* identity,
                           std::string* error = nullptr);
std::optional<ProjectIdentity> FindProjectIdentityUpwards(const std::filesystem::path& start_path,
                                                          std::string* error = nullptr);
std::vector<ProjectIdentity> ListManagedProjects(std::string* error = nullptr);

ProjectConfig LoadProjectConfig(const std::filesystem::path& project_root, std::string* error = nullptr);
bool InitProject(const std::filesystem::path& target_dir,
                 const std::string& template_name,
                 InitProjectResult* result,
                 std::string* error = nullptr);
bool InitProject(const std::filesystem::path& target_dir,
                 const InitProjectOptions& options,
                 InitProjectResult* result,
                 std::string* error = nullptr);
std::vector<std::string> ListSupportedInitTemplates();
std::vector<std::string> ListSupportedInitCombinations();

nlohmann::json OkResponse();
nlohmann::json ErrorResponse(const std::string& code, const std::string& message);
nlohmann::json StateToJson(const DaemonState& state);
nlohmann::json ProjectToJson(const std::filesystem::path& root, const ProjectConfig& config);

}  // namespace mbink::ui_dev
