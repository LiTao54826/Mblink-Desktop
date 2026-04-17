#include "common.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace mbink::ui_dev {

namespace {

std::string NormalizePathString(const std::filesystem::path& path) {
    return std::filesystem::absolute(path).lexically_normal().generic_string();
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
    identity.project_root = j.value("project_root", std::string{});
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
        j["runtime_status"] = state.runtime_status.empty() ? "stopped" : state.runtime_status;
        j["runtime_stop_reason"] = state.runtime_stop_reason.empty() ? "not_started" : state.runtime_stop_reason;
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
        state.runtime_status = j.value("runtime_status", state.runtime_pid > 0 ? std::string("running") : std::string("stopped"));
        state.runtime_stop_reason = j.value("runtime_stop_reason", std::string("unknown"));
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
    cfg.name = project_root.filename().string();
    cfg.title = cfg.name.empty() ? cfg.title : cfg.name;

    const auto normalize_rel = [](const std::string& value) -> std::string {
        if (value.empty()) return "";
        return std::filesystem::path(value).lexically_normal().generic_string();
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
        }
        if (cfg.name.empty()) cfg.name = project_root.filename().string();
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
        const auto entry_path = std::filesystem::absolute(project_root / cfg.entry).lexically_normal();
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
    if (error) error->clear();
    if (result) *result = InitProjectResult{};

    const auto supported = ListSupportedInitTemplates();
    if (std::find(supported.begin(), supported.end(), template_name) == supported.end()) {
        if (error) *error = "template 非法，请使用受支持的模板类型";
        return false;
    }

    const auto root = std::filesystem::absolute(target_dir).lexically_normal();
    std::error_code ec;
    if (std::filesystem::exists(root, ec)) {
        if (!std::filesystem::is_directory(root, ec)) {
            if (error) *error = "目标路径已存在且不是目录";
            return false;
        }
        if (std::filesystem::directory_iterator(root, ec) != std::filesystem::directory_iterator()) {
            if (error) *error = "目标目录非空，请使用空目录或不存在的目录";
            return false;
        }
    } else {
        std::filesystem::create_directories(root, ec);
        if (ec) {
            if (error) *error = "创建目标目录失败";
            return false;
        }
    }

    const std::string project_name = root.filename().string().empty() ? "mbink-app" : root.filename().string();
    const bool is_preact_jsx = template_name == "preact-jsx";
    const bool is_preact_ts = template_name == "preact-ts";
    const bool is_vanilla_js = template_name == "vanilla-js";
    const bool is_python_host = template_name == "python-host";
    const bool is_rust_host = template_name == "rust-host";

    const std::string app_js = is_preact_jsx
        ? std::string(
R"JS(const { h, render } = Preact;

const styles = {
  app: { fontFamily: 'Segoe UI, sans-serif', padding: '24px', background: '#0f172a', color: '#e2e8f0', minHeight: '100vh' },
  card: { background: '#111827', borderRadius: '12px', padding: '20px', border: '1px solid #334155' },
  title: { fontSize: '28px', fontWeight: '700', marginBottom: '8px' },
  hint: { color: '#94a3b8' }
};

function App() {
  return h('div', { style: styles.app },
    h('div', { style: styles.card },
      h('div', { style: styles.title }, 'MBink Preact JSX Starter'),
      h('div', { style: styles.hint }, 'open 可直接运行；build 会将 src/App.jsx 编译到 .dist/App.js')
    )
  );
}

render(h(App), document.body);
)JS")
        : is_preact_ts
        ? std::string(
R"JS(import './App.tsx';
)JS")
        : is_vanilla_js
        ? std::string(
R"JS(const { h, render } = Preact;

function App() {
  return h('div', {
    style: {
      fontFamily: 'Segoe UI, sans-serif',
      minHeight: '100vh',
      padding: '24px',
      background: '#f8fafc',
      color: '#0f172a'
    }
  },
    h('h1', null, 'MBink Vanilla JS Starter'),
    h('p', null, '这个模板不依赖 JSX，open/build/snapshot 可直接闭环。'),
    h('button', {
      onClick: () => console.log('hello from vanilla-js'),
      style: { padding: '10px 14px', borderRadius: '8px', border: '1px solid #cbd5e1', cursor: 'pointer' }
    }, 'Click me')
  );
}

render(h(App), document.body);
)JS")
        : is_python_host
        ? std::string(
R"JS(const { h, render } = Preact;

function App() {
  return h('div', {
    style: {
      fontFamily: 'Segoe UI, sans-serif',
      minHeight: '100vh',
      padding: '24px',
      background: '#111827',
      color: '#e5e7eb'
    }
  },
    h('h1', null, 'MBink Python Host Starter'),
    h('p', null, 'Python 侧逻辑示例在 host/main.py；前端入口仍然可直接 open/build。'),
    h('button', {
      onClick: () => console.log('hello from python-host template'),
      style: { padding: '10px 14px', borderRadius: '8px', border: '1px solid #374151', cursor: 'pointer' }
    }, 'Ping Python Host Template')
  );
}

render(h(App), document.body);
)JS")
        : std::string(
R"JS(const { h, render } = Preact;

function App() {
  return h('div', {
    style: {
      fontFamily: 'Segoe UI, sans-serif',
      minHeight: '100vh',
      padding: '24px',
      background: '#0f172a',
      color: '#e2e8f0'
    }
  },
    h('h1', null, 'MBink Rust Host Starter'),
    h('p', null, 'Rust 侧逻辑示例在 rust_host/src/main.rs；前端入口仍然可直接 open/build。'),
    h('button', {
      onClick: () => console.log('hello from rust-host template'),
      style: { padding: '10px 14px', borderRadius: '8px', border: '1px solid #475569', cursor: 'pointer' }
    }, 'Ping Rust Host Template')
  );
}

render(h(App), document.body);
)JS");
    const std::string app_jsx = std::string(
R"JSX(const { h, render } = Preact;
const { useState } = PreactHooks;

function App() {
  const [count, setCount] = useState(0);
  return (
    <div style={{ fontFamily: 'Segoe UI, sans-serif', minHeight: '100vh', padding: 24, background: '#020617', color: '#e2e8f0' }}>
      <h1>MBink Preact JSX Starter</h1>
      <p>这是给 esbuild 的 JSX 入口，构建产物输出到 .dist/App.js。</p>
      <button onClick={() => setCount(count + 1)} style={{ padding: '10px 14px', borderRadius: 8, border: '1px solid #475569', cursor: 'pointer' }}>
        Count: {count}
      </button>
    </div>
  );
}

render(<App />, document.body);
)JSX");
    const std::string app_tsx = std::string(
R"TSX(const { h, render } = Preact;
const { useMemo, useState } = PreactHooks;

function Card(props) {
  return (
    <div style={{ border: '1px solid #334155', borderRadius: 12, padding: 16, background: '#111827' }}>
      <div style={{ fontSize: 24, fontWeight: 700 }}>{props.title}</div>
      <div style={{ color: '#94a3b8', marginTop: 8 }}>Current count: {props.count}</div>
    </div>
  );
}

function App() {
  const [count, setCount] = useState(0);
  const title = useMemo(() => 'MBink Preact TS Starter', []);

  return (
    <div style={{ fontFamily: 'Segoe UI, sans-serif', minHeight: '100vh', padding: 24, background: '#020617', color: '#e2e8f0' }}>
      <Card title={title} count={count} />
      <button onClick={() => setCount(count + 1)} style={{ marginTop: 16, padding: '10px 14px', borderRadius: 8, border: '1px solid #475569', cursor: 'pointer' }}>
        Count: {count}
      </button>
    </div>
  );
}

render(<App />, document.body);
)TSX");
    const std::string python_main = std::string(
R"PY(def main():
    print("MBink python-host template")
    print("在这里接入 bindings/python 或你的业务逻辑。")


if __name__ == "__main__":
    main()
)PY");
    const std::string python_requirements = std::string(
R"REQ(# 可在此处声明你的 Python 依赖
# mbink-python-binding
)REQ");
    const std::string rust_cargo_toml = std::string(
R"TOML([package]
name = "mbink-rust-host"
version = "0.1.0"
edition = "2021"

[dependencies]
)TOML");
    const std::string rust_main = std::string(
R"RS(fn main() {
    println!("MBink rust-host template");
    println!("在这里接入 Rust bindings 或宿主逻辑。");
}
)RS");
    const std::string config = std::string("{\n") +
        "  \"name\": \"" + project_name + "\",\n" +
        "  \"template\": \"" + template_name + "\",\n" +
        "  \"entry\": \"src/app.js\",\n" +
        "  \"src_dir\": \"src\",\n" +
        "  \"out_dir\": \".dist\",\n" +
        "  \"window\": {\n" +
        "    \"title\": \"" + project_name + "\",\n" +
        "    \"width\": 800,\n" +
        "    \"height\": 600\n" +
        "  },\n" +
        "  \"build\": {\n" +
        "    \"builder\": \"esbuild\",\n" +
        "    \"jsx_factory\": \"Preact.h\",\n" +
        "    \"jsx_fragment\": \"Preact.Fragment\",\n" +
        "    \"external\": [\"preact\", \"preact/hooks\"],\n" +
        "    \"sourcemap\": true,\n" +
        "    \"minify\": false\n" +
        "  }\n" +
        "}\n";

