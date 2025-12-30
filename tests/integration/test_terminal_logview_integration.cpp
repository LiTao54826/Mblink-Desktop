/**
 * @file test_terminal_logview_integration.cpp
 * @brief Terminal 和 LogView 元素集成测试
 *
 * 测试 HTMLTerminalElement 和 HTMLLogViewElement 的 DOM 操作和 JS API。
 */

#include "test_utils/test_helpers.h"

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/elements/logview/html_logview_element.h"
#include "core/dom/elements/logview/log_entry.h"
#include "core/dom/elements/terminal/html_terminal_element.h"

#include <gtest/gtest.h>

namespace lightui {
namespace test {

class TerminalLogViewIntegrationTest : public DOMTestBase {
protected:
    void SetUp() override { DOMTestBase::SetUp(); }

    void TearDown() override { DOMTestBase::TearDown(); }
};

// ========== Terminal 元素测试 ==========

TEST_F(TerminalLogViewIntegrationTest, CreateTerminalElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();

    // 模拟 JS: document.createElement('terminal')
    auto terminal = doc->CreateElement("terminal");
    ASSERT_NE(terminal, nullptr);

    // 模拟 JS: document.body.appendChild(terminal)
    body->AppendChild(terminal);

    // 验证元素已添加 - 通过 ID 查找验证
    terminal->SetAttribute("id", "test-terminal");
    auto found = doc->GetElementById("test-terminal");
    EXPECT_NE(found, nullptr);
}

TEST_F(TerminalLogViewIntegrationTest, TerminalDefaultAttributes) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 验证默认值
        EXPECT_EQ(terminal->rows(), 24);
        EXPECT_EQ(terminal->cols(), 80);
        EXPECT_EQ(terminal->scrollback(), 10000);
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalSetAttributes) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 模拟 JS: terminal.rows = 30
        terminal->set_rows(30);
        EXPECT_EQ(terminal->rows(), 30);

        // 模拟 JS: terminal.cols = 120
        terminal->set_cols(120);
        EXPECT_EQ(terminal->cols(), 120);

        // 模拟 JS: terminal.scrollback = 5000
        terminal->set_scrollback(5000);
        EXPECT_EQ(terminal->scrollback(), 5000);
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalWriteAndSerialize) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 模拟 JS: terminal.write('Hello World')
        terminal->Write("Hello World");

        // 模拟 JS: terminal.serialize()
        std::string content = terminal->Serialize();
        EXPECT_TRUE(content.find("Hello") != std::string::npos);
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalWriteWithAnsi) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 模拟 JS: terminal.write('\x1b[31mRed Text\x1b[0m')
        terminal->Write("\x1b[31mRed Text\x1b[0m");

        std::string content = terminal->Serialize();
        EXPECT_TRUE(content.find("Red Text") != std::string::npos);
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalClear) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        terminal->Write("Some content");
        std::string before = terminal->Serialize();
        EXPECT_FALSE(before.empty());

        // 模拟 JS: terminal.clear()
        terminal->Clear();

        std::string after = terminal->Serialize();
        // 清除后内容应该为空或只有空白
        EXPECT_TRUE(after.find("Some content") == std::string::npos);
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalScrollTo) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 写入多行内容
        for (int i = 0; i < 50; i++) {
            terminal->Write("Line " + std::to_string(i) + "\n");
        }

        // 模拟 JS: terminal.scrollTo(10)
        terminal->ScrollTo(10);

        // 不抛出异常即为成功
        SUCCEED();
    }
}

TEST_F(TerminalLogViewIntegrationTest, TerminalResize) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("terminal");
    auto* terminal = dynamic_cast<HTMLTerminalElement*>(elem.get());

    if (terminal) {
        // 模拟 JS: terminal.resize(40, 100)
        terminal->Resize(40, 100);

        EXPECT_EQ(terminal->rows(), 40);
        EXPECT_EQ(terminal->cols(), 100);
    }
}

// ========== LogView 元素测试 ==========

TEST_F(TerminalLogViewIntegrationTest, CreateLogViewElement) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();

    // 模拟 JS: document.createElement('logview')
    auto logview = doc->CreateElement("logview");
    ASSERT_NE(logview, nullptr);

    // 模拟 JS: document.body.appendChild(logview)
    body->AppendChild(logview);

    // 验证元素已添加
    logview->SetAttribute("id", "test-logview");
    auto found = doc->GetElementById("test-logview");
    EXPECT_NE(found, nullptr);
}

