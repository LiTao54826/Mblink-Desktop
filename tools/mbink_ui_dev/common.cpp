#include "common.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <cstddef>
#include <utility>


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace mbink::ui_dev {

#ifdef _WIN32
std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (size <= 0) {
        code_page = CP_ACP;
        flags = 0;
        size = MultiByteToWideChar(code_page, flags, value.data(), static_cast<int>(value.size()), nullptr, 0);
    }
    if (size <= 0) return L"";
    std::wstring result(size, L'\0');
    MultiByteToWideChar(code_page, flags, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}
#endif

std::filesystem::path PathFromUtf8(const std::string& value) {
#ifdef _WIN32
    return std::filesystem::path(Utf8ToWide(value));
#else
    return std::filesystem::path(value);
#endif
}

std::string PathToUtf8(const std::filesystem::path& path) {
#ifdef _WIN32
    return WideToUtf8(path.wstring());
#else
    return path.string();
#endif
}

namespace {

std::string NormalizePathString(const std::filesystem::path& path) {
    auto normalized = PathToUtf8(std::filesystem::absolute(path).lexically_normal());
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

std::string SanitizeIdComponent(std::string value) {
    for (char& ch : value) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if ((uch >= 'a' && uch <= 'z') || (uch >= 'A' && uch <= 'Z') || (uch >= '0' && uch <= '9')) continue;
        ch = '-';
    }
    return value;
}

std::string GenerateOpaqueId(const char* prefix) {
    const auto now = static_cast<unsigned long long>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::mt19937_64 rng(std::random_device{}());
    const auto value = now ^ (static_cast<unsigned long long>(CurrentProcessId()) << 32U) ^ rng();
    std::ostringstream oss;
    oss << prefix << '-' << std::hex << value;
    return oss.str();
}

bool WriteJsonFile(const std::filesystem::path& path, const nlohmann::json& value, std::string* error) {
    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) *error = "创建目录失败: " + parent.string();
        return false;
    }
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        if (error) *error = "写入文件失败: " + path.string();
        return false;
    }
    ofs << value.dump(2);
    if (!ofs.good()) {
        if (error) *error = "写入文件失败: " + path.string();
        return false;
    }
    return true;
}

std::optional<nlohmann::json> ReadJsonFile(const std::filesystem::path& path, std::string* error) {
    if (!std::filesystem::exists(path)) return std::nullopt;
    try {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            if (error) *error = "读取文件失败: " + path.string();
            return std::nullopt;
        }
        return nlohmann::json::parse(ifs);
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

void ToJson(nlohmann::json& j, const ProjectIdentity& identity) {
    j = nlohmann::json{{"version", 1},
                       {"project_id", identity.project_id},
                       {"runtime_id", identity.runtime_id.empty() ? identity.project_id : identity.runtime_id},
                       {"project_root", NormalizePathString(identity.project_root)}};
}

void FromJson(const nlohmann::json& j, ProjectIdentity& identity) {
    identity.project_id = j.value("project_id", std::string{});
    identity.runtime_id = j.value("runtime_id", identity.project_id);
    identity.project_root = PathFromUtf8(j.value("project_root", std::string{}));
}


struct EmbeddedTemplateEntry {
    const char* path;
    const unsigned char* data;
    size_t size;
};

#include "generated_templates/embedded_template_registry.inc"

std::string EmbeddedTemplateText(const EmbeddedTemplateEntry& entry) {
    return std::string(reinterpret_cast<const char*>(entry.data), entry.size);
}

bool LooksLikeMbinkRepoRoot(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path / "bindings" / "python" / "mbink", ec) &&
           std::filesystem::exists(path / "bindings" / "rust" / "mbink" / "Cargo.toml", ec) &&
           std::filesystem::exists(path / "bindings" / "go" / "go.mod", ec);
}