    if (!std::filesystem::exists(root / "src", ec)) {
        std::filesystem::create_directories(root / "src", ec);
        if (ec) {
            if (error) *error = "创建 src 目录失败";
            return false;
        }
    }

    std::vector<std::pair<std::filesystem::path, std::string>> files = {
        {root / "mbink.config.json", config},
        {root / "src" / "app.js", app_js},
    };
    if (is_python_host) {
        files.push_back({root / "host" / "main.py", python_main});
        files.push_back({root / "requirements.txt", python_requirements});
    }
    if (is_rust_host) {
        files.push_back({root / "rust_host" / "Cargo.toml", rust_cargo_toml});
        files.push_back({root / "rust_host" / "src" / "main.rs", rust_main});
    }
    for (const auto& [path, content] : files) {
        const auto parent = path.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                if (error) *error = "创建模板目录失败: " + parent.string();
                return false;
            }
        }
        std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            if (error) *error = "写入模板文件失败: " + path.string();
            return false;
        }
        ofs << content;
        if (!ofs.good()) {
            if (error) *error = "写入模板文件失败: " + path.string();
            return false;
        }
    }
    if (is_preact_jsx || is_preact_ts) {
        const auto component_path = is_preact_jsx ? (root / "src" / "App.jsx") : (root / "src" / "App.tsx");
        std::ofstream ofs(component_path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            if (error) *error = "写入模板文件失败: " + component_path.lexically_relative(root).generic_string();
            return false;
        }
        ofs << (is_preact_jsx ? app_jsx : app_tsx);
        if (!ofs.good()) {
            if (error) *error = "写入模板文件失败: " + component_path.lexically_relative(root).generic_string();
            return false;
        }
    }

    if (result) {
        result->project_root = root;
        result->project_name = project_name;
        result->template_name = template_name;
        result->files_created = {"mbink.config.json", "src/app.js"};
        if (is_preact_jsx) result->files_created.push_back("src/App.jsx");
        if (is_preact_ts) result->files_created.push_back("src/App.tsx");
        if (is_python_host) {
            result->files_created.push_back("host/main.py");
            result->files_created.push_back("requirements.txt");
        }
        if (is_rust_host) {
            result->files_created.push_back("rust_host/Cargo.toml");
            result->files_created.push_back("rust_host/src/main.rs");
        }
        result->next_step = "执行 mbink-ui-dev open \"" + root.string() + "\"";
    }
    return true;
}

std::vector<std::string> ListSupportedInitTemplates() {
    return {"preact-jsx", "preact-ts", "vanilla-js", "python-host", "rust-host"};
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
    j["runtime_status"] = state.runtime_status.empty() ? "stopped" : state.runtime_status;
    j["runtime_stop_reason"] = state.runtime_stop_reason.empty() ? "not_started" : state.runtime_stop_reason;
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
