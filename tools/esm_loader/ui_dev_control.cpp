#include "ui_dev_control.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/dom/document.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/render/pipeline/render_pipeline.h"
#include "core/utils/encoding_utils.h"
#include "core/window/window.h"

namespace mbink::ui_dev {
namespace {
namespace fs = std::filesystem;

fs::path Utf8PathToFsPathLocal(const std::string& path) {
#ifdef _WIN32
    return fs::path(utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

std::string NormalizeFsPathLocal(const fs::path& path) {
    return path.lexically_normal().generic_string();
}

void ClearBody(Document* document) {
    if (!document) return;
    auto body = document->GetBody();
    if (!body) return;
    while (body->GetFirstChild()) {
        body->RemoveChild(body->GetFirstChild());
    }
}

std::string BuildReloadFilename(const std::string& bundle_path) {
    const auto tick = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return bundle_path + "#reload-" + std::to_string(tick);
}

}  // namespace

bool TryHandleUiDevCommand(QuickJSRuntime* runtime,
                           Window* window,
                           Document* document,
                           const std::string& command_path,
                           const std::string& response_path,
                           std::string* last_command_id,
                           bool* handled,
                           std::string* error) {
    if (handled) *handled = false;
    if (!runtime || !window || !document || command_path.empty() || response_path.empty() || !last_command_id) return false;

    fs::path cmdp(command_path);
    if (!fs::exists(cmdp)) return false;

    nlohmann::json cmd;
    try {
        std::ifstream ifs(cmdp, std::ios::binary);
        if (!ifs) return false;
        cmd = nlohmann::json::parse(ifs);
    } catch (...) {
        std::error_code ec;
        fs::remove(cmdp, ec);
        if (error) *error = "ui-dev command 解析失败";
        return false;
    }

    const std::string id = cmd.value("id", "");
    if (id.empty() || id == *last_command_id) return false;

    nlohmann::json resp;
    resp["ok"] = true;
    resp["id"] = id;
    resp["type"] = cmd.value("type", "");

    try {
        const std::string type = cmd.value("type", "");
        if (type == "eval") {
            const std::string code = cmd.value("code", "");
            resp["result"] = runtime->Eval(code, "<mbink-ui-dev eval>");
        } else if (type == "reload_bundle") {
            const std::string bundle_path = cmd.value("bundle_path", "");
            if (bundle_path.empty()) {
                throw std::runtime_error("reload_bundle 缺少 bundle_path");
            }
            fs::path abs_path = fs::absolute(Utf8PathToFsPathLocal(bundle_path));
            std::ifstream ifs(abs_path, std::ios::binary);
            if (!ifs) {
                throw std::runtime_error("无法打开 bundle: " + NormalizeFsPathLocal(abs_path));
            }
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            const std::string bundle_code = buffer.str();
            const std::string normalized_path = NormalizeFsPathLocal(abs_path);
            ClearBody(document);
            runtime->SetBaseModulePath(normalized_path);
            resp["result"] = runtime->EvalModule(bundle_code, BuildReloadFilename(normalized_path));
            if (auto* pipeline = window->GetRenderPipeline()) {
                pipeline->ForceFullUpdate();
                pipeline->ForceRasterize();
            }
            window->InvalidateRenderTree();
            window->SetNeedsRepaint();
            window->Render();
            window->SwapBuffers();
            resp["bundle_path"] = normalized_path;
            resp["mode"] = "reload_bundle";
        } else {
            resp["ok"] = false;
            resp["error"] = nlohmann::json{{"code", "unknown_command"}, {"message", "未知 ui-dev 命令"}};
        }
    } catch (const std::exception& e) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", resp.value("type", "") == "reload_bundle" ? "reload_bundle_failed" : "eval_failed"},
                                         {"message", e.what()}};
    } catch (...) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", resp.value("type", "") == "reload_bundle" ? "reload_bundle_failed" : "eval_failed"},
                                         {"message", "unknown exception"}};
    }

    try {
        std::ofstream ofs(fs::path(response_path), std::ios::binary | std::ios::trunc);
        ofs << resp.dump(2);
    } catch (...) {
        if (error) *error = "写入 ui-dev response 失败";
        return false;
    }

    {
        std::error_code ec;
        fs::remove(cmdp, ec);
    }

    *last_command_id = id;
    if (handled) *handled = true;
    return true;
}

}  // namespace mbink::ui_dev