std::optional<std::filesystem::path> FindMbinkRepoRootUpwards(std::filesystem::path start) {
    std::error_code ec;
    start = std::filesystem::absolute(start.empty() ? std::filesystem::current_path() : start, ec).lexically_normal();
    if (ec) return std::nullopt;
    if (std::filesystem::exists(start, ec) && !std::filesystem::is_directory(start, ec)) start = start.parent_path();
    while (!start.empty()) {
        if (LooksLikeMbinkRepoRoot(start)) return start;
        const auto parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return std::nullopt;
}

std::filesystem::path CurrentExecutablePath() {
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (n == 0) return {};
        if (n < buffer.size()) {
            buffer.resize(n);
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    return std::filesystem::current_path();
#endif
}

std::string RuntimeLibraryFileName() {
#ifdef _WIN32
    return "mbink.dll";
#elif defined(__APPLE__)
    return "libmbink.dylib";
#else
    return "libmbink.so";
#endif
}

bool CopyRuntimeLibraryFromToolDirectory(const std::filesystem::path& output_path,
                                         std::string* error) {
    const auto source = CurrentExecutablePath().parent_path() / RuntimeLibraryFileName();
    std::error_code ec;
    if (!std::filesystem::exists(source, ec) || std::filesystem::is_directory(source, ec)) {
        if (error) {
            *error = "MBink runtime library not found next to mbink-ui-dev: " + source.string();
        }
        return false;
    }

    const auto parent = output_path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, ec);
    if (ec) {
        if (error) *error = "failed to create runtime library directory: " + parent.string();
        return false;
    }

    std::filesystem::copy_file(source, output_path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (error) *error = "failed to copy MBink runtime library: " + ec.message();
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> RuntimeLibraryTargetsForProject(const std::filesystem::path& root,
                                                                   const std::string& runtime) {
    const auto file_name = RuntimeLibraryFileName();
    if (runtime == "rust") {
        return {root / "rust_host" / "vendor" / "mbink-sys" / "runtime" / file_name};
    }
    if (runtime == "go") {
        return {root / ".mbink" / "mbink-go" / file_name};
    }
    if (runtime == "python") {
        return {root / "vendor" / "mbink" / "bin" / file_name};
    }
    return {};
}

bool CopyRuntimeLibrariesForProject(const std::filesystem::path& root,
                                    const std::string& runtime,
                                    std::vector<std::string>* files_created,
                                    std::string* error) {
    for (const auto& target : RuntimeLibraryTargetsForProject(root, runtime)) {
        if (!CopyRuntimeLibraryFromToolDirectory(target, error)) return false;
        if (files_created) {
            const auto rel_path = std::filesystem::relative(target, root).generic_string();
            if (std::find(files_created->begin(), files_created->end(), rel_path) == files_created->end()) {
                files_created->push_back(rel_path);
            }
        }
    }
    return true;
}

std::filesystem::path ResolveMbinkRepoRoot(const std::filesystem::path& project_root) {
    std::vector<std::filesystem::path> candidates;
    if (const char* env_root = std::getenv("MBINK_REPO_ROOT"); env_root && *env_root) {
        candidates.emplace_back(env_root);
    }
    candidates.push_back(project_root);
    candidates.push_back(std::filesystem::current_path());
    candidates.push_back(CurrentExecutablePath().parent_path());
    for (const auto& candidate : candidates) {
        if (const auto found = FindMbinkRepoRootUpwards(candidate); found.has_value()) return *found;
    }
    return {};
}

void ReplaceAll(std::string& value, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string ApplyEmbeddedTemplateVariables(std::string content,
                                           const std::filesystem::path& project_root,
                                           const std::string& project_name) {
    const auto repo_root = ResolveMbinkRepoRoot(project_root);
    const auto repo_root_value = repo_root.empty() ? std::string{} : repo_root.generic_string();
    ReplaceAll(content, "{{MBINK_REPO_ROOT}}", repo_root_value);
    ReplaceAll(content, "{{PROJECT_NAME}}", project_name);
    return content;
}

struct TemplateResolution {
    std::string purpose = "minimal";
    std::string runtime = "tool";
    std::string canonical_key = "minimal/tool";
    std::string requested_purpose;
    std::string requested_runtime;
    std::string legacy_template;
    bool used_default = false;
    std::vector<std::string> layers;
    std::vector<std::string> warnings;
};

struct LegacyTemplateMapping {
    const char* legacy;
    const char* purpose;
    const char* runtime;
    const char* warning;
};

std::string CanonicalKey(const std::string& purpose, const std::string& runtime) {
    return purpose + "/" + runtime;
}

std::vector<std::string> CanonicalLayers(const std::string& purpose, const std::string& runtime) {
    std::vector<std::string> layers = {"base/common"};
    if (runtime != "tool") layers.push_back("runtime/" + runtime);
    layers.push_back("purpose/" + purpose + "/shared");
    layers.push_back("purpose/" + purpose + "/runtime/" + runtime);
    return layers;
}

bool IsSupportedPurpose(const std::string& purpose) {
    return purpose == "minimal" || purpose == "showcase" || purpose == "desktop-app";
}

bool IsSupportedRuntime(const std::string& runtime) {
    return runtime == "tool" || runtime == "python" || runtime == "rust" || runtime == "go";
}

const LegacyTemplateMapping* FindLegacyTemplateMapping(const std::string& legacy_template) {
    static constexpr LegacyTemplateMapping kMappings[] = {
        {"preact-jsx", "minimal", "tool", "legacy template 'preact-jsx' maps to canonical minimal/tool"},
        {"preact-ts", "minimal", "tool", "legacy template 'preact-ts' maps to canonical minimal/tool; TypeScript flavor is no longer canonical"},
        {"vanilla-js", "minimal", "tool", "legacy template 'vanilla-js' maps to canonical minimal/tool"},
        {"python", "showcase", "python", "legacy template 'python' maps to canonical showcase/python"},
        {"go", "showcase", "go", "legacy template 'go' maps to canonical showcase/go"},
        {"rust", "showcase", "rust", "legacy template 'rust' maps to canonical showcase/rust"},
        {"python-host", "showcase", "python", "legacy template 'python-host' maps to canonical showcase/python"},
        {"rust-host", "showcase", "rust", "legacy template 'rust-host' maps to canonical showcase/rust"},
    };
    for (const auto& mapping : kMappings) {
        if (legacy_template == mapping.legacy) return &mapping;
    }
    return nullptr;
}

std::vector<std::string> SupportedCanonicalKeys() {
    std::vector<std::string> keys;
    for (const auto& purpose : {"minimal", "showcase", "desktop-app"}) {
        for (const auto& runtime : {"tool", "python", "rust", "go"}) {
            keys.push_back(CanonicalKey(purpose, runtime));
        }
    }
    return keys;
}

bool ResolveInitTemplate(const InitProjectOptions& options, TemplateResolution* resolution, std::string* error) {
    if (resolution) *resolution = TemplateResolution{};
    const bool has_purpose = !options.purpose.empty();
    const bool has_runtime = !options.runtime.empty();
    const bool has_legacy = !options.legacy_template.empty();

    TemplateResolution resolved;
    resolved.requested_purpose = options.purpose;
    resolved.requested_runtime = options.runtime;
    if (!has_purpose && !has_runtime && !has_legacy) {
        resolved.used_default = true;
    } else if (has_purpose != has_runtime) {
        if (error) *error = "purpose and runtime must be provided together; bare init defaults to minimal/tool";
        return false;
    } else if (has_purpose) {
        if (!IsSupportedPurpose(options.purpose)) {
            if (error) *error = "unsupported purpose: " + options.purpose;
            return false;
        }
        if (!IsSupportedRuntime(options.runtime)) {
            if (error) *error = "unsupported runtime: " + options.runtime;
            return false;
        }
        resolved.purpose = options.purpose;
        resolved.runtime = options.runtime;
    }

    if (has_legacy) {
        const auto* mapping = FindLegacyTemplateMapping(options.legacy_template);
        if (!mapping) {
            if (error) *error = "unsupported legacy template: " + options.legacy_template;
            return false;
        }
        const std::string mapped_key = CanonicalKey(mapping->purpose, mapping->runtime);
        if (has_purpose && mapped_key != CanonicalKey(resolved.purpose, resolved.runtime)) {
            if (error) *error = "legacy template conflicts with purpose/runtime: " + options.legacy_template + " -> " + mapped_key;
            return false;
        }
        resolved.purpose = mapping->purpose;
        resolved.runtime = mapping->runtime;
        resolved.legacy_template = options.legacy_template;
        resolved.warnings.push_back(mapping->warning);
    }

    resolved.canonical_key = CanonicalKey(resolved.purpose, resolved.runtime);
    resolved.layers = CanonicalLayers(resolved.purpose, resolved.runtime);
    if (resolved.canonical_key == "showcase/tool") {
        resolved.warnings.push_back("showcase/tool is supported with caveat: tray/native host behavior is documented in UI but unavailable without a host binding runtime");
    }
    if (resolution) *resolution = resolved;
    return true;
}

bool IsHiddenEmbeddedTemplatePath(const std::string& rel_path) {
    for (const auto& part : std::filesystem::path(rel_path)) {
        const auto name = part.string();
        if (name == ".mbink") continue;
        if (!name.empty() && name[0] == '.') return true;
    }
    return false;
}

std::vector<const EmbeddedTemplateEntry*> FindEmbeddedTemplateEntries(const std::string& template_name) {
    std::vector<const EmbeddedTemplateEntry*> entries;
    const std::string prefix = template_name + "/";
    for (const auto& entry : kEmbeddedTemplateEntries) {
        const std::string full_path = entry.path ? entry.path : "";
        if (full_path.rfind(prefix, 0) != 0) continue;
        const std::string rel_path = full_path.substr(prefix.size());
        if (rel_path.empty() || IsHiddenEmbeddedTemplatePath(rel_path)) continue;
        entries.push_back(&entry);
    }
    std::sort(entries.begin(), entries.end(), [](const EmbeddedTemplateEntry* lhs, const EmbeddedTemplateEntry* rhs) {
        return std::string(lhs->path ? lhs->path : "") < std::string(rhs->path ? rhs->path : "");
    });
    return entries;
}

bool TryInitProjectFromEmbeddedTemplates(const std::filesystem::path& root,
                                         const std::string& project_name,
                                         const std::string& template_name,
                                         InitProjectResult* result,
                                         std::string* error) {
    if (error) error->clear();
    const auto entries = FindEmbeddedTemplateEntries(template_name);
    if (entries.empty()) return false;

    std::vector<std::string> files_created;
    const std::string prefix = template_name + "/";
    std::error_code ec;
    for (const auto* entry : entries) {
        const std::string rel_path = std::string(entry->path).substr(prefix.size());
        const auto output_path = root / std::filesystem::path(rel_path);
        const auto parent = output_path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent, ec);
        if (ec) {
            if (error) *error = "创建模板目录失败: " + parent.string();
            return false;
        }
        std::ofstream ofs(output_path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            if (error) *error = "写入模板文件失败: " + output_path.string();
            return false;
        }
        const auto content = ApplyEmbeddedTemplateVariables(EmbeddedTemplateText(*entry), root, project_name);
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!ofs.good()) {
            if (error) *error = "写入模板文件失败: " + output_path.string();
            return false;
        }
        files_created.push_back(rel_path);
    }

    const auto config_path = root / "mbink.config.json";
    if (std::filesystem::exists(config_path)) {
        auto config_json = ReadJsonFile(config_path, error);
        if (!config_json || !config_json->is_object()) {
            if (error && error->empty()) *error = "模板 mbink.config.json 无效";
            return false;
        }
        (*config_json)["name"] = project_name;
        (*config_json)["template"] = template_name;
        if (!config_json->contains("window") || !(*config_json)["window"].is_object()) {
            (*config_json)["window"] = nlohmann::json::object();
        }
        (*config_json)["window"]["title"] = project_name;
        if (!WriteJsonFile(config_path, *config_json, error)) return false;
    }

    if (result) {
        result->project_root = root;
        result->project_name = project_name;
        result->template_name = template_name;
        result->files_created = files_created;
        result->next_step = "执行 mbink-ui-dev open \"" + PathToUtf8(root) + "\"";
    }
    return true;
}

bool TryInitProjectFromEmbeddedTemplateLayers(const std::filesystem::path& root,
                                             const std::string& project_name,
                                             const TemplateResolution& resolution,
                                             InitProjectResult* result,
                                             std::string* error) {
    if (error) error->clear();
    std::vector<std::string> files_created;
    std::error_code ec;
    bool copied_any = false;
    for (const auto& layer : resolution.layers) {
        const auto entries = FindEmbeddedTemplateEntries(layer);
        if (entries.empty()) {
            if (error) *error = "template layer missing: " + layer;
            return false;
        }
        const std::string prefix = layer + "/";
        for (const auto* entry : entries) {
            const std::string rel_path = std::string(entry->path).substr(prefix.size());
            const auto output_path = root / std::filesystem::path(rel_path);
            const auto parent = output_path.parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent, ec);
            if (ec) {
                if (error) *error = "failed to create template directory: " + parent.string();
                return false;
            }
            std::ofstream ofs(output_path, std::ios::binary | std::ios::trunc);
            if (!ofs) {
                if (error) *error = "failed to write template file: " + output_path.string();
                return false;
            }
            const auto content = ApplyEmbeddedTemplateVariables(EmbeddedTemplateText(*entry), root, project_name);
            ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
            if (!ofs.good()) {
                if (error) *error = "failed to write template file: " + output_path.string();
                return false;
            }
            copied_any = true;
            if (std::find(files_created.begin(), files_created.end(), rel_path) == files_created.end()) {
                files_created.push_back(rel_path);
            }
        }
    }
    if (!copied_any) return false;

    if (!CopyRuntimeLibrariesForProject(root, resolution.runtime, &files_created, error)) {
        return false;
    }

    const auto config_path = root / "mbink.config.json";
    if (std::filesystem::exists(config_path)) {
        auto config_json = ReadJsonFile(config_path, error);
        if (!config_json || !config_json->is_object()) {
            if (error && error->empty()) *error = "template mbink.config.json is invalid";
            return false;
        }
        (*config_json)["name"] = project_name;
        (*config_json)["template"] = resolution.canonical_key;
        (*config_json)["purpose"] = resolution.purpose;
        (*config_json)["runtime"] = resolution.runtime;
        if (!config_json->contains("window") || !(*config_json)["window"].is_object()) {
            (*config_json)["window"] = nlohmann::json::object();
        }
        (*config_json)["window"]["title"] = project_name;
        if (!WriteJsonFile(config_path, *config_json, error)) return false;
    }

    if (result) {
        result->project_root = root;
        result->project_name = project_name;
        result->template_name = resolution.canonical_key;
        result->purpose = resolution.purpose;
        result->runtime = resolution.runtime;
        result->canonical_key = resolution.canonical_key;
        result->requested_purpose = resolution.requested_purpose;
        result->requested_runtime = resolution.requested_runtime;
        result->legacy_template = resolution.legacy_template;
        result->used_default = resolution.used_default;
        result->layers = resolution.layers;
        result->warnings = resolution.warnings;
        result->files_created = files_created;
        result->next_step = "鎵ц mbink-ui-dev open \"" + PathToUtf8(root) + "\"";
    }
    return true;
}

}  // namespace

