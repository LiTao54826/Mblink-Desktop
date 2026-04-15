#include "ui_dev_control.h"

#include <fstream>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "core/quickjs/quickjs_runtime.h"

namespace mbink::ui_dev {

bool TryHandleUiDevCommand(QuickJSRuntime* runtime,
                           const std::string& command_path,
                           const std::string& response_path,
                           std::string* last_command_id,
                           bool* handled,
                           std::string* error) {
    if (handled) *handled = false;
    if (!runtime || command_path.empty() || response_path.empty() || !last_command_id) return false;

    std::filesystem::path cmdp(command_path);
    if (!std::filesystem::exists(cmdp)) return false;

    nlohmann::json cmd;
    try {
        std::ifstream ifs(cmdp, std::ios::binary);
        if (!ifs) return false;
        cmd = nlohmann::json::parse(ifs);
    } catch (...) {
        // 读/解析失败：避免循环报错，尝试移走
        std::error_code ec;
        std::filesystem::remove(cmdp, ec);
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
            auto result = runtime->Eval(code, "<mbink-ui-dev eval>");
            resp["result"] = result;
        } else {
            resp["ok"] = false;
            resp["error"] = nlohmann::json{{"code", "unknown_command"}, {"message", "未知 ui-dev 命令"}};
        }
    } catch (const std::exception& e) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", "eval_failed"}, {"message", e.what()}};
    } catch (...) {
        resp["ok"] = false;
        resp["error"] = nlohmann::json{{"code", "eval_failed"}, {"message", "unknown exception"}};
    }

    try {
        std::ofstream ofs(std::filesystem::path(response_path), std::ios::binary | std::ios::trunc);
        ofs << resp.dump(2);
    } catch (...) {
        if (error) *error = "写入 ui-dev response 失败";
        return false;
    }

    // best-effort：删除命令文件，避免重复处理
    {
        std::error_code ec;
        std::filesystem::remove(cmdp, ec);
    }

    *last_command_id = id;
    if (handled) *handled = true;
    return true;
}

}  // namespace mbink::ui_dev
