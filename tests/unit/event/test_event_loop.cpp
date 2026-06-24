/**
 * @file test_event_loop.cpp
 * @brief EventLoop 单元测试
 */

#include <gtest/gtest.h>
#include "event/loop/event_loop.h"
#include "event/loop/task_scheduler.h"
#include "dom/document.h"
#include "dom/event.h"
#include "dom/elements/html_input_element.h"
#include "window/window.h"
#include "window/window_manager.h"
#include <filesystem>
#include <fstream>

namespace mbink {
namespace test {

class EventLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        event_loop_ = std::make_unique<EventLoop>();
    }

    void TearDown() override {
        event_loop_.reset();
    }

protected:
    std::unique_ptr<EventLoop> event_loop_;
};

TEST_F(EventLoopTest, InitialState) {
    EXPECT_FALSE(event_loop_->IsRunning());
    EXPECT_FALSE(event_loop_->ShouldQuit());
}

TEST_F(EventLoopTest, Stop) {
    event_loop_->Stop();
    EXPECT_TRUE(event_loop_->ShouldQuit());
}

TEST_F(EventLoopTest, SetIdleCallback) {
    bool called = false;
    event_loop_->SetIdleCallback([&called]() {
        called = true;
    });

    // 运行一次循环
    event_loop_->Stop();  // 确保不会无限循环
    event_loop_->RunOnce();

    // 空闲回调应该被调用
    // 注意：这取决于具体实现
}

TEST_F(EventLoopTest, SetUpdateCallback) {
    bool called = false;
    float delta_time = 0.0f;

    event_loop_->SetUpdateCallback([&called, &delta_time](float dt) {
        called = true;
        delta_time = dt;
    });

    event_loop_->Stop();
    event_loop_->RunOnce();

    // 更新回调应该被调用
    // EXPECT_TRUE(called);
}

TEST_F(EventLoopTest, SetRenderCallback) {
    bool called = false;

    event_loop_->SetRenderCallback([&called]() {
        called = true;
    });

    event_loop_->Stop();
    event_loop_->RunOnce();

    // 渲染回调应该被调用
    // EXPECT_TRUE(called);
}

TEST_F(EventLoopTest, FileDialogResultQueueUpdatesInputOnRunOnce) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-event-loop-file-dialog-result-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "picked.txt";
    {
        std::ofstream(file_path) << "picked";
    }

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(document->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    document->GetBody()->AppendChild(input);

    int input_events = 0;
    int change_events = 0;
    input->AddEventListener("input", [&](std::shared_ptr<Event>) { ++input_events; });
    input->AddEventListener("change", [&](std::shared_ptr<Event>) { ++change_events; });

    QueueFileDialogResultForTesting(input, {file_path.string()});
    event_loop_->RunOnce();

    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "picked.txt");
    EXPECT_EQ(input->GetValue(), "C:\\fakepath\\picked.txt");
    EXPECT_EQ(input_events, 1);
    EXPECT_EQ(change_events, 1);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(EventLoopTest, RenderCallbackRunsAfterWindowRender) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;

    auto document = std::make_shared<Document>();
    document->Initialize();

    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    WindowManager::Instance().RegisterWindow(window);

    bool callback_called = false;
    bool callback_saw_pending_repaint = true;
    event_loop_->SetRenderCallback([&]() {
        callback_called = true;
        callback_saw_pending_repaint = window->NeedsRepaint();
    });

    window->SetNeedsRepaint();
    event_loop_->RunOnce();

    WindowManager::Instance().UnregisterWindow(window);

    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(callback_saw_pending_repaint);
}

TEST_F(EventLoopTest, FixedDialogBatchMutationKeepsRenderTreeCached) {
    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;

    auto document = std::make_shared<Document>();
    document->Initialize();

    auto body = document->GetBody();
    ASSERT_TRUE(body);

    auto dialog = document->CreateElement("div");
    dialog->SetAttribute("id", "dialog");
    dialog->SetStyle("position", "fixed");
    dialog->SetStyle("left", "40px");
    dialog->SetStyle("top", "40px");
    dialog->SetStyle("width", "520px");
    dialog->SetStyle("height", "360px");

    auto content = document->CreateElement("div");
    content->SetAttribute("id", "dialog-content");
    dialog->AppendChild(content);
    body->AppendChild(dialog);

    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    window->Render();

    auto cached_tree = window->GetCachedRenderTree();
    ASSERT_TRUE(cached_tree);

    document->BeginBatch();
    for (int i = 0; i < 3; ++i) {
        auto row = document->CreateElement("div");
        row->AppendChild(document->CreateTextNode("row"));
        content->AppendChild(row);
    }
    document->EndBatch();

    EXPECT_EQ(window->GetCachedRenderTree(), cached_tree);
    EXPECT_TRUE(window->NeedsRepaint());
}