std::filesystem::path GetUiDevFilePath(const std::filesystem::path& project_root) {
    return std::filesystem::absolute(project_root).lexically_normal() / ".devui";
}

std::filesystem::path GetProjectsDataRoot() {
    return std::filesystem::temp_directory_path() / "mbink-ui-dev" / "projects";
}

std::filesystem::path GetProjectDataPath(const std::string& project_id) {
    return GetProjectsDataRoot() / SanitizeIdComponent(project_id);
}

std::filesystem::path GetProjectMetadataPath(const std::string& project_id) {
    return GetProjectDataPath(project_id) / "project.json";
}

std::filesystem::path GetStateFilePath(const std::string& project_id) {
    if (project_id.empty()) return std::filesystem::temp_directory_path() / "mbink-ui-dev.state.json";
    return GetProjectDataPath(project_id) / "daemon.state.json";
}

std::filesystem::path GetRuntimeFilePath(const std::string& project_id, const std::string& file_name) {
    return GetProjectDataPath(project_id) / file_name;
}

std::string GetDaemonPipeName(const std::string& project_id) {
    if (project_id.empty()) return "\\\\.\\pipe\\mbink-ui-dev";
    return "\\\\.\\pipe\\mbink-ui-dev-" + SanitizeIdComponent(project_id);
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
        {"purpose", p.purpose},
        {"runtime", p.runtime},
        {"entry", p.entry},
        {"src_dir", p.src_dir},
        {"out_dir", p.out_dir},
        {"build", {{"builder", p.build_builder},
                   {"jsx_factory", p.build_jsx_factory},
                   {"jsx_fragment", p.build_jsx_fragment},
                   {"external", p.build_external},
                   {"sourcemap", p.build_sourcemap},
                   {"minify", p.build_minify},
                   {"hide_console", p.build_hide_console}}},
        {"window", {{"width", p.width},
                    {"height", p.height},
                    {"title", p.title},
                    {"borderless", p.borderless},
                    {"resizable", p.resizable}}},
    };
}

