#include "ui_dev_runtime_support.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

#include <nlohmann/json.hpp>

#include "core/dom/document.h"
#include "core/event/loop/event_loop.h"
#include "core/quickjs/quickjs_runtime.h"
#include "core/utils/encoding_utils.h"
#include "core/window/window.h"
#include "ui_dev_control.h"
#include "ui_dev_snapshot.h"

namespace mbink::ui_dev {
namespace {
namespace fs = std::filesystem;

std::string CurrentTimestampIso8601();

fs::path BufferFilePathFromUtf8(const std::string& path) {
#ifdef _WIN32
    return fs::path(utils::UTF8ToWide(path));
#else
    return fs::path(path);
#endif
}

bool WriteJsonFile(const std::string& path, const nlohmann::json& value) {
    if (path.empty()) return false;
    std::ofstream ofs(BufferFilePathFromUtf8(path), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << value.dump(2);
    return ofs.good();
}

void WriteLifecycleState(const std::string& path,
                         const std::string& status,
                         const std::string& reason,
                         const std::string& runtime_epoch) {
    if (path.empty()) return;
    WriteJsonFile(path,
                  nlohmann::json{{"ok", true},
                                 {"status", status},
                                 {"reason", reason},
                                 {"runtime_epoch", runtime_epoch},
                                 {"timestamp", CurrentTimestampIso8601()}});
}

void ExportPendingSnapshot(const std::shared_ptr<Window>& window,
                           const std::shared_ptr<Document>& document,
                           const std::string& snapshot_file,
                           size_t snapshot_max_nodes,
                           int snapshot_max_depth,
                           const std::string& snapshot_root_selector,
                           const std::shared_ptr<std::string>& runtime_epoch,
                           const std::shared_ptr<std::atomic<bool>>& shutdown_requested,
                           const std::shared_ptr<bool>& snapshot_pending) {
    const bool needs_snapshot = !snapshot_file.empty() && *snapshot_pending;
    if (!needs_snapshot) return;
    if (shutdown_requested && shutdown_requested->load()) return;

    std::string err;
    SnapshotExportOptions snapshot_options;
    snapshot_options.runtime_epoch = runtime_epoch ? *runtime_epoch : std::string{};
    snapshot_options.max_nodes = snapshot_max_nodes;
    snapshot_options.max_depth = snapshot_max_depth;
    snapshot_options.root_selector = snapshot_root_selector;
    snapshot_options.shutdown_requested = shutdown_requested;
    if (ExportUiDevSnapshot(window, document, snapshot_file, snapshot_options, &err)) {
        *snapshot_pending = false;
    }
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

class StructuredJsonBuffer {
public:
    StructuredJsonBuffer(std::string field_name, size_t max_entries, std::string file_path)
        : field_name_(std::move(field_name)), max_entries_(max_entries), file_path_(std::move(file_path)) {}

    void Push(nlohmann::json entry) {
        if (file_path_.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        entry["timestamp"] = CurrentTimestampIso8601();
        entry["line"] = ++next_line_;
        entries_.push_back(std::move(entry));
        while (entries_.size() > max_entries_) entries_.pop_front();
        FlushLocked();
    }

private:
    void FlushLocked() const {
        nlohmann::json payload;
        payload[field_name_] = nlohmann::json::array();
        for (const auto& entry : entries_) payload[field_name_].push_back(entry);
        std::ofstream ofs(BufferFilePathFromUtf8(file_path_), std::ios::binary | std::ios::trunc);
        if (ofs) ofs << payload.dump(2);
    }

    std::string field_name_;
    size_t max_entries_ = 0;
    std::string file_path_;
    mutable std::mutex mutex_;
    std::deque<nlohmann::json> entries_;
    int next_line_ = 0;
};

}  // namespace

void AttachStructuredRuntimeBuffers(QuickJSRuntime* runtime,
                                    const RuntimeSupportOptions& options) {
    if (!runtime) return;
    auto console_buffer = std::make_shared<StructuredJsonBuffer>("entries", 500, options.console_file);
    auto error_buffer = std::make_shared<StructuredJsonBuffer>("errors", 200, options.errors_file);
    runtime->SetConsoleCallback([console_buffer](const nlohmann::json& entry) {
        console_buffer->Push(entry);
    });
    runtime->SetErrorCallback([error_buffer](const nlohmann::json& entry) {
        error_buffer->Push(entry);
    });
}

void ConfigureRuntimeControl(EventLoop* event_loop,
                             QuickJSRuntime* runtime,
                             const std::shared_ptr<Window>& window,
                             const std::shared_ptr<Document>& document,
                             const RuntimeSupportOptions& options) {
    if (!event_loop || !runtime || !window || !document) return;
    auto snapshot_pending = std::make_shared<bool>(!options.snapshot_file.empty());
    auto lifecycle_file = std::make_shared<std::string>(options.lifecycle_file);
    auto runtime_epoch = std::make_shared<std::string>(options.runtime_epoch);
    auto shutdown_requested = std::make_shared<std::atomic<bool>>(false);
    auto stopped_reason = std::make_shared<std::string>();
    auto quit_elapsed = std::make_shared<float>(0.0f);
    auto quit_frames = std::make_shared<int>(0);

    WriteLifecycleState(*lifecycle_file, "running", "started", *runtime_epoch);
    if (*snapshot_pending) window->SetNeedsRepaint();
    window->SetOnCloseCallback([event_loop, lifecycle_file, stopped_reason, runtime_epoch, shutdown_requested]() {
        shutdown_requested->store(true);
        if (stopped_reason->empty()) {
            *stopped_reason = "user_closed";
            WriteLifecycleState(*lifecycle_file, "stopped", *stopped_reason, *runtime_epoch);
        }
        event_loop->Stop();
    });

    event_loop->SetRenderCallback([window,
                                   document,
                                   snapshot_file = options.snapshot_file,
                                   snapshot_max_nodes = options.snapshot_max_nodes,
                                   snapshot_max_depth = options.snapshot_max_depth,
                                   snapshot_root_selector = options.snapshot_root_selector,
                                   runtime_epoch,
                                   shutdown_requested,
                                   snapshot_pending]() {
        ExportPendingSnapshot(window,
                              document,
                              snapshot_file,
                              snapshot_max_nodes,
                              snapshot_max_depth,
                              snapshot_root_selector,
                              runtime_epoch,
                              shutdown_requested,
                              snapshot_pending);
    });

    auto last_command_id = std::make_shared<std::string>();
    event_loop->SetUpdateCallback([runtime,
                                   window,
                                   document,
                                   command_file = options.command_file,
                                   response_file = options.response_file,
                                   snapshot_file = options.snapshot_file,
                                   snapshot_max_nodes = options.snapshot_max_nodes,
                                   snapshot_max_depth = options.snapshot_max_depth,
                                   snapshot_root_selector = options.snapshot_root_selector,
                                   runtime_epoch,
                                   shutdown_requested,
                                   last_command_id,
                                   snapshot_pending,
                                   event_loop,
                                   lifecycle_file,
                                   stopped_reason,
                                   quit_elapsed,
                                   quit_frames,
                                   quit_after_seconds = options.quit_after_seconds](float delta_time) {
        if (shutdown_requested->load()) return;
        ExportPendingSnapshot(window,
                              document,
                              snapshot_file,
                              snapshot_max_nodes,
                              snapshot_max_depth,
                              snapshot_root_selector,
                              runtime_epoch,
                              shutdown_requested,
                              snapshot_pending);
        if (!command_file.empty() && !response_file.empty()) {
            bool handled = false;
            std::string err;
            if (TryHandleUiDevCommand(runtime,
                                      window.get(),
                                      document.get(),
                                      command_file,
                                      response_file,
                                      last_command_id.get(),
                                      runtime_epoch.get(),
                                      shutdown_requested,
                                      &handled,
                                      &err) && handled) {
                *snapshot_pending = !snapshot_file.empty();
                if (*snapshot_pending) window->SetNeedsRepaint();
            }
        }

        if (quit_after_seconds <= 0) return;
        ++(*quit_frames);
        *quit_elapsed += delta_time;
        if (*quit_elapsed >= quit_after_seconds) {
            shutdown_requested->store(true);
            std::cout << "[Auto-quit] Completed " << *quit_elapsed << " seconds (" << *quit_frames
                      << " frames), exiting..." << std::endl;
            if (stopped_reason->empty()) {
                *stopped_reason = "auto_quit";
                WriteLifecycleState(*lifecycle_file, "stopped", *stopped_reason, *runtime_epoch);
            }
            event_loop->Stop();
        }
    });
}

}  // namespace mbink::ui_dev