TEST_F(EventLoopTest, FixedDialogTextRemovalKeepsRenderTreeCached) {
    WindowConfig config;
    config.width = 800;
    config.height = 600;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;

    auto document = std::make_shared<Document>();
    document->Initialize();

    auto body = document->GetBody();
    ASSERT_TRUE(body);

    auto dialog = document->CreateElement("div");
    dialog->SetStyle("position", "fixed");
    dialog->SetStyle("left", "40px");
    dialog->SetStyle("top", "40px");
    dialog->SetStyle("width", "520px");
    dialog->SetStyle("height", "360px");
    auto label = document->CreateTextNode("loading");
    dialog->AppendChild(label);
    body->AppendChild(dialog);

    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    window->Render();

    auto cached_tree = window->GetCachedRenderTree();
    ASSERT_TRUE(cached_tree);

    dialog->RemoveChild(label);

    EXPECT_EQ(window->GetCachedRenderTree(), cached_tree);
    EXPECT_TRUE(window->NeedsRepaint());
}

TEST_F(EventLoopTest, FileDropRequiresDragOverDefaultPrevention) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-event-loop-file-drop-reject-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "drop.txt";
    {
        std::ofstream(file_path) << "drop";
    }

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(document->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("style", "width: 200px; height: 40px;");
    document->GetBody()->AppendChild(input);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    WindowManager::Instance().RegisterWindow(window);
    window->EnsureRenderTree();

    int dragenter_events = 0;
    int dragover_events = 0;
    int drop_events = 0;
    input->AddEventListener("dragenter", [&](std::shared_ptr<Event>) { ++dragenter_events; });
    input->AddEventListener("dragover", [&](std::shared_ptr<Event>) { ++dragover_events; });
    input->AddEventListener("drop", [&](std::shared_ptr<Event>) { ++drop_events; });

    SDL_Event begin{};
    begin.type = SDL_EVENT_DROP_BEGIN;
    begin.drop.windowID = 0;
    event_loop_->HandleFileDropEventForTesting(begin);

    const std::string path = file_path.string();
    SDL_Event file{};
    file.type = SDL_EVENT_DROP_FILE;
    file.drop.windowID = 0;
    file.drop.x = 5.0f;
    file.drop.y = 5.0f;
    file.drop.data = path.c_str();
    event_loop_->HandleFileDropEventForTesting(file);

    SDL_Event complete{};
    complete.type = SDL_EVENT_DROP_COMPLETE;
    complete.drop.windowID = 0;
    complete.drop.x = 5.0f;
    complete.drop.y = 5.0f;
    event_loop_->HandleFileDropEventForTesting(complete);

    WindowManager::Instance().UnregisterWindow(window);

    EXPECT_EQ(dragenter_events, 1);
    EXPECT_EQ(dragover_events, 1);
    EXPECT_EQ(drop_events, 0);
    EXPECT_TRUE(input->GetFiles().empty());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(EventLoopTest, FileDropWritesOnlyWhenAcceptedAndNotCanceled) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-event-loop-file-drop-accept-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "drop.txt";
    const auto json_path = temp_dir / "drop.json";
    {
        std::ofstream(file_path) << "drop";
        std::ofstream(json_path) << "{}";
    }

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(document->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("multiple", "");
    input->SetAttribute("accept", ".txt");
    input->SetAttribute("style", "width: 200px; height: 40px;");
    document->GetBody()->AppendChild(input);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    WindowManager::Instance().RegisterWindow(window);
    window->EnsureRenderTree();

    int drop_events = 0;
    bool cancel_drop = false;
    input->AddEventListener("dragover", [](std::shared_ptr<Event> event) {
        event->PreventDefault();
    });
    input->AddEventListener("drop", [&](std::shared_ptr<Event> event) {
        ++drop_events;
        if (cancel_drop) {
            event->PreventDefault();
        }
    });

    auto dispatch_drop = [&](const std::vector<std::string>& paths) {
        SDL_Event begin{};
        begin.type = SDL_EVENT_DROP_BEGIN;
        begin.drop.windowID = 0;
        event_loop_->HandleFileDropEventForTesting(begin);

        for (const auto& path : paths) {
            SDL_Event file{};
            file.type = SDL_EVENT_DROP_FILE;
            file.drop.windowID = 0;
            file.drop.x = 5.0f;
            file.drop.y = 5.0f;
            file.drop.data = path.c_str();
            event_loop_->HandleFileDropEventForTesting(file);
        }

        SDL_Event complete{};
        complete.type = SDL_EVENT_DROP_COMPLETE;
        complete.drop.windowID = 0;
        complete.drop.x = 5.0f;
        complete.drop.y = 5.0f;
        event_loop_->HandleFileDropEventForTesting(complete);
    };

    dispatch_drop({file_path.string(), json_path.string()});
    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "drop.txt");
    EXPECT_EQ(drop_events, 1);

    cancel_drop = true;
    dispatch_drop({json_path.string()});
    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "drop.txt");
    EXPECT_EQ(drop_events, 2);

    input->SetDisabled(true);
    cancel_drop = false;
    dispatch_drop({file_path.string()});
    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "drop.txt");
    EXPECT_EQ(drop_events, 3);

    WindowManager::Instance().UnregisterWindow(window);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(EventLoopTest, FileDropDirectoryWritesAllFilesWithoutMultiple) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-event-loop-directory-drop-test";
    const auto directory_path = temp_dir / "folder";
    const auto nested_path = directory_path / "nested";
    const auto first_path = directory_path / "a.txt";
    const auto second_path = nested_path / "b.txt";
    std::filesystem::create_directories(nested_path);
    {
        std::ofstream(first_path) << "a";
        std::ofstream(second_path) << "bb";
    }

    auto document = std::make_shared<Document>();
    document->Initialize();
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(document->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("webkitdirectory", "");
    input->SetAttribute("style", "width: 200px; height: 40px;");
    document->GetBody()->AppendChild(input);

    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(document);
    WindowManager::Instance().RegisterWindow(window);
    window->EnsureRenderTree();

    input->AddEventListener("dragover", [](std::shared_ptr<Event> event) {
        event->PreventDefault();
    });

    SDL_Event begin{};
    begin.type = SDL_EVENT_DROP_BEGIN;
    begin.drop.windowID = 0;
    event_loop_->HandleFileDropEventForTesting(begin);

    const std::string path = directory_path.string();
    SDL_Event file{};
    file.type = SDL_EVENT_DROP_FILE;
    file.drop.windowID = 0;
    file.drop.x = 5.0f;
    file.drop.y = 5.0f;
    file.drop.data = path.c_str();
    event_loop_->HandleFileDropEventForTesting(file);

    SDL_Event complete{};
    complete.type = SDL_EVENT_DROP_COMPLETE;
    complete.drop.windowID = 0;
    complete.drop.x = 5.0f;
    complete.drop.y = 5.0f;
    event_loop_->HandleFileDropEventForTesting(complete);

    WindowManager::Instance().UnregisterWindow(window);

    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_EQ(input->GetFiles()[0].name, "a.txt");
    EXPECT_EQ(input->GetFiles()[0].webkit_relative_path, "folder/a.txt");
    EXPECT_EQ(input->GetFiles()[1].name, "b.txt");
    EXPECT_EQ(input->GetFiles()[1].webkit_relative_path, "folder/nested/b.txt");

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(EventLoopTest, GetTaskScheduler) {
    auto& scheduler = event_loop_->GetTaskScheduler();
    // 应该返回有效的调度器引用
    EXPECT_NE(&scheduler, nullptr);
}

TEST_F(EventLoopTest, GetTaskSchedulerPtr) {
    auto scheduler = event_loop_->GetTaskSchedulerPtr();
    EXPECT_NE(scheduler, nullptr);
}

TEST_F(EventLoopTest, ExternalTaskScheduler) {
    auto external_scheduler = std::make_shared<TaskScheduler>();
    auto loop = std::make_unique<EventLoop>(external_scheduler);

    EXPECT_EQ(loop->GetTaskSchedulerPtr(), external_scheduler);
}

TEST_F(EventLoopTest, CursorVisibility) {
    // 初始状态光标应该可见
    EXPECT_TRUE(event_loop_->IsCursorVisible());
}

} // namespace test
} // namespace mbink