static void FromJson(const nlohmann::json& j, ProjectConfig& p) {
    p.name = j.value("name", p.name);
    p.template_name = j.value("template", p.template_name);
    p.purpose = j.value("purpose", p.purpose);
    p.runtime = j.value("runtime", p.runtime);
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
        p.build_hide_console = b.value("hide_console", p.build_hide_console);
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
        p.borderless = w.value("borderless", p.borderless);
        p.resizable = w.value("resizable", p.resizable);
    }
}

std::optional<ProjectIdentity> LoadProjectIdentity(const std::filesystem::path& project_root, std::string* error) {
    if (error) error->clear();
    const auto ui_dev_path = GetUiDevFilePath(project_root);
    auto json = ReadJsonFile(ui_dev_path, error);
    if (!json.has_value()) return std::nullopt;
    if (!json->is_object()) {
        if (error) *error = ".devui 必须是 JSON object";
        return std::nullopt;
    }
    ProjectIdentity identity;
    FromJson(*json, identity);
    if (identity.project_id.empty()) {
        if (error) *error = ".devui 缺少 project_id";
        return std::nullopt;
    }
    identity.project_root = std::filesystem::absolute(project_root).lexically_normal();
    if (identity.runtime_id.empty()) identity.runtime_id = identity.project_id;
    return identity;
}