TEST_F(TerminalLogViewIntegrationTest, LogViewDefaultAttributes) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        // 验证默认值
        EXPECT_GT(logview->max_entries(), 0);
        EXPECT_TRUE(logview->auto_scroll());
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewSetAttributes) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        // 模拟 JS: logview.maxEntries = 5000
        logview->set_max_entries(5000);
        EXPECT_EQ(logview->max_entries(), 5000);

        // 模拟 JS: logview.autoScroll = false
        logview->set_auto_scroll(false);
        EXPECT_FALSE(logview->auto_scroll());
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewAppend) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        // 模拟 JS: logview.append('info', 'app', 'Application started')
        logview->Append(LogLevel::INFO, "app", "Application started");

        // 模拟 JS: logview.append('error', 'app', 'Something went wrong')
        logview->Append(LogLevel::ERROR, "app", "Something went wrong");

        // 验证条目数
        EXPECT_EQ(logview->GetLogCount(), 2u);
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewClear) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        logview->Append(LogLevel::INFO, "test", "Entry 1");
        logview->Append(LogLevel::INFO, "test", "Entry 2");
        EXPECT_EQ(logview->GetLogCount(), 2u);

        // 模拟 JS: logview.clear()
        logview->Clear();
        EXPECT_EQ(logview->GetLogCount(), 0u);
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewFilter) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        logview->Append(LogLevel::DEBUG, "app", "Debug message");
        logview->Append(LogLevel::INFO, "app", "Info message");
        logview->Append(LogLevel::WARN, "app", "Warning message");
        logview->Append(LogLevel::ERROR, "app", "Error message");

        // 模拟 JS: logview.setFilter({levels: ['ERROR', 'WARN']})
        std::vector<std::string> levels = {"ERROR", "WARN"};
        logview->SetLevelFilter(levels);

        // 过滤后总数不变，但显示的会变少
        EXPECT_EQ(logview->GetLogCount(), 4u);
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewSearch) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        logview->Append(LogLevel::INFO, "app", "User logged in");
        logview->Append(LogLevel::INFO, "app", "User performed action");
        logview->Append(LogLevel::ERROR, "app", "Connection failed");

        // 模拟 JS: logview.search('User')
        int matches = logview->Search("User");
        EXPECT_EQ(matches, 2);

        // 模拟 JS: logview.search('failed')
        matches = logview->Search("failed");
        EXPECT_EQ(matches, 1);
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewExport) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        logview->Append(LogLevel::INFO, "app", "Test entry");

        // 模拟 JS: logview.export('text')
        std::string text_export = logview->Export("text");
        EXPECT_TRUE(text_export.find("Test entry") != std::string::npos);

        // 模拟 JS: logview.export('json')
        std::string json_export = logview->Export("json");
        EXPECT_TRUE(json_export.find("Test entry") != std::string::npos);
    }
}

TEST_F(TerminalLogViewIntegrationTest, LogViewScrollTo) {
    auto doc = CreateDocument();

    auto elem = doc->CreateElement("logview");
    auto* logview = dynamic_cast<HTMLLogViewElement*>(elem.get());

    if (logview) {
        // 添加多条日志
        for (int i = 0; i < 100; i++) {
            logview->Append(LogLevel::INFO, "app", "Entry " + std::to_string(i));
        }

        // 模拟 JS: logview.scrollTo(50)
        logview->ScrollTo(50);

        // 不抛出异常即为成功
        SUCCEED();
    }
}

// ========== 组合测试 ==========

TEST_F(TerminalLogViewIntegrationTest, MultipleElementsInDocument) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();

    // 创建多个终端和日志视图
    auto terminal1 = doc->CreateElement("terminal");
    auto terminal2 = doc->CreateElement("terminal");
    auto logview1 = doc->CreateElement("logview");
    auto logview2 = doc->CreateElement("logview");

    terminal1->SetAttribute("id", "terminal1");
    terminal2->SetAttribute("id", "terminal2");
    logview1->SetAttribute("id", "logview1");
    logview2->SetAttribute("id", "logview2");

    body->AppendChild(terminal1);
    body->AppendChild(logview1);
    body->AppendChild(terminal2);
    body->AppendChild(logview2);

    // 验证所有元素都已添加
    EXPECT_NE(doc->GetElementById("terminal1"), nullptr);
    EXPECT_NE(doc->GetElementById("terminal2"), nullptr);
    EXPECT_NE(doc->GetElementById("logview1"), nullptr);
    EXPECT_NE(doc->GetElementById("logview2"), nullptr);
}

TEST_F(TerminalLogViewIntegrationTest, TerminalWithIdAttribute) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();

    auto terminal = doc->CreateElement("terminal");
    terminal->SetAttribute("id", "main-terminal");
    body->AppendChild(terminal);

    // 模拟 JS: document.getElementById('main-terminal')
    auto found = doc->GetElementById("main-terminal");
    EXPECT_NE(found, nullptr);
}

TEST_F(TerminalLogViewIntegrationTest, LogViewWithClassAttribute) {
    auto doc = CreateDocument();
    auto body = doc->GetBody();

    auto logview = doc->CreateElement("logview");
    logview->AddClass("debug-panel");
    logview->AddClass("visible");
    body->AppendChild(logview);

    EXPECT_TRUE(logview->HasClass("debug-panel"));
    EXPECT_TRUE(logview->HasClass("visible"));

    // 模拟 JS: logview.classList.remove('visible')
    logview->RemoveClass("visible");
    EXPECT_FALSE(logview->HasClass("visible"));
}

}  // namespace test
}  // namespace lightui