bool EnsureProjectIdentity(const std::filesystem::path& project_root,
                           ProjectIdentity* identity,
                           std::string* error) {
    if (error) error->clear();
    if (!identity) {
        if (error) *error = "identity 不能为空";
        return false;
    }
    const auto normalized_root = std::filesystem::absolute(project_root).lexically_normal();
    auto loaded = LoadProjectIdentity(normalized_root, nullptr);
    ProjectIdentity resolved = loaded.value_or(ProjectIdentity{});
    if (resolved.project_id.empty()) resolved.project_id = GenerateOpaqueId("project");
    if (resolved.runtime_id.empty()) resolved.runtime_id = resolved.project_id;
    resolved.project_root = normalized_root;

    nlohmann::json ui_dev_json;
    ToJson(ui_dev_json, resolved);
    if (!WriteJsonFile(GetUiDevFilePath(normalized_root), ui_dev_json, error)) return false;

    nlohmann::json metadata = ui_dev_json;
    metadata["updated_at"] = CurrentTimestampIso8601();
    if (!WriteJsonFile(GetProjectMetadataPath(resolved.project_id), metadata, error)) return false;

    *identity = resolved;
    return true;
}

std::optional<ProjectIdentity> FindProjectIdentityUpwards(const std::filesystem::path& start_path, std::string* error) {
    if (error) error->clear();
    std::error_code ec;
    auto current = std::filesystem::absolute(start_path.empty() ? std::filesystem::current_path() : start_path, ec).lexically_normal();
    if (ec) current = std::filesystem::current_path();
    if (std::filesystem::exists(current, ec) && !std::filesystem::is_directory(current, ec)) current = current.parent_path();
    while (!current.empty()) {
        if (std::filesystem::exists(GetUiDevFilePath(current), ec)) return LoadProjectIdentity(current, error);
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return std::nullopt;
}

std::vector<ProjectIdentity> ListManagedProjects(std::string* error) {
    if (error) error->clear();
    std::vector<ProjectIdentity> projects;
    std::error_code ec;
    const auto root = GetProjectsDataRoot();
    if (!std::filesystem::exists(root, ec)) return projects;
    for (std::filesystem::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_directory(ec)) continue;
        auto json = ReadJsonFile(it->path() / "project.json", nullptr);
        if (!json.has_value() || !json->is_object()) continue;
        ProjectIdentity identity;
        FromJson(*json, identity);
        if (!identity.project_id.empty() && !identity.project_root.empty()) projects.push_back(identity);
    }
    return projects;
}

bool SaveState(const DaemonState& state, std::string* error) {
    try {
        nlohmann::json j;
        j["running"] = state.running;
        j["pid"] = state.pid;
        j["runtime_pid"] = state.runtime_pid;
        j["started_at"] = state.started_at;
        j["project_id"] = state.project_id;
        j["runtime_id"] = state.runtime_id;
        j["project_root"] = state.project_root;
        j["runtime_epoch"] = state.runtime_epoch;
        j["runtime_status"] = state.runtime_status.empty() ? "stopped" : state.runtime_status;
        j["runtime_stop_reason"] = state.runtime_stop_reason.empty() ? "not_started" : state.runtime_stop_reason;
        j["stopping"] = state.stopping;
        ToJson(j["project"], state.project);
        j["last_build"] = state.last_build.is_null()
                              ? nlohmann::json{{"ok", true}, {"status", "not_built"}}
                              : state.last_build;
        j["watch"] = state.watch.is_null()
                         ? nlohmann::json{{"enabled", false}, {"status", "idle"}}
                         : state.watch;

        return WriteJsonFile(GetStateFilePath(state.project_id), j, error);
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::optional<DaemonState> LoadState(const std::string& project_id, std::string* error) {
    try {
        const auto path = GetStateFilePath(project_id);
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
        state.project_id = j.value("project_id", project_id);
        state.runtime_id = j.value("runtime_id", state.project_id);
        state.project_root = j.value("project_root", std::string{});
        state.runtime_epoch = j.value("runtime_epoch", std::string{});
        state.runtime_status = j.value("runtime_status", state.runtime_pid > 0 ? std::string("running") : std::string("stopped"));
        state.runtime_stop_reason = j.value("runtime_stop_reason", std::string("unknown"));
        state.stopping = j.value("stopping", false);
        if (j.contains("project")) FromJson(j.at("project"), state.project);
        state.last_build = j.value("last_build", nlohmann::json{{"ok", true}, {"status", "not_built"}});
        state.watch = j.value("watch", nlohmann::json{{"enabled", false}, {"status", "idle"}});
        return state;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

bool RemoveState(const std::string& project_id, std::string* error) {
    try {
        const auto path = GetStateFilePath(project_id);
        if (!std::filesystem::exists(path)) return true;
        std::filesystem::remove(path);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

ProjectConfig LoadProjectConfig(const std::filesystem::path& project_root, std::string* error) {
    if (error) error->clear();

    ProjectConfig cfg;
    cfg.name = PathToUtf8(project_root.filename());
    cfg.title = cfg.name.empty() ? cfg.title : cfg.name;

    const auto normalize_rel = [](const std::string& value) -> std::string {
        if (value.empty()) return "";
        auto normalized = PathToUtf8(PathFromUtf8(value).lexically_normal());
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    };
    const auto detect_entry = [&project_root]() -> std::string {
        static const std::vector<std::string> candidates = {
            "src/app.js", "src/index.js", "src/main.js",
            "src/index.html", "src/app.html", "src/main.html",
            "app.js", "index.js", "main.js",
            "index.html", "app.html", "main.html",
            "src/app.mjs", "src/index.mjs", "src/main.mjs"
        };
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(project_root / candidate)) return candidate;
        }
        return std::string{"src/app.js"};
    };

    cfg.entry = detect_entry();

    const auto config_path = project_root / "mbink.config.json";
    if (!std::filesystem::exists(config_path)) return cfg;

    try {
        std::ifstream ifs(config_path, std::ios::binary);
        if (!ifs) {
            if (error) *error = "读取 mbink.config.json 失败";
            return cfg;
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        const auto j = nlohmann::json::parse(ss.str());
        if (!j.is_object()) {
            if (error) *error = "mbink.config.json 必须是 JSON object";
            return cfg;
        }

        cfg.name = j.value("name", cfg.name);
        cfg.template_name = j.value("template", cfg.template_name);
        cfg.purpose = j.value("purpose", cfg.purpose);
        cfg.runtime = j.value("runtime", cfg.runtime);
        cfg.entry = normalize_rel(j.value("entry", cfg.entry));
        cfg.src_dir = normalize_rel(j.value("src_dir", cfg.src_dir));
        cfg.out_dir = normalize_rel(j.value("out_dir", cfg.out_dir));
        if (cfg.src_dir.empty()) cfg.src_dir = "src";
        if (cfg.out_dir.empty()) cfg.out_dir = ".dist";
        if (cfg.entry.empty()) {
            if (error) *error = "mbink.config.json 缺少 entry";
            return cfg;
        }
        if (cfg.build_builder != "esbuild") cfg.build_builder = "esbuild";
        if (j.contains("build") && j.at("build").is_object()) {
            const auto& b = j.at("build");
            cfg.build_builder = b.value("builder", cfg.build_builder);
            cfg.build_jsx_factory = b.value("jsx_factory", cfg.build_jsx_factory);
            cfg.build_jsx_fragment = b.value("jsx_fragment", cfg.build_jsx_fragment);
            cfg.build_sourcemap = b.value("sourcemap", cfg.build_sourcemap);
            cfg.build_minify = b.value("minify", cfg.build_minify);
            cfg.build_hide_console = b.value("hide_console", cfg.build_hide_console);
            if (b.contains("external") && b.at("external").is_array()) {
                cfg.build_external.clear();
                for (const auto& item : b.at("external")) {
                    if (item.is_string()) cfg.build_external.push_back(item.get<std::string>());
                }
            }
        }
        if (j.contains("window") && j.at("window").is_object()) {
            const auto& w = j.at("window");
            cfg.title = w.value("title", cfg.title);
            cfg.width = w.value("width", cfg.width);
            cfg.height = w.value("height", cfg.height);
            cfg.borderless = w.value("borderless", cfg.borderless);
            cfg.resizable = w.value("resizable", cfg.resizable);
        }
        if (cfg.name.empty()) cfg.name = PathToUtf8(project_root.filename());
        if (cfg.title.empty()) cfg.title = cfg.name.empty() ? "MBink UI Dev" : cfg.name;
        if (cfg.build_builder.empty()) cfg.build_builder = "esbuild";
        if (cfg.build_builder != "esbuild") {
            if (error) *error = "当前仅支持 build.builder=esbuild";
            return cfg;
        }
        if (cfg.width <= 0 || cfg.height <= 0) {
            if (error) *error = "window.width/window.height 必须大于 0";
            return cfg;
        }
        const auto entry_path = std::filesystem::absolute(project_root / PathFromUtf8(cfg.entry)).lexically_normal();
        if (!std::filesystem::exists(entry_path)) {
            if (error) *error = "entry 文件不存在: " + cfg.entry;
            return cfg;
        }
        return cfg;
    } catch (const std::exception& e) {
        if (error) *error = std::string("解析 mbink.config.json 失败: ") + e.what();
        return cfg;
    }
}

bool InitProject(const std::filesystem::path& target_dir,
                 const std::string& template_name,
                 InitProjectResult* result,
                 std::string* error) {
    InitProjectOptions options;
    options.legacy_template = template_name;
    return InitProject(target_dir, options, result, error);
}

bool InitProject(const std::filesystem::path& target_dir,
                 const InitProjectOptions& options,
                 InitProjectResult* result,
                 std::string* error) {
    if (error) error->clear();
    if (result) *result = InitProjectResult{};

    TemplateResolution resolution;
    if (!ResolveInitTemplate(options, &resolution, error)) {
        if (error && error->empty()) *error = "invalid template selection";
        return false;
    }

    const auto root = std::filesystem::absolute(target_dir).lexically_normal();
    std::error_code ec;
    if (std::filesystem::exists(root, ec)) {
        if (!std::filesystem::is_directory(root, ec)) {
            if (error) *error = "target path exists and is not a directory";
            return false;
        }
        if (std::filesystem::directory_iterator(root, ec) != std::filesystem::directory_iterator()) {
            if (error) *error = "target directory is not empty; use an empty or non-existing directory";
            return false;
        }
    } else {
        std::filesystem::create_directories(root, ec);
        if (ec) {
            if (error) *error = "failed to create target directory";
            return false;
        }
    }

    const std::string root_name = PathToUtf8(root.filename());
    const std::string project_name = root_name.empty() ? "mbink-app" : root_name;
    if (!TryInitProjectFromEmbeddedTemplateLayers(root, project_name, resolution, result, error)) {
        if (error && error->empty()) *error = "failed to initialize project from template layers";
        return false;
    }
    return true;
}

std::vector<std::string> ListSupportedInitTemplates() {
    return {"preact-jsx", "preact-ts", "vanilla-js", "python", "go", "rust", "python-host", "rust-host"};
}

std::vector<std::string> ListSupportedInitCombinations() {
    return SupportedCanonicalKeys();
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
    j["project_id"] = state.project_id;
    j["runtime_id"] = state.runtime_id;
    j["project_root"] = state.project_root;
    j["runtime_epoch"] = state.runtime_epoch;
    j["runtime_status"] = state.runtime_status.empty() ? "stopped" : state.runtime_status;
    j["runtime_stop_reason"] = state.runtime_stop_reason.empty() ? "not_started" : state.runtime_stop_reason;
    j["stopping"] = state.stopping;
    ToJson(j["project"], state.project);
    j["last_build"] = state.last_build.is_null()
                          ? nlohmann::json{{"ok", true}, {"status", "not_built"}}
                          : state.last_build;
    j["watch"] = state.watch.is_null()
                     ? nlohmann::json{{"enabled", false}, {"status", "idle"}}
                     : state.watch;
    return j;
}

nlohmann::json ProjectToJson(const std::filesystem::path& root, const ProjectConfig& config) {
    nlohmann::json j;
    j["name"] = config.name;
    j["root"] = PathToUtf8(root);
    ToJson(j["config"], config);
    j["entry"] = config.entry;
    j["src_dir"] = config.src_dir;
    j["out_dir"] = config.out_dir;
    j["template"] = config.template_name;
    j["purpose"] = config.purpose;
    j["runtime"] = config.runtime;
    j["window"] = {{"width", config.width},
                   {"height", config.height},
                   {"title", config.title},
                   {"borderless", config.borderless},
                   {"resizable", config.resizable}};
    return j;
}

}  // namespace mbink::ui_dev
