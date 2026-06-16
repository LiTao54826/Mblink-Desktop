/**
 * @file test_dom_bindings.cpp
 * @brief DOM JavaScript 绑定单元测试
 */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <sstream>
#include <stdexcept>
#include "bridge/state_manager.h"
#include "core/utils/encoding_utils.h"
#include "quickjs/quickjs_runtime.h"
#include "quickjs/window_bindings.h"
#include "quickjs/bindings/js_data_transfer.h"
#include "dom/bindings/dom_bindings.h"
#include "dom/document.h"
#include "dom/elements/html_input_element.h"
#include "event/types/data_transfer.h"
#include "window/window.h"
#include "core/event/loop/task_scheduler.h"

namespace mbink {
namespace test {

namespace {

std::filesystem::path Utf8PathToFsPath(const std::string& path) {
#ifdef _WIN32
    return std::filesystem::path(mbink::utils::UTF8ToWide(path));
#else
    return std::filesystem::path(path);
#endif
}

std::string FsPathToUtf8String(const std::filesystem::path& path) {
#ifdef _WIN32
    return mbink::utils::WideToUTF8(path.wstring());
#else
    return path.string();
#endif
}

std::filesystem::path FindRepoRoot() {
    static const auto kOfficialPreactRelativeRoot =
        Utf8PathToFsPath("third_party") / Utf8PathToFsPath("preact") / Utf8PathToFsPath("package.json");
    auto current = std::filesystem::current_path();
    while (!current.empty()) {
        if (std::filesystem::exists(current / kOfficialPreactRelativeRoot)) {
            return current;
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate repository root for official Preact fixtures");
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input) {
        throw std::runtime_error("Failed to open JS fixture: " + FsPathToUtf8String(path));
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string NormalizeModulePath(const std::filesystem::path& path) {
    auto utf8 = std::filesystem::absolute(path).lexically_normal().u8string();
    std::string normalized(utf8.begin(), utf8.end());
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

void EvalJsFixture(QuickJSRuntime* runtime,
                   const std::filesystem::path& repo_root,
                   const char* relative_path,
                   const char* eval_name) {
    runtime->Eval(ReadTextFile(repo_root / "js" / relative_path), eval_name);
}

std::optional<std::string> ReadModuleFileForTest(const std::filesystem::path& repo_root,
                                                 const std::string& path) {
    if (path.empty()) {
        return std::nullopt;
    }

    std::filesystem::path candidate;
    if ((path.size() > 1 && path[1] == ':') || path.front() == '/') {
        candidate = Utf8PathToFsPath(path);
    } else {
        candidate = repo_root / Utf8PathToFsPath(path);
    }

    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec) || !std::filesystem::is_regular_file(candidate, ec)) {
        return std::nullopt;
    }

    return ReadTextFile(candidate);
}

bool EndsWithCaseInsensitive(std::string value, const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }

    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::string normalized_suffix = suffix;
    std::transform(normalized_suffix.begin(), normalized_suffix.end(), normalized_suffix.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value.compare(value.size() - normalized_suffix.size(), normalized_suffix.size(), normalized_suffix) == 0;
}

void RegisterOfficialPreactModules(QuickJSRuntime* runtime,
                                   const std::filesystem::path& repo_root) {
    static const auto kOfficialPreactRelativeRoot =
        Utf8PathToFsPath("third_party") / Utf8PathToFsPath("preact");
    const auto preact_root = repo_root / kOfficialPreactRelativeRoot;
    const auto preact_entry = NormalizeModulePath(preact_root / "src" / "index.js");
    const auto hooks_entry = NormalizeModulePath(preact_root / "hooks" / "src" / "index.js");
    const auto jsx_runtime_entry = NormalizeModulePath(preact_root / "jsx-runtime" / "src" / "index.js");

    runtime->RegisterModule("preact", "export * from '" + preact_entry + "';");
    runtime->RegisterModule("preact/hooks", "export * from '" + hooks_entry + "';");
    runtime->RegisterModule("preact/jsx-runtime", "export * from '" + jsx_runtime_entry + "';");
    runtime->RegisterModule("preact/jsx-dev-runtime", "export * from '" + jsx_runtime_entry + "';");
}

}  // namespace

class DOMBindingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime_ = std::make_unique<QuickJSRuntime>();
        repo_root_ = FindRepoRoot();
        runtime_->SetFileLoader([this](const std::string& path, std::string& out, std::string* error) {
            if (EndsWithCaseInsensitive(path, "/js/preact/preact.js")) {
                legacy_preact_requested_ = true;
            }
            if (EndsWithCaseInsensitive(path, "/js/preact/hooks.js")) {
                legacy_hooks_requested_ = true;
            }

            auto content = ReadModuleFileForTest(repo_root_, path);
            if (!content.has_value()) {
                if (error) {
                    *error = "File not found: " + path;
                }
                return false;
            }

            out = std::move(*content);
            return true;
        });

        doc_ = std::make_shared<Document>();
        doc_->Initialize();
        doc_->SetStateManager(&state_manager_);

        WindowConfig config;
        config.hidden = true;
        config.headless = true;
        config.backend = RenderBackend::CPU;
        window_ = std::make_shared<Window>(config);
        window_->SetDocument(doc_);

        task_scheduler_ = std::make_shared<TaskScheduler>();
        window_bindings_ = std::make_unique<WindowBindings>(runtime_.get(), window_, task_scheduler_);
        window_bindings_->InitBindings();

        EvalJsFixture(runtime_.get(), repo_root_, "polyfills/dom.js", "dom.js");
        EvalJsFixture(runtime_.get(), repo_root_, "runtime/bootstrap.js", "bootstrap.js");
        RegisterOfficialPreactModules(runtime_.get(), repo_root_);
    }

    void TearDown() override {
        HTMLInputElement::SetFilePickerForTesting(nullptr);
        if (window_bindings_) {
            window_bindings_->Cleanup();
            DOMBindings::Cleanup(nullptr);
        }
        window_bindings_.reset();
        window_.reset();
        doc_.reset();
        runtime_.reset();
    }

protected:
    std::unique_ptr<QuickJSRuntime> runtime_;
    std::filesystem::path repo_root_;
    std::shared_ptr<Document> doc_;
    std::shared_ptr<Window> window_;
    std::shared_ptr<TaskScheduler> task_scheduler_;
    std::unique_ptr<WindowBindings> window_bindings_;
    StateManager state_manager_;
    bool legacy_preact_requested_ = false;
    bool legacy_hooks_requested_ = false;
};

// ========== document 对象测试 ==========

TEST_F(DOMBindingsTest, DocumentExists) {
    auto result = runtime_->Eval("typeof document");
    EXPECT_EQ(result, "object");
}

TEST_F(DOMBindingsTest, WindowDocumentAlias) {
    auto result = runtime_->Eval("window.document === document");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentBody) {
    auto result = runtime_->Eval("document.body !== null");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentHead) {
    auto result = runtime_->Eval("document.head !== null");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentDocumentElement) {
    auto result = runtime_->Eval("document.documentElement !== null");
    EXPECT_EQ(result, true);
}

// ========== createElement 测试 ==========

TEST_F(DOMBindingsTest, CreateElement) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.tagName.toLowerCase();
    )");
    EXPECT_EQ(result, "div");
}

TEST_F(DOMBindingsTest, AudioElementExposesFirstPassMediaApi) {
    auto result = runtime_->Eval(R"(
        const audio = document.createElement('audio');
        audio.src = 'tone.wav';
        audio.controls = true;
        audio.loop = true;
        audio.muted = true;
        audio.volume = 0.25;
        audio.currentTime = 3;
        [
          audio.tagName.toLowerCase(),
          audio.src,
          audio.controls,
          audio.loop,
          audio.muted,
          audio.volume,
          typeof audio.currentTime,
          typeof audio.duration,
          audio.paused,
          audio.ended,
          typeof audio.load,
          typeof audio.play,
          typeof audio.pause
        ].join('|');
    )");

    EXPECT_EQ(result, "audio|tone.wav|true|true|true|0.25|number|number|true|false|function|function|function");
}

TEST_F(DOMBindingsTest, AudioPropertiesAreUndefinedOnNonAudioElements) {
    auto result = runtime_->Eval(R"(
        const div = document.createElement('div');
        [
          typeof div.controls,
          typeof div.loop,
          typeof div.muted,
          typeof div.volume,
          typeof div.currentTime,
          typeof div.duration,
          typeof div.paused,
          typeof div.ended
        ].join('|');
    )");

    EXPECT_EQ(result, "undefined|undefined|undefined|undefined|undefined|undefined|undefined|undefined");
}

TEST_F(DOMBindingsTest, CreateTextNode) {
    auto result = runtime_->Eval(R"(
        var text = document.createTextNode('Hello');
        text.textContent;
    )");
    EXPECT_EQ(result, "Hello");
}

TEST_F(DOMBindingsTest, DocumentNativeBindingTextApi) {
    ASSERT_EQ(state_manager_.createJson("profile", json{{"name", "Alice"}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        globalThis.__bindHost = document.createElement('span');
        globalThis.__bindText = document.createTextNode('');
        __bindHost.appendChild(__bindText);
        document.body.appendChild(__bindHost);
        document.nativeBinding.text(__bindHost, __bindText, 'profile.name');
        document.nativeBinding.flush();
        __bindText.textContent;
    )");
    EXPECT_EQ(result, "Alice");

    ASSERT_TRUE(state_manager_.set("profile.name", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        __bindText.textContent;
    )");
    EXPECT_EQ(result, "Bob");
}

TEST_F(DOMBindingsTest, DocumentNativeBindingModelValueApi) {
    ASSERT_EQ(state_manager_.createJson("form", json{{"username", "Alice"}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        globalThis.__host = document.createElement('div');
        globalThis.__input = document.createElement('input');
        __host.appendChild(__input);
        document.body.appendChild(__host);
        document.nativeBinding.modelValue(__host, __input, 'form.username');
        document.nativeBinding.flush();
        __input.value;
    )");
    EXPECT_EQ(result, "Alice");

    ASSERT_TRUE(state_manager_.set("form.username", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        __input.value;
    )");
    EXPECT_EQ(result, "Bob");
}

TEST_F(DOMBindingsTest, DocumentNativeBindingMountDeclarativeApi) {
    ASSERT_EQ(state_manager_.createJson("profile", json{{"name", "Alice"}}), MBinkError::Ok);
    ASSERT_EQ(state_manager_.createJson("form", json{{"username", "tom"}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        globalThis.__declRoot = document.createElement('div');
        globalThis.__declText = document.createElement('span');
        globalThis.__declInput = document.createElement('input');
        __declText.setAttribute('mb-text', 'profile.name');
        __declInput.setAttribute('mb-model', 'form.username');
        __declRoot.appendChild(__declText);
        __declRoot.appendChild(__declInput);
        document.body.appendChild(__declRoot);
        document.nativeBinding.mountDeclarative(__declRoot);
        document.nativeBinding.flush();
        JSON.stringify({ text: __declText.textContent, value: __declInput.value });
    )");
    EXPECT_EQ(result, R"({"text":"Alice","value":"tom"})");

    ASSERT_TRUE(state_manager_.set("profile.name", "Bob"));
    ASSERT_TRUE(state_manager_.set("form.username", "rose"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        JSON.stringify({ text: __declText.textContent, value: __declInput.value });
    )");
    EXPECT_EQ(result, R"({"text":"Bob","value":"rose"})");
}

TEST_F(DOMBindingsTest, DocumentLoadHTMLAutoMountsDeclarativeBindings) {
    ASSERT_EQ(state_manager_.createJson("page", json{{"form", json{{"user", json{{"name", "Alice"}}}}}}), MBinkError::Ok);

    ASSERT_TRUE(doc_->LoadHTML("<html><body><div mb-scope:user='page.form.user'><span id='name' mb-text='user.name'></span></div></body></html>"));

    auto result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        document.getElementById('name').textContent;
    )");
    EXPECT_EQ(result, "Alice");

    ASSERT_TRUE(state_manager_.set("page.form.user.name", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        document.getElementById('name').textContent;
    )");
    EXPECT_EQ(result, "Bob");
}

TEST_F(DOMBindingsTest, DocumentExecuteScriptsTriggersSecondDeclarativeScan) {
    ASSERT_EQ(state_manager_.createJson("profile", json{{"name", "Alice"}}), MBinkError::Ok);

    ASSERT_TRUE(doc_->LoadHTML(R"(
        <html><body>
            <script>
                var span = document.createElement('span');
                span.id = 'dyn';
                span.setAttribute('mb-text', 'profile.name');
                document.body.appendChild(span);
            </script>
        </body></html>
    )"));
    doc_->ExecuteScripts(runtime_.get());

    auto result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        document.getElementById('dyn').textContent;
    )");
    EXPECT_EQ(result, "Alice");

    ASSERT_TRUE(state_manager_.set("profile.name", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        document.getElementById('dyn').textContent;
    )");
    EXPECT_EQ(result, "Bob");
}

TEST_F(DOMBindingsTest, DynamicSetAttributeAndAppendChildAutoMountDeclarativeBindings) {
    ASSERT_EQ(state_manager_.createJson("profile", json{{"name", "Alice"}, {"title", "Admin"}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        var afterAppend = document.createElement('span');
        afterAppend.id = 'after-append';
        document.body.appendChild(afterAppend);
        afterAppend.setAttribute('mb-text', 'profile.name');

        var beforeAppend = document.createElement('div');
        beforeAppend.id = 'before-append';
        beforeAppend.setAttribute('mb-attr:title', 'profile.title');
        document.body.appendChild(beforeAppend);

        document.nativeBinding.flush();
        JSON.stringify({
            text: document.getElementById('after-append').textContent,
            title: document.getElementById('before-append').getAttribute('title')
        });
    )");
    EXPECT_EQ(result, R"({"text":"Alice","title":"Admin"})");

    ASSERT_TRUE(state_manager_.set("profile.name", "Bob"));
    ASSERT_TRUE(state_manager_.set("profile.title", "Owner"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        JSON.stringify({
            text: document.getElementById('after-append').textContent,
            title: document.getElementById('before-append').getAttribute('title')
        });
    )");
    EXPECT_EQ(result, R"({"text":"Bob","title":"Owner"})");
}

TEST_F(DOMBindingsTest, RemoveAttributeUnmountsDeclarativeBindingsPrecisely) {
    ASSERT_EQ(state_manager_.createJson("page", json{{"form", json{{"user", json{{"name", "Alice"}}}}}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        var host = document.createElement('div');
        host.id = 'scope-host';
        host.setAttribute('mb-scope:user', 'page.form.user');
        var text = document.createElement('span');
        text.id = 'scope-text';
        text.setAttribute('mb-text', 'user.name');
        host.appendChild(text);
        document.body.appendChild(host);
        document.nativeBinding.flush();
        document.getElementById('scope-text').textContent;
    )");
    EXPECT_EQ(result, "Alice");

    result = runtime_->Eval(R"(
        document.getElementById('scope-host').removeAttribute('mb-scope:user');
        document.nativeBinding.flush();
        document.getElementById('scope-text').textContent;
    )");
    EXPECT_EQ(result, "");

    ASSERT_TRUE(state_manager_.set("page.form.user.name", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        document.getElementById('scope-text').textContent;
    )");
    EXPECT_EQ(result, "");
}

TEST_F(DOMBindingsTest, RemoveChildUnmountsDeclarativeSubtree) {
    ASSERT_EQ(state_manager_.createJson("profile", json{{"name", "Alice"}}), MBinkError::Ok);

    auto result = runtime_->Eval(R"(
        globalThis.__host = document.createElement('div');
        globalThis.__text = document.createElement('span');
        __text.setAttribute('mb-text', 'profile.name');
        __host.appendChild(__text);
        document.body.appendChild(__host);
        document.nativeBinding.flush();
        __text.textContent;
    )");
    EXPECT_EQ(result, "Alice");

    result = runtime_->Eval(R"(
        document.body.removeChild(__host);
        document.nativeBinding.flush();
        __text.textContent;
    )");
    EXPECT_EQ(result, "Alice");

    ASSERT_TRUE(state_manager_.set("profile.name", "Bob"));
    result = runtime_->Eval(R"(
        document.nativeBinding.flush();
        __text.textContent;
    )");
    EXPECT_EQ(result, "Alice");
}

// ========== 元素属性测试 ==========

TEST_F(DOMBindingsTest, SetAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('id', 'test');
        div.getAttribute('id');
    )");
    EXPECT_EQ(result, "test");
}

TEST_F(DOMBindingsTest, HasAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('class', 'foo');
        div.hasAttribute('class');
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, RemoveAttribute) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.setAttribute('id', 'test');
        div.removeAttribute('id');
        div.hasAttribute('id');
    )");
    EXPECT_EQ(result, false);
}

// ========== className 和 classList 测试 ==========

TEST_F(DOMBindingsTest, ClassName) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.className = 'foo bar';
        div.className;
    )");
    EXPECT_EQ(result, "foo bar");
}

TEST_F(DOMBindingsTest, ClassListAdd) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.add('active');
        div.classList.contains('active');
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, ClassListRemove) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.add('active');
        div.classList.remove('active');
        div.classList.contains('active');
    )");
    EXPECT_EQ(result, false);
}

TEST_F(DOMBindingsTest, ClassListToggle) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.classList.toggle('active');
        var first = div.classList.contains('active');
        div.classList.toggle('active');
        var second = div.classList.contains('active');
        first && !second;
    )");
    EXPECT_EQ(result, true);
}

// ========== style 测试 ==========

TEST_F(DOMBindingsTest, StyleProperty) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.style.color = 'red';
        div.style.color;
    )");
    EXPECT_EQ(result, "red");
}

TEST_F(DOMBindingsTest, StyleSetProperty) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.style.setProperty('background-color', 'blue');
        div.style.getPropertyValue('background-color');
    )");
    EXPECT_EQ(result, "blue");
}

// ========== DOM 操作测试 ==========

TEST_F(DOMBindingsTest, StyleVendorPrefixedProperty) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.style.WebkitAppRegion = 'no-drag';
        div.style.WebkitWindowControl = 'close';
        div.style.getPropertyValue('-webkit-app-region') + '|' +
            div.style.getPropertyValue('-webkit-window-control');
    )");
    EXPECT_EQ(result, "no-drag|close");
}

TEST_F(DOMBindingsTest, AppendChild) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        parent.children.length;
    )");
    EXPECT_EQ(result, 1);
}

TEST_F(DOMBindingsTest, RemoveChild) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        parent.removeChild(child);
        parent.children.length;
    )");
    EXPECT_EQ(result, 0);
}

TEST_F(DOMBindingsTest, InsertBefore) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child1 = document.createElement('span');
        var child2 = document.createElement('p');
        parent.appendChild(child2);
        parent.insertBefore(child1, child2);
        parent.firstChild.tagName.toLowerCase();
    )");
    EXPECT_EQ(result, "span");
}

// ========== 查询选择器测试 ==========

TEST_F(DOMBindingsTest, QuerySelector) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        span.id = 'target';
        div.appendChild(span);
        document.body.appendChild(div);
        var found = document.querySelector('#target');
        found !== null;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, QuerySelectorAll) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span></span><span></span><span></span>';
        document.body.appendChild(div);
        document.querySelectorAll('span').length;
    )");
    EXPECT_GE(result.get<int>(), 3);
}

TEST_F(DOMBindingsTest, TableRangeToStringUsesTabAndNewlineSeparators) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML =
            '<table><tbody>' +
            '<tr><td>Alpha</td><td>Beta</td></tr>' +
            '<tr><td>Gamma</td><td>Delta</td></tr>' +
            '</tbody></table>';

        var cells = document.querySelectorAll('td');
        var range = document.createRange();
        range.setStart(cells[0].childNodes[0], 0);
        range.setEnd(cells[3].childNodes[0], 5);

        range.toString();
    )");
    EXPECT_EQ(result, "Alpha\tBeta\nGamma\tDelta");
}

TEST_F(DOMBindingsTest, GetComputedStyleReportsStandardTableDisplayValues) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML =
            '<table id="table">' +
            '<caption id="caption">People</caption>' +
            '<thead id="head"><tr id="head-row"><th id="head-cell">Name</th></tr></thead>' +
            '<tbody id="body"><tr id="body-row"><td id="body-cell">Alice</td></tr></tbody>' +
            '<tfoot id="foot"><tr id="foot-row"><td id="foot-cell">Total</td></tr></tfoot>' +
            '</table>';

        JSON.stringify({
            table: getComputedStyle(document.getElementById('table')).display,
            caption: getComputedStyle(document.getElementById('caption')).display,
            head: getComputedStyle(document.getElementById('head')).display,
            body: getComputedStyle(document.getElementById('body')).display,
            foot: getComputedStyle(document.getElementById('foot')).display,
            row: getComputedStyle(document.getElementById('body-row')).display,
            cell: getComputedStyle(document.getElementById('body-cell')).display,
            th: getComputedStyle(document.getElementById('head-cell')).display
        });
    )");
    EXPECT_EQ(result,
              R"({"table":"table","caption":"table-caption","head":"table-header-group","body":"table-row-group","foot":"table-footer-group","row":"table-row","cell":"table-cell","th":"table-cell"})");
}

TEST_F(DOMBindingsTest, ScrollContainerExposesClientSizeForTableOverflow) {
    auto setup_result = runtime_->Eval(R"(
        document.body.innerHTML =
            '<div id="scroller" style="width: 520px; height: 260px; overflow: auto; border: 1px solid #000;">' +
            '<table id="table" style="min-width: 920px; border-spacing: 0;">' +
            '<tbody><tr>' +
            '<td style="min-width: 140px; padding: 10px 12px;">Alpha</td>' +
            '<td style="min-width: 140px; padding: 10px 12px;">North</td>' +
            '<td style="min-width: 140px; padding: 10px 12px;">Ready</td>' +
            '<td style="min-width: 140px; padding: 10px 12px;">Owner</td>' +
            '<td style="min-width: 140px; padding: 10px 12px;">Updated</td>' +
            '<td style="min-width: 140px; padding: 10px 12px;">Notes</td>' +
            '</tr></tbody></table></div>';
        'ok';
    )");
    EXPECT_EQ(setup_result, "ok");

    window_->EnsureRenderTree();

    auto result = runtime_->Eval(R"(
        var scroller = document.getElementById('scroller');
        var table = document.getElementById('table');
        JSON.stringify({
            clientWidth: scroller.clientWidth,
            clientHeight: scroller.clientHeight,
            hasHorizontalOverflow: scroller.scrollWidth > scroller.clientWidth,
            tableOverflowsScroller: table.getBoundingClientRect().width > scroller.clientWidth
        });
    )");

    EXPECT_EQ(result,
              R"({"clientWidth":520,"clientHeight":260,"hasHorizontalOverflow":true,"tableOverflowsScroller":true})");
}

TEST_F(DOMBindingsTest, InputScrollLeftIsNotExposedAsPublicApi) {
    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.value = 'abcdefghijklmnopqrstuvwxyz';
        input.scrollLeft = 40;
        JSON.stringify({
            inputScrollLeftType: typeof input.scrollLeft,
            inputHasScrollLeft: 'scrollLeft' in input,
            elementPrototypeHasScrollLeft:
                Object.getOwnPropertyDescriptor(Object.getPrototypeOf(input), 'scrollLeft') !== undefined,
            divScrollLeftType: typeof document.createElement('div').scrollLeft
        });
    )");

    EXPECT_EQ(result,
              R"({"inputScrollLeftType":"undefined","inputHasScrollLeft":false,"elementPrototypeHasScrollLeft":false,"divScrollLeftType":"number"})");
}

TEST_F(DOMBindingsTest, TableElementExposesRowsAndSectionCollections) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML =
            '<table id="people">' +
            '<caption id="caption">People</caption>' +
            '<thead id="head"><tr id="head-row"><th>Name</th></tr></thead>' +
            '<tbody id="body-a"><tr id="row-a"><td>Alice</td></tr></tbody>' +
            '<tbody id="body-b"><tr id="row-b"><td>Bob</td></tr></tbody>' +
            '<tfoot id="foot"><tr id="foot-row"><td>Total</td></tr></tfoot>' +
            '</table>';

        var table = document.getElementById('people');
        JSON.stringify({
            caption: table.caption.id,
            tHead: table.tHead.id,
            tFoot: table.tFoot.id,
            bodies: [table.tBodies.length, table.tBodies[0].id, table.tBodies[1].id],
            rows: [
                table.rows.length,
                table.rows[0].id,
                table.rows[1].id,
                table.rows[2].id,
                table.rows[3].id
            ],
            sectionRows: [table.tBodies[0].rows.length, table.tBodies[0].rows[0].id]
        });
    )");
    EXPECT_EQ(result,
              R"({"caption":"caption","tHead":"head","tFoot":"foot","bodies":[2,"body-a","body-b"],"rows":[4,"head-row","row-a","row-b","foot-row"],"sectionRows":[1,"row-a"]})");
}

TEST_F(DOMBindingsTest, TableRowsAndCellsExposeIndexesAndSpanProperties) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML =
            '<table>' +
            '<thead><tr id="head-row"><th id="head-cell">Name</th></tr></thead>' +
            '<tbody>' +
            '<tr id="row-a"><td id="cell-a" colspan="2" rowspan="0">A</td><th id="cell-b">B</th></tr>' +
            '<tr id="row-b"><td>C</td></tr>' +
            '</tbody>' +
            '</table>';

        var rowA = document.getElementById('row-a');
        var rowB = document.getElementById('row-b');
        var cellA = document.getElementById('cell-a');
        var cellB = document.getElementById('cell-b');

        var before = {
            rowIndex: rowA.rowIndex,
            sectionRowIndex: rowA.sectionRowIndex,
            nextRowIndex: rowB.rowIndex,
            cells: [rowA.cells.length, rowA.cells[0].id, rowA.cells[1].id],
            cellIndexes: [cellA.cellIndex, cellB.cellIndex],
            spans: [cellA.colSpan, cellA.rowSpan]
        };

        cellA.colSpan = 3;
        cellA.rowSpan = 2;
        var after = {
            spans: [cellA.colSpan, cellA.rowSpan],
            attrs: [cellA.getAttribute('colspan'), cellA.getAttribute('rowspan')]
        };

        JSON.stringify({ before: before, after: after });
    )");
    EXPECT_EQ(result,
              R"({"before":{"rowIndex":1,"sectionRowIndex":0,"nextRowIndex":2,"cells":[2,"cell-a","cell-b"],"cellIndexes":[0,1],"spans":[2,0]},"after":{"spans":[3,2],"attrs":["3","2"]}})");
}

TEST_F(DOMBindingsTest, TableDomMutationMethodsCreateAndDeleteRowsCellsAndSections) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        var table = document.createElement('table');
        document.body.appendChild(table);

        var rowA = table.insertRow();
        rowA.id = 'row-a';
        rowA.insertCell().textContent = 'A';
        rowA.insertCell(0).textContent = 'B';

        var rowB = table.insertRow(-1);
        rowB.id = 'row-b';
        rowB.insertCell().textContent = 'C';
        rowB.deleteCell(0);
        rowB.insertCell().textContent = 'D';

        var caption = table.createCaption();
        caption.id = 'caption';
        caption.textContent = 'People';
        var head = table.createTHead();
        head.id = 'head';
        var headRow = head.insertRow();
        headRow.id = 'head-row';
        headRow.insertCell().textContent = 'Name';
        var foot = table.createTFoot();
        foot.id = 'foot';
        var footRow = foot.insertRow();
        footRow.id = 'foot-row';
        footRow.insertCell().textContent = 'Total';

        table.deleteRow(1);
        var beforeDeletes = {
            caption: table.caption.id,
            tHead: table.tHead.id,
            tFoot: table.tFoot.id,
            bodies: table.tBodies.length,
            rows: [table.rows.length, table.rows[0].id, table.rows[1].id, table.rows[2].id],
            rowB: [rowB.rowIndex, rowB.sectionRowIndex, rowB.cells.length, rowB.cells[0].textContent],
            rowAttached: rowA.parentNode === null
        };

        table.deleteCaption();
        table.deleteTHead();
        table.deleteTFoot();
        var afterDeletes = {
            caption: table.caption,
            tHead: table.tHead,
            tFoot: table.tFoot,
            rows: [table.rows.length, table.rows[0].id]
        };

        JSON.stringify({ beforeDeletes: beforeDeletes, afterDeletes: afterDeletes });
    )");
    EXPECT_EQ(result,
              R"({"beforeDeletes":{"caption":"caption","tHead":"head","tFoot":"foot","bodies":1,"rows":[3,"head-row","row-b","foot-row"],"rowB":[1,0,1,"D"],"rowAttached":true},"afterDeletes":{"caption":null,"tHead":null,"tFoot":null,"rows":[1,"row-b"]}})");
}

TEST_F(DOMBindingsTest, TableRowsFollowBrowserOrderAndInsertRowUsesRowsCollection) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        var table = document.createElement('table');
        document.body.appendChild(table);

        var foot = document.createElement('tfoot');
        var footRow = document.createElement('tr');
        footRow.id = 'foot-row';
        foot.appendChild(footRow);
        table.appendChild(foot);

        var directRow = document.createElement('tr');
        directRow.id = 'direct-row';
        table.appendChild(directRow);

        var head = document.createElement('thead');
        var headRow = document.createElement('tr');
        headRow.id = 'head-row';
        head.appendChild(headRow);
        table.appendChild(head);

        var body = document.createElement('tbody');
        var bodyRow = document.createElement('tr');
        bodyRow.id = 'body-row';
        body.appendChild(bodyRow);
        table.appendChild(body);

        var orderBefore = [
            table.rows[0].id,
            table.rows[1].id,
            table.rows[2].id,
            table.rows[3].id
        ];
        var indexesBefore = [headRow.rowIndex, directRow.rowIndex, bodyRow.rowIndex, footRow.rowIndex];
        var sectionIndexesBefore = [
            headRow.sectionRowIndex,
            directRow.sectionRowIndex,
            bodyRow.sectionRowIndex,
            footRow.sectionRowIndex
        ];

        var appended = table.insertRow(-1);
        appended.id = 'appended-row';

        var inserted = table.insertRow(1);
        inserted.id = 'inserted-row';

        JSON.stringify({
            orderBefore: orderBefore,
            indexesBefore: indexesBefore,
            sectionIndexesBefore: sectionIndexesBefore,
            appendedParent: appended.parentNode.tagName.toLowerCase(),
            insertedParent: inserted.parentNode.tagName.toLowerCase(),
            orderAfter: [
                table.rows[0].id,
                table.rows[1].id,
                table.rows[2].id,
                table.rows[3].id,
                table.rows[4].id,
                table.rows[5].id
            ],
            indexesAfter: [inserted.rowIndex, appended.rowIndex],
            sectionIndexesAfter: [
                inserted.sectionRowIndex,
                directRow.sectionRowIndex,
                bodyRow.sectionRowIndex,
                footRow.sectionRowIndex,
                appended.sectionRowIndex
            ]
        });
    )");
    EXPECT_EQ(result,
              R"({"orderBefore":["head-row","direct-row","body-row","foot-row"],"indexesBefore":[0,1,2,3],"sectionIndexesBefore":[0,1,0,0],"appendedParent":"tfoot","insertedParent":"table","orderAfter":["head-row","inserted-row","direct-row","body-row","foot-row","appended-row"],"indexesAfter":[1,5],"sectionIndexesAfter":[1,2,0,0,1]})");
}

TEST_F(DOMBindingsTest, TableCreateTBodyInsertsAfterLastTBodyAndBeforeFollowingSections) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        var table = document.createElement('table');
        document.body.appendChild(table);

        var bodyA = document.createElement('tbody');
        bodyA.id = 'body-a';
        table.appendChild(bodyA);

        var foot = document.createElement('tfoot');
        foot.id = 'foot';
        table.appendChild(foot);

        var bodyB = table.createTBody();
        bodyB.id = 'body-b';

        var tableWithoutBodies = document.createElement('table');
        document.body.appendChild(tableWithoutBodies);
        var lonelyFoot = document.createElement('tfoot');
        lonelyFoot.id = 'lonely-foot';
        tableWithoutBodies.appendChild(lonelyFoot);
        var createdBody = tableWithoutBodies.createTBody();
        createdBody.id = 'created-body';

        JSON.stringify({
            bodyCount: table.tBodies.length,
            orderWithBody: [
                table.childNodes[0].id,
                table.childNodes[1].id,
                table.childNodes[2].id
            ],
            orderWithoutBody: [
                tableWithoutBodies.childNodes[0].id,
                tableWithoutBodies.childNodes[1].id
            ]
        });
    )");
    EXPECT_EQ(result,
              R"({"bodyCount":2,"orderWithBody":["body-a","body-b","foot"],"orderWithoutBody":["lonely-foot","created-body"]})");
}

TEST_F(DOMBindingsTest, TableColumnElementsExposeStandardSpanProperty) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        var col = document.createElement('col');
        var group = document.createElement('colgroup');

        var defaults = [col.span, group.span];

        col.span = 3;
        group.span = 2;
        var afterSet = [
            col.span,
            col.getAttribute('span'),
            group.span,
            group.getAttribute('span')
        ];

        col.span = 0;
        group.span = -4;
        var afterClamp = [
            col.span,
            col.getAttribute('span'),
            group.span,
            group.getAttribute('span')
        ];

        JSON.stringify({ defaults: defaults, afterSet: afterSet, afterClamp: afterClamp });
    )");
    EXPECT_EQ(result,
              R"({"defaults":[1,1],"afterSet":[3,"3",2,"2"],"afterClamp":[1,"1",1,"1"]})");
}

TEST_F(DOMBindingsTest, GetElementById) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.id = 'myDiv';
        document.body.appendChild(div);
        document.getElementById('myDiv') !== null;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, GetElementsByTagName) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        document.body.appendChild(document.createElement('span'));
        document.body.appendChild(document.createElement('div'));
        document.body.appendChild(document.createElement('span'));
        document.getElementsByTagName('span').length;
    )");
    EXPECT_EQ(result, 2);
}

TEST_F(DOMBindingsTest, GetElementsByClassName) {
    auto result = runtime_->Eval(R"(
        document.body.innerHTML = '';
        var a = document.createElement('div');
        var b = document.createElement('span');
        var c = document.createElement('p');
        a.className = 'item';
        b.className = 'item active';
        c.className = 'other';
        document.body.appendChild(a);
        document.body.appendChild(b);
        document.body.appendChild(c);
        document.getElementsByClassName('item').length;
    )");
    EXPECT_EQ(result, 2);
}

TEST_F(DOMBindingsTest, CreateElementNSAndSVGAttributeAliases) {
    auto result = runtime_->Eval(R"(
        var svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        var defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
        var gradient = document.createElementNS('http://www.w3.org/2000/svg', 'linearGradient');
        var stop = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
        var polyline = document.createElementNS('http://www.w3.org/2000/svg', 'polyline');

        svg.setAttributeNS(null, 'viewBox', '0 0 300 44');
        svg.setAttributeNS(null, 'preserveAspectRatio', 'none');
        gradient.setAttribute('id', 'g_cpu');
        stop.setAttributeNS(null, 'stopColor', '#7dd3fc');
        stop.setAttributeNS(null, 'stopOpacity', '0.35');
        polyline.setAttributeNS(null, 'strokeWidth', '1.5');
        polyline.setAttributeNS(null, 'strokeLinejoin', 'round');

        gradient.appendChild(stop);
        defs.appendChild(gradient);
        svg.appendChild(defs);
        svg.appendChild(polyline);

        [
            svg.tagName === 'svg',
            defs.tagName === 'defs',
            gradient.tagName === 'linearGradient',
            stop.tagName === 'stop',
            polyline.tagName === 'polyline',
            svg.getAttribute('viewBox') === '0 0 300 44',
            svg.getAttributeNS(null, 'preserveAspectRatio') === 'none',
            stop.getAttribute('stop-color') === '#7dd3fc',
            stop.getAttributeNS(null, 'stopColor') === '#7dd3fc',
            polyline.getAttribute('stroke-width') === '1.5',
            polyline.getAttributeNS(null, 'strokeLinejoin') === 'round'
        ].every(Boolean);
    )");
    EXPECT_EQ(result, true);
}



// ========== 事件监听器测试 ==========


TEST_F(DOMBindingsTest, OfficialPreactCoreModulesImportWork) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import {
            h,
            render,
            Fragment,
            createElement,
            createContext,
            cloneElement,
            createRef,
            Component,
            isValidElement,
            options
        } from 'preact';
        import {
            useState,
            useEffect,
            useRef,
            useMemo,
            useCallback,
            useContext,
            useReducer,
            useLayoutEffect,
            useImperativeHandle,
            useDebugValue
        } from 'preact/hooks';

        globalThis.__officialPreactCoreOk =
            typeof h === 'function' &&
            typeof render === 'function' &&
            typeof Fragment !== 'undefined' &&
            typeof createElement === 'function' &&
            typeof createContext === 'function' &&
            typeof cloneElement === 'function' &&
            typeof createRef === 'function' &&
            typeof Component === 'function' &&
            typeof isValidElement === 'function' &&
            typeof options === 'object' &&
            typeof useState === 'function' &&
            typeof useEffect === 'function' &&
            typeof useRef === 'function' &&
            typeof useMemo === 'function' &&
            typeof useCallback === 'function' &&
            typeof useContext === 'function' &&
            typeof useReducer === 'function' &&
            typeof useLayoutEffect === 'function' &&
            typeof useImperativeHandle === 'function' &&
            typeof useDebugValue === 'function';
    )", "<official-preact-core-import-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactCoreOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactJsxRuntimeModulesImportWork) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { jsx, jsxs, jsxDEV, Fragment, jsxTemplate, jsxAttr, jsxEscape } from 'preact/jsx-runtime';
        import { jsxDEV as devJsxDEV, Fragment as devFragment } from 'preact/jsx-dev-runtime';

        globalThis.__officialPreactJsxRuntimeOk =
            typeof jsx === 'function' &&
            typeof jsxs === 'function' &&
            typeof jsxDEV === 'function' &&
            typeof Fragment !== 'undefined' &&
            typeof jsxTemplate === 'function' &&
            typeof jsxAttr === 'function' &&
            typeof jsxEscape === 'function' &&
            typeof devJsxDEV === 'function' &&
            typeof devFragment !== 'undefined';
    )", "<official-preact-jsx-runtime-import-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactJsxRuntimeOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactImportsDoNotUseLegacyGlobalsOrLegacyFiles) {
    EXPECT_NO_THROW(runtime_->Eval(R"(
        globalThis.__officialPreactLegacyGlobalReads = 0;
        Object.defineProperty(globalThis, 'Preact', {
            configurable: true,
            get() {
                globalThis.__officialPreactLegacyGlobalReads++;
                throw new Error('legacy Preact global should not be read');
            }
        });
        Object.defineProperty(globalThis, 'preact', {
            configurable: true,
            get() {
                globalThis.__officialPreactLegacyGlobalReads++;
                throw new Error('legacy preact global should not be read');
            }
        });
        Object.defineProperty(globalThis, 'PreactHooks', {
            configurable: true,
            get() {
                globalThis.__officialPreactLegacyGlobalReads++;
                throw new Error('legacy PreactHooks global should not be read');
            }
        });
        Object.defineProperty(globalThis, 'preactHooks', {
            configurable: true,
            get() {
                globalThis.__officialPreactLegacyGlobalReads++;
                throw new Error('legacy preactHooks global should not be read');
            }
        });

        globalThis.__officialPreactLegacyImportOk = false;

        true;
    )"));

    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import * as preactModule from 'preact';
        import * as hooksModule from 'preact/hooks';
        import * as jsxRuntimeModule from 'preact/jsx-runtime';
        import * as jsxDevRuntimeModule from 'preact/jsx-dev-runtime';

        globalThis.__officialPreactLegacyImportOk =
            typeof preactModule.render === 'function' &&
            typeof hooksModule.useState === 'function' &&
            typeof jsxRuntimeModule.jsx === 'function' &&
            typeof jsxDevRuntimeModule.jsxDEV === 'function' &&
            globalThis.__officialPreactLegacyGlobalReads === 0;
    )", "<official-preact-no-legacy-globals-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactLegacyImportOk"), true);
    EXPECT_FALSE(legacy_preact_requested_);
    EXPECT_FALSE(legacy_hooks_requested_);
}

TEST_F(DOMBindingsTest, OfficialPreactAutomaticRuntimeCanCreateVNode) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { jsx } from 'preact/jsx-runtime';

        const vnode = jsx('div', { children: 'hello' });
        globalThis.__officialPreactAutomaticRuntimeVNodeOk =
            !!vnode &&
            vnode.type === 'div' &&
            !!vnode.props &&
            vnode.props.children === 'hello';
    )", "<official-preact-automatic-runtime-vnode-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactAutomaticRuntimeVNodeOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactAutomaticRuntimeCanRenderSimpleDiv) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('div', { children: 'hello' }), document.body);

        const first = document.body.firstChild;
        globalThis.__officialPreactAutomaticRuntimeRenderOk =
            !!first &&
            first.nodeType === 1 &&
            first.tagName.toLowerCase() === 'div' &&
            !!first.firstChild &&
            first.firstChild.nodeType === 3 &&
            first.firstChild.nodeValue === 'hello';
    )", "<official-preact-automatic-runtime-render-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactAutomaticRuntimeRenderOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanUpdateExistingTextNode) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('div', { children: 'hello' }), document.body);
        render(jsx('div', { children: 'world' }), document.body);

        const first = document.body.firstChild;
        globalThis.__officialPreactRenderTextUpdateOk =
            document.body.childNodes.length === 1 &&
            !!first &&
            first.nodeType === 1 &&
            first.tagName.toLowerCase() === 'div' &&
            first.childNodes.length === 1 &&
            !!first.firstChild &&
            first.firstChild.nodeType === 3 &&
            first.firstChild.nodeValue === 'world';
    )", "<official-preact-render-text-update-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactRenderTextUpdateOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactHooksUseEffectDoesNotBreakRender) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { useEffect } from 'preact/hooks';
        import { jsx } from 'preact/jsx-runtime';

        function App() {
            useEffect(() => {
                globalThis.__officialPreactUseEffectRan = true;
            }, []);
            return jsx('div', { children: 'effect-ok' });
        }

        document.body.textContent = '';
        globalThis.__officialPreactUseEffectRan = false;
        render(jsx(App, {}), document.body);

        const first = document.body.firstChild;
        globalThis.__officialPreactUseEffectRenderOk =
            !!first &&
            first.nodeType === 1 &&
            first.tagName.toLowerCase() === 'div' &&
            first.textContent === 'effect-ok';
    )", "<official-preact-hooks-useeffect-render-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactUseEffectRenderOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactDelegatedInputAndClickCanUpdateState) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { h, render } from 'preact';
        import { useState } from 'preact/hooks';

        function App() {
            const [value, setValue] = useState('');
            const [count, setCount] = useState(0);
            return h('div', {}, [
                h('input', { id: 'official-input', value, onInput: (event) => setValue(event.target.value) }),
                h('button', { id: 'official-button', onClick: () => setCount((current) => current + 1) }, 'Count:' + count),
                h('span', { id: 'official-state' }, value + '|' + count)
            ]);
        }

        document.body.textContent = '';
        render(h(App), document.body);

        const input = document.getElementById('official-input');
        const button = document.getElementById('official-button');
        input.value = 'hello';
        input.dispatchEvent(new Event('input', { bubbles: true, cancelable: true }));
        button.click();
    )", "<official-preact-delegated-events-test>"));

    runtime_->RunEventLoop(4);
    runtime_->ProcessMicrotasks();

    EXPECT_EQ(runtime_->Eval(R"(
        (() => {
            const state = document.getElementById('official-state');
            const button = document.getElementById('official-button');
            return !!state && state.textContent === 'hello|1' && button && button.textContent === 'Count:1';
        })()
    )"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactUseEffectCanFlushAfterAnimationFrameAndTimeout) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { h, render } from 'preact';
        import { useEffect } from 'preact/hooks';

        function App() {
            useEffect(() => {
                globalThis.__officialUseEffectTick = 'effect-ran';
            }, []);
            return h('div', { id: 'effect-app' }, 'effect-app');
        }

        globalThis.__officialUseEffectTick = 'pending';
        document.body.textContent = '';
        render(h(App), document.body);
    )", "<official-preact-useeffect-runtime-test>"));

    ASSERT_TRUE(task_scheduler_ != nullptr);
    task_scheduler_->ProcessAnimationFrames(16.0);
    task_scheduler_->ProcessTasks();
    runtime_->ProcessMicrotasks();

    EXPECT_EQ(runtime_->Eval("globalThis.__officialUseEffectTick"), "effect-ran");
}


TEST_F(DOMBindingsTest, OfficialPreactRenderCanMountMinimalSvgTree) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx, jsxs } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(
            jsx('svg', {
                viewBox: '0 0 10 10',
                children: jsxs('g', {
                    children: [
                        jsx('title', { children: 'chart' }),
                        jsx('polyline', { strokeWidth: '2', points: '0,0 10,10' })
                    ]
                })
            }),
            document.body
        );

        const svg = document.body.firstChild;
        const group = svg && svg.firstChild;
        const title = group && group.firstChild;
        const polyline = title && title.nextSibling;
        globalThis.__officialPreactSvgRenderOk =
            document.body.childNodes.length === 1 &&
            !!svg &&
            svg.namespaceURI === 'http://www.w3.org/2000/svg' &&
            svg.localName === 'svg' &&
            svg.getAttribute('viewBox') === '0 0 10 10' &&
            !!group &&
            group.namespaceURI === 'http://www.w3.org/2000/svg' &&
            group.localName === 'g' &&
            !!title &&
            title.localName === 'title' &&
            title.textContent === 'chart' &&
            !!polyline &&
            polyline.localName === 'polyline' &&
            polyline.getAttribute('stroke-width') === '2' &&
            polyline.getAttribute('points') === '0,0 10,10';
    )", "<official-preact-svg-render-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactSvgRenderOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanReorderKeyedChildren) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx, jsxs } from 'preact/jsx-runtime';

        const view = items => jsxs('div', {
            children: items.map(item => jsx('span', { children: item.label }, item.key))
        });

        document.body.textContent = '';
        render(view([
            { key: 'a', label: 'A' },
            { key: 'b', label: 'B' }
        ]), document.body);

        render(view([
            { key: 'b', label: 'B' },
            { key: 'a', label: 'A' }
        ]), document.body);

        const root = document.body.firstChild;
        const first = root && root.firstChild;
        const second = first && first.nextSibling;
        globalThis.__officialPreactKeyedReorderOk =
            document.body.childNodes.length === 1 &&
            !!root &&
            root.childNodes.length === 2 &&
            !!first &&
            !!second &&
            first.textContent === 'B' &&
            second.textContent === 'A';
    )", "<official-preact-keyed-reorder-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactKeyedReorderOk"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanControlSelectValue) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx, jsxs } from 'preact/jsx-runtime';

        const view = value => jsxs('select', {
            value,
            children: [
                jsx('option', { value: 'a', children: 'Alpha' }),
                jsx('option', { value: 'b', children: 'Beta' })
            ]
        });

        document.body.textContent = '';
        render(view('b'), document.body);
    )", "<official-preact-select-value-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "select");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "b");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.firstChild && document.body.firstChild.firstChild.getAttribute('value')"), "a");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.firstChild && document.body.firstChild.firstChild.nextSibling && document.body.firstChild.firstChild.nextSibling.getAttribute('value')"), "b");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanControlCheckboxChecked) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        const view = checked => jsx('input', {
            type: 'checkbox',
            checked
        });

        document.body.textContent = '';
        render(view(true), document.body);
        render(view(false), document.body);
    )", "<official-preact-checkbox-checked-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "input");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.checked"), false);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanUpdateStyleObject) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        const view = style => jsx('div', { style });

        document.body.textContent = '';
        render(view({ color: 'red', backgroundColor: 'blue' }), document.body);
        render(view({ color: 'green' }), document.body);
    )", "<official-preact-style-object-update-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.style.color"), "green");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.style.backgroundColor"), "");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanUpdateOptionSelected) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx, jsxs } from 'preact/jsx-runtime';

        const view = selected => jsxs('select', {
            children: [
                jsx('option', { value: 'a', selected, children: 'Alpha' }),
                jsx('option', { value: 'b', children: 'Beta' })
            ]
        });

        document.body.textContent = '';
        render(view(true), document.body);
        render(view(false), document.body);
    )", "<official-preact-option-selected-update-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanControlTextareaValueAfterTextChild) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('textarea', { children: 'hello' }), document.body);
        render(jsx('textarea', { value: 'world' }), document.body);
    )", "<official-preact-textarea-value-after-children-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "textarea");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "world");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanApplyTextareaDefaultValue) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('textarea', { defaultValue: 'seed' }), document.body);
    )", "<official-preact-textarea-defaultvalue-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "textarea");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "seed");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanApplyInputDefaultChecked) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('input', { type: 'checkbox', defaultChecked: true }), document.body);
    )", "<official-preact-input-defaultchecked-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "input");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.checked"), true);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanApplyInputDefaultValue) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('input', { defaultValue: 'seed' }), document.body);
    )", "<official-preact-input-defaultvalue-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.tagName.toLowerCase()"), "input");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "seed");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanUpdateInputDefaultChecked) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        const view = checked => jsx('input', { type: 'checkbox', defaultChecked: checked });

        document.body.textContent = '';
        render(view(true), document.body);
        render(view(false), document.body);
    )", "<official-preact-input-defaultchecked-update-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.checked"), false);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanUpdateOptionDefaultSelected) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx, jsxs } from 'preact/jsx-runtime';

        const view = selected => jsxs('select', {
            children: [
                jsx('option', { value: 'a', defaultSelected: selected, children: 'Alpha' }),
                jsx('option', { value: 'b', children: 'Beta' })
            ]
        });

        document.body.textContent = '';
        render(view(true), document.body);
        render(view(false), document.body);
    )", "<official-preact-option-defaultselected-update-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.value"), "");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanClearStyleString) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        const view = style => jsx('div', { style });

        document.body.textContent = '';
        render(view('color: red; background-color: blue;'), document.body);
        render(view(null), document.body);
    )", "<official-preact-style-string-clear-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.style.color"), "");
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.style.backgroundColor"), "");
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanRemoveCheckedProp) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('input', { type: 'checkbox', checked: true }), document.body);
        render(jsx('input', { type: 'checkbox' }), document.body);
    )", "<official-preact-remove-checked-prop-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.checked"), false);
}

TEST_F(DOMBindingsTest, OfficialPreactRenderCanRemoveClassName) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.textContent = '';
        render(jsx('div', { className: 'alpha beta' }), document.body);
        render(jsx('div', {}), document.body);
    )", "<official-preact-remove-classname-test>"));

    EXPECT_EQ(runtime_->Eval("document.body.childNodes.length"), 1);
    EXPECT_EQ(runtime_->Eval("document.body.firstChild && document.body.firstChild.className"), "");
}


















TEST_F(DOMBindingsTest, OfficialPreactHydrateMinimalUseCaseWorks) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { hydrate } from 'preact';
        import { jsx } from 'preact/jsx-runtime';

        document.body.innerHTML = '<div id="app"><span>hello</span></div>';

        hydrate(
            jsx('div', { id: 'app', children: jsx('span', { children: 'hello' }) }),
            document.body
        );

        const app = document.body.firstChild;
        globalThis.__officialPreactHydrateOk =
            !!app &&
            app.nodeType === 1 &&
            app.tagName.toLowerCase() === 'div' &&
            app.id === 'app' &&
            document.body.childNodes.length === 1 &&
            app.childNodes.length === 1 &&
            !!app.firstChild &&
            app.firstChild.nodeType === 1 &&
            app.firstChild.tagName.toLowerCase() === 'span' &&
            app.firstChild.textContent === 'hello';
    )", "<official-preact-hydrate-minimal-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactHydrateOk"), true);
}

TEST_F(DOMBindingsTest, HydrateMarkersAreVisibleAndTraversableLikeOfficialPreactExpects) {
    auto result = runtime_->Eval(R"(
        document.body.textContent = '';
        var root = document.createElement('div');
        var open = document.createComment('$s');
        var span = document.createElement('span');
        var close = document.createComment('/$s');
        var tail = document.createElement('p');
        span.textContent = 'hello';
        tail.textContent = 'tail';
        root.appendChild(open);
        root.appendChild(span);
        root.appendChild(close);
        root.appendChild(tail);
        document.body.appendChild(root);

        var excess = Array.prototype.slice.call(root.childNodes);
        var markers = [];
        var current = root.firstChild;
        while (current) {
            if (current.nodeType === 8) {
                markers.push(current.data);
            }
            current = current.nextSibling;
        }

        open &&
        open.nodeType === 8 &&
        open.data === '$s' &&
        span &&
        span.nodeType === 1 &&
        span.textContent === 'hello' &&
        close &&
        close.nodeType === 8 &&
        close.data === '/$s' &&
        tail &&
        tail.nodeType === 1 &&
        tail.tagName.toLowerCase() === 'p' &&
        excess.length === 4 &&
        excess[0] === open &&
        excess[1] === span &&
        excess[2] === close &&
        excess[3] === tail &&
        markers.length === 2 &&
        markers[0] === '$s' &&
        markers[1] === '/$s';
    )");

    EXPECT_EQ(result, true);
}


TEST_F(DOMBindingsTest, OfficialPreactJsxDevRuntimeCanCreateVNode) {
    EXPECT_NO_THROW(runtime_->EvalModule(R"(
        import { render } from 'preact';
        import { jsxDEV } from 'preact/jsx-dev-runtime';

        const vnode = jsxDEV('div', { children: 'hello-dev' }, undefined, false, undefined, undefined);
        document.body.textContent = '';
        render(vnode, document.body);

        const first = document.body.firstChild;
        globalThis.__officialPreactJsxDevRuntimeVNodeOk =
            !!vnode &&
            vnode.type === 'div' &&
            !!vnode.props &&
            vnode.props.children === 'hello-dev' &&
            !!first &&
            first.nodeType === 1 &&
            first.tagName.toLowerCase() === 'div' &&
            !!first.firstChild &&
            first.firstChild.nodeType === 3 &&
            first.firstChild.nodeValue === 'hello-dev';
    )", "<official-preact-jsx-dev-runtime-vnode-test>"));

    EXPECT_EQ(runtime_->Eval("globalThis.__officialPreactJsxDevRuntimeVNodeOk"), true);
}

TEST_F(DOMBindingsTest, HostNodesExposeCommentAndDataProperties) {
    auto result = runtime_->Eval(R"(
        var text = document.createTextNode('hello');
        var comment = document.createComment('note');
        var beforeText = text.data === 'hello' && text.nodeValue === 'hello';
        var beforeComment =
            comment.nodeType === 8 &&
            comment.nodeName === '#comment' &&
            comment.data === 'note' &&
            comment.nodeValue === 'note';

        text.data = 'world';
        comment.data = 'changed';

        beforeText &&
        beforeComment &&
        text.data === 'world' &&
        text.nodeValue === 'world' &&
        comment.data === 'changed' &&
        comment.nodeValue === 'changed';
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, HostElementsExposeLocalNameAndNamespaceURI) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        var math = document.createElementNS('http://www.w3.org/1998/Math/MathML', 'math');

        div.localName === 'div' &&
        div.namespaceURI === 'http://www.w3.org/1999/xhtml' &&
        svg.localName === 'svg' &&
        svg.namespaceURI === 'http://www.w3.org/2000/svg' &&
        math.localName === 'math' &&
        math.namespaceURI === 'http://www.w3.org/1998/Math/MathML';
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, TemplateElementExposesContentFragment) {
    auto result = runtime_->Eval(R"(
        var template = document.createElement('template');
        template.innerHTML = '<span>hello</span><!--x-->world';

        template.childNodes.length === 0 &&
        template.content &&
        template.content.nodeType === 11 &&
        template.content.childNodes.length === 3 &&
        template.content.firstChild.tagName === 'span' &&
        template.content.childNodes[1].nodeType === 8 &&
        template.content.lastChild.data === 'world';
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, TemplateCloneNodeDeepClonesContent) {
    auto result = runtime_->Eval(R"(
        var template = document.createElement('template');
        template.innerHTML = '<div>hello</div><!--ok-->';
        var clone = template.cloneNode(true);

        clone !== template &&
        clone.tagName === 'template' &&
        clone.content &&
        clone.content !== template.content &&
        clone.content.childNodes.length === 2 &&
        clone.content.firstChild.tagName === 'div' &&
        clone.content.firstChild.textContent === 'hello' &&
        clone.content.childNodes[1].nodeType === 8 &&
        clone.innerHTML === '<div>hello</div><!--ok-->';
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, TemplateContentCloneNodeDeepClonesFragmentChildren) {
    auto result = runtime_->Eval(R"(
        var template = document.createElement('template');
        template.innerHTML = '<span>hello</span><!--ok-->tail';

        var clone = template.content.cloneNode(true);

        clone !== template.content &&
        clone.nodeType === 11 &&
        clone.childNodes.length === 3 &&
        clone.firstChild !== template.content.firstChild &&
        clone.firstChild.tagName === 'span' &&
        clone.firstChild.textContent === 'hello' &&
        clone.firstChild.nextSibling.nodeType === 8 &&
        clone.firstChild.nextSibling.data === 'ok' &&
        clone.childNodes[2].nodeType === 3 &&
        clone.childNodes[2].data === 'tail';
    )");

    EXPECT_EQ(result, true);
}


TEST_F(DOMBindingsTest, IsConnectedReflectsTreeAttachment) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        var text = document.createTextNode('hello');
        child.appendChild(text);
        parent.appendChild(child);

        var detachedOk =
            parent.isConnected === false &&
            child.isConnected === false &&
            text.isConnected === false;

        document.body.appendChild(parent);

        var attachedOk =
            parent.isConnected === true &&
            child.isConnected === true &&
            text.isConnected === true;

        parent.remove();

        detachedOk &&
        attachedOk &&
        parent.isConnected === false &&
        child.isConnected === false &&
        text.isConnected === false;
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, CompareDocumentPositionReflectsTreeRelationships) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var first = document.createElement('span');
        var second = document.createElement('span');
        var detached = document.createElement('p');

        parent.appendChild(first);
        parent.appendChild(second);
        document.body.appendChild(parent);

        var sameNodeOk = parent.compareDocumentPosition(parent) === 0;
        var parentChildOk = parent.compareDocumentPosition(first) === 20;
        var childParentOk = first.compareDocumentPosition(parent) === 10;
        var siblingOrderOk =
            first.compareDocumentPosition(second) === 4 &&
            second.compareDocumentPosition(first) === 2;
        var disconnectedOk = parent.compareDocumentPosition(detached) === 35;

        parent.remove();

        sameNodeOk &&
        parentChildOk &&
        childParentOk &&
        siblingOrderOk &&
        disconnectedOk;
    )");

    EXPECT_EQ(result, true);
}


TEST_F(DOMBindingsTest, AddEventListener) {
    auto result = runtime_->Eval(R"(
        var clicked = false;
        var div = document.createElement('div');
        div.addEventListener('click', function() {
            clicked = true;
        });
        // 模拟点击
        var event = new Event('click');
        div.dispatchEvent(event);
        clicked;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, EventListenerThisMatchesCurrentTarget) {
    auto result = runtime_->Eval(R"(
        var observed = false;
        var button = document.createElement('button');
        button.addEventListener('click', function(event) {
            observed = (this === button) && (event.currentTarget === button) && (event.target === button);
        });
        button.click();
        observed;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, RemoveEventListener) {
    auto result = runtime_->Eval(R"(
        var count = 0;
        var div = document.createElement('div');
        var handler = function() { count++; };
        div.addEventListener('click', handler);
        div.removeEventListener('click', handler);
        div.dispatchEvent(new Event('click'));
        count;
    )");
    EXPECT_EQ(result, 0);
}

TEST_F(DOMBindingsTest, ElementClickDispatchesEvent) {
    auto result = runtime_->Eval(R"(
        var clicked = false;
        var button = document.createElement('button');
        button.addEventListener('click', function() {
            clicked = true;
        });
        button.click();
        clicked;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, ElementClickDoesNotDispatchForDisabledButton) {
    auto result = runtime_->Eval(R"(
        var clicks = 0;
        var button = document.createElement('button');
        button.setAttribute('disabled', 'true');
        button.addEventListener('click', function() {
            clicks++;
        });
        button.click();
        clicks;
    )");
    EXPECT_EQ(result, 0);
}

TEST_F(DOMBindingsTest, FileInputClickPopulatesFileListThroughPickerHook) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-input-test";
    std::filesystem::create_directories(temp_dir);
    const auto first_path = temp_dir / "picked.txt";
    const auto second_path = temp_dir / "picked.json";
    {
        std::ofstream(first_path) << "abc";
        std::ofstream(second_path) << "{}";
    }

    const std::string first = FsPathToUtf8String(first_path);
    const std::string second = FsPathToUtf8String(second_path);
    HTMLInputElement::SetFilePickerForTesting([first, second](HTMLInputElement& input) {
        input.SetFilesFromPaths({first, second}, true);
    });

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.setAttribute('multiple', '');
        var inputEvents = 0;
        var changeEvents = 0;
        input.addEventListener('input', function() { inputEvents++; });
        input.addEventListener('change', function() { changeEvents++; });
        input.click();
        var files = input.files;
        files &&
          files.length === 2 &&
          files[0].name === 'picked.txt' &&
          files.item(1).name === 'picked.json' &&
          files.item(2) === null &&
          files[0].size === 3 &&
          files[0].type === 'text/plain' &&
          inputEvents === 1 &&
          changeEvents === 1;
    )");
    EXPECT_EQ(result, true);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, FileInputClickDefaultCanBePrevented) {
    HTMLInputElement::SetFilePickerForTesting([](HTMLInputElement& input) {
        input.SetFilesFromPaths({"should-not-be-picked.txt"}, true);
    });

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.addEventListener('click', function(event) {
            event.preventDefault();
        });
        input.click();
        input.files.length === 0;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, FileInputFilesAreReadonlySnapshotsWithBrowserTags) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-input-tags-test";
    std::filesystem::create_directories(temp_dir);
    const auto first_path = temp_dir / "snapshot.txt";
    {
        std::ofstream(first_path) << "abc";
    }

    const std::string first = FsPathToUtf8String(first_path);
    const std::string expected_normalized_path = NormalizeModulePath(first_path);
    HTMLInputElement::SetFilePickerForTesting([first](HTMLInputElement& input) {
        input.SetFilesFromPaths({first}, true);
    });

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.click();
        var firstSnapshot = input.files;
        var secondSnapshot = input.files;
        var file = firstSnapshot[0];
        var lengthDescriptor = Object.getOwnPropertyDescriptor(firstSnapshot, 'length');
        var indexDescriptor = Object.getOwnPropertyDescriptor(firstSnapshot, '0');
        var tagDescriptor = Object.getOwnPropertyDescriptor(firstSnapshot, Symbol.toStringTag);
        var originalName = file.name;
        firstSnapshot.length = 99;
        firstSnapshot[0] = { name: 'mutated' };
        file.name = 'mutated';
        ({
          snapshotDistinct: firstSnapshot !== secondSnapshot,
          value: input.value,
          listTag: Object.prototype.toString.call(firstSnapshot),
          fileTag: Object.prototype.toString.call(file),
          lengthWritable: lengthDescriptor.writable,
          lengthConfigurable: lengthDescriptor.configurable,
          indexWritable: indexDescriptor.writable,
          indexConfigurable: indexDescriptor.configurable,
          tagWritable: tagDescriptor.writable,
          tagConfigurable: tagDescriptor.configurable,
          lengthAfterWrite: input.files.length,
          nameAfterWrite: input.files[0].name,
          originalName: originalName,
          relativePath: input.files[0].webkitRelativePath,
          normalizedPath: input.files[0].normalizedPath,
          pathIsUndefined: typeof input.files[0].path === 'undefined',
          isDirectoryIsUndefined: typeof input.files[0].isDirectory === 'undefined',
          missing: input.files.item(4),
          nonFileFiles: document.createElement('div').files
        });
    )");

    EXPECT_EQ(result["snapshotDistinct"], true);
    EXPECT_EQ(result["value"], "C:\\fakepath\\snapshot.txt");
    EXPECT_EQ(result["listTag"], "[object FileList]");
    EXPECT_EQ(result["fileTag"], "[object File]");
    EXPECT_EQ(result["lengthWritable"], false);
    EXPECT_EQ(result["lengthConfigurable"], false);
    EXPECT_EQ(result["indexWritable"], false);
    EXPECT_EQ(result["indexConfigurable"], false);
    EXPECT_EQ(result["tagWritable"], false);
    EXPECT_EQ(result["tagConfigurable"], false);
    EXPECT_EQ(result["lengthAfterWrite"], 1);
    EXPECT_EQ(result["nameAfterWrite"], "snapshot.txt");
    EXPECT_EQ(result["originalName"], "snapshot.txt");
    EXPECT_EQ(result["relativePath"], "");
    EXPECT_EQ(result["normalizedPath"], expected_normalized_path);
    EXPECT_EQ(result["pathIsUndefined"], true);
    EXPECT_EQ(result["isDirectoryIsUndefined"], true);
    EXPECT_TRUE(result["missing"].is_null());
    EXPECT_TRUE(result["nonFileFiles"].is_null());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, FileInputDefaultValueDoesNotAliasLiveSelection) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-default-value-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "picked.txt";
    {
        std::ofstream(file_path) << "abc";
    }

    const std::string picked = FsPathToUtf8String(file_path);
    HTMLInputElement::SetFilePickerForTesting([picked](HTMLInputElement& input) {
        input.SetFilesFromPaths({picked}, true);
    });

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.defaultValue = 'C:/fake/path.txt';
        var before = {
          value: input.value,
          defaultValue: input.defaultValue,
          files: input.files.length
        };
        input.click();
        var picked = {
          value: input.value,
          defaultValue: input.defaultValue,
          files: input.files.length,
          name: input.files[0].name
        };
        input.defaultValue = '';
        ({
          beforeValue: before.value,
          beforeDefaultValue: before.defaultValue,
          beforeFiles: before.files,
          pickedValue: picked.value,
          pickedDefaultValue: picked.defaultValue,
          pickedFiles: picked.files,
          pickedName: picked.name,
          afterClearValue: input.value,
          afterClearDefaultValue: input.defaultValue,
          afterClearFiles: input.files.length
        });
    )");

    EXPECT_EQ(result["beforeValue"], "");
    EXPECT_EQ(result["beforeDefaultValue"], "C:/fake/path.txt");
    EXPECT_EQ(result["beforeFiles"], 0);
    EXPECT_EQ(result["pickedValue"], "C:\\fakepath\\picked.txt");
    EXPECT_EQ(result["pickedDefaultValue"], "C:/fake/path.txt");
    EXPECT_EQ(result["pickedFiles"], 1);
    EXPECT_EQ(result["pickedName"], "picked.txt");
    EXPECT_EQ(result["afterClearValue"], "");
    EXPECT_EQ(result["afterClearDefaultValue"], "");
    EXPECT_EQ(result["afterClearFiles"], 0);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, FileInputDirectorySelectionExposesRelativeFilePaths) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-directory-test";
    const auto directory = temp_dir / "folder";
    const auto nested = directory / "nested";
    std::filesystem::create_directories(nested);
    const auto file_path = nested / "child.txt";
    {
        std::ofstream(file_path) << "abc";
    }

    const std::string picked_directory = FsPathToUtf8String(directory);
    HTMLInputElement::SetFilePickerForTesting([picked_directory](HTMLInputElement& input) {
        input.SetFilesFromPaths({picked_directory}, true);
    });

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.setAttribute('webkitdirectory', '');
        input.click();
        ({
          length: input.files.length,
          value: input.value,
          name: input.files[0].name,
          relativePath: input.files[0].webkitRelativePath,
          pathIsUndefined: typeof input.files[0].path === 'undefined',
          isDirectoryIsUndefined: typeof input.files[0].isDirectory === 'undefined'
        });
    )");

    EXPECT_EQ(result["length"], 1);
    EXPECT_EQ(result["value"], "C:\\fakepath\\child.txt");
    EXPECT_EQ(result["name"], "child.txt");
    EXPECT_EQ(result["relativePath"], "folder/nested/child.txt");
    EXPECT_EQ(result["pathIsUndefined"], true);
    EXPECT_EQ(result["isDirectoryIsUndefined"], true);

    HTMLInputElement::SetFilePickerForTesting(nullptr);
    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, DataTransferFilesExposeFileListToJavaScript) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-data-transfer-files-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "dragged.txt";
    {
        std::ofstream(file_path) << "drop";
    }

    auto transfer = std::make_shared<DataTransfer>();
    const std::string expected_normalized_path = NormalizeModulePath(file_path);
    transfer->SetData("text/plain", "payload");
    transfer->SetFilesFromPaths({FsPathToUtf8String(file_path)});

    JSContext* ctx = runtime_->GetContext();
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "__testTransfer", bindings::WrapDataTransfer(ctx, transfer));
    JS_FreeValue(ctx, global);

    auto result = runtime_->Eval(R"(
        var files = __testTransfer.files;
        var secondSnapshot = __testTransfer.files;
        var lengthDescriptor = Object.getOwnPropertyDescriptor(files, 'length');
        var indexDescriptor = Object.getOwnPropertyDescriptor(files, '0');
        files.length = 99;
        files[0] = { name: 'mutated' };
        files[0].name = 'mutated';
        ({
          snapshotDistinct: files !== secondSnapshot,
          typeCount: __testTransfer.types.length,
          firstType: __testTransfer.types[0],
          secondType: __testTransfer.types[1],
          listTag: Object.prototype.toString.call(files),
          fileTag: Object.prototype.toString.call(files[0]),
          lengthWritable: lengthDescriptor.writable,
          lengthConfigurable: lengthDescriptor.configurable,
          indexWritable: indexDescriptor.writable,
          indexConfigurable: indexDescriptor.configurable,
          length: files.length,
          freshLength: __testTransfer.files.length,
          name: __testTransfer.files[0].name,
          normalizedPath: __testTransfer.files[0].normalizedPath,
          pathIsUndefined: typeof __testTransfer.files[0].path === 'undefined',
          text: __testTransfer.getData('text/plain'),
          missing: files.item(3)
        });
    )");

    EXPECT_EQ(result["snapshotDistinct"], true);
    EXPECT_EQ(result["typeCount"], 2);
    EXPECT_EQ(result["firstType"], "text/plain");
    EXPECT_EQ(result["secondType"], "Files");
    EXPECT_EQ(result["listTag"], "[object FileList]");
    EXPECT_EQ(result["fileTag"], "[object File]");
    EXPECT_EQ(result["lengthWritable"], false);
    EXPECT_EQ(result["lengthConfigurable"], false);
    EXPECT_EQ(result["indexWritable"], false);
    EXPECT_EQ(result["indexConfigurable"], false);
    EXPECT_EQ(result["length"], 1);
    EXPECT_EQ(result["freshLength"], 1);
    EXPECT_EQ(result["name"], "dragged.txt");
    EXPECT_EQ(result["normalizedPath"], expected_normalized_path);
    EXPECT_EQ(result["pathIsUndefined"], true);
    EXPECT_EQ(result["text"], "payload");
    EXPECT_TRUE(result["missing"].is_null());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, FileInputFilesSetterAcceptsDataTransferFileList) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-input-setter-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "dragged.txt";
    {
        std::ofstream(file_path) << "drop";
    }

    auto transfer = std::make_shared<DataTransfer>();
    transfer->SetFilesFromPaths({FsPathToUtf8String(file_path)});

    JSContext* ctx = runtime_->GetContext();
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "__testTransfer", bindings::WrapDataTransfer(ctx, transfer));
    JS_FreeValue(ctx, global);

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.files = __testTransfer.files;
        var afterSet = {
          length: input.files.length,
          value: input.value,
          name: input.files[0].name
        };
        input.files = null;
        ({
          length: afterSet.length,
          value: afterSet.value,
          name: afterSet.name,
          clearedLength: input.files.length,
          clearedValue: input.value
        });
    )");

    EXPECT_EQ(result["length"], 1);
    EXPECT_EQ(result["value"], "C:\\fakepath\\dragged.txt");
    EXPECT_EQ(result["name"], "dragged.txt");
    EXPECT_EQ(result["clearedLength"], 0);
    EXPECT_EQ(result["clearedValue"], "");

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, FileInputFilesSetterKeepsDirectoryFilesWithoutMultiple) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mbink-js-file-input-directory-setter-test";
    const auto directory = temp_dir / "folder";
    const auto nested = directory / "nested";
    const auto first_path = directory / "a.txt";
    const auto second_path = nested / "b.txt";
    std::filesystem::create_directories(nested);
    {
        std::ofstream(first_path) << "a";
        std::ofstream(second_path) << "bb";
    }

    auto transfer = std::make_shared<DataTransfer>();
    transfer->SetFilesFromPaths({FsPathToUtf8String(directory)});

    JSContext* ctx = runtime_->GetContext();
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "__testTransfer", bindings::WrapDataTransfer(ctx, transfer));
    JS_FreeValue(ctx, global);

    auto result = runtime_->Eval(R"(
        var input = document.createElement('input');
        input.setAttribute('type', 'file');
        input.setAttribute('webkitdirectory', '');
        input.files = __testTransfer.files;
        ({
          transferLength: __testTransfer.files.length,
          inputLength: input.files.length,
          firstName: input.files[0].name,
          firstRelativePath: input.files[0].webkitRelativePath,
          secondName: input.files[1].name,
          secondRelativePath: input.files[1].webkitRelativePath,
          value: input.value
        });
    )");

    EXPECT_EQ(result["transferLength"], 2);
    EXPECT_EQ(result["inputLength"], 2);
    EXPECT_EQ(result["firstName"], "a.txt");
    EXPECT_EQ(result["firstRelativePath"], "folder/a.txt");
    EXPECT_EQ(result["secondName"], "b.txt");
    EXPECT_EQ(result["secondRelativePath"], "folder/nested/b.txt");
    EXPECT_EQ(result["value"], "C:\\fakepath\\a.txt");

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(DOMBindingsTest, EventTimeStampAndStopImmediatePropagation) {
    auto result = runtime_->Eval(R"(
        var calls = 0;
        var tsOk = false;
        var div = document.createElement('div');
        div.addEventListener('click', function(event) {
            calls++;
            tsOk = typeof event.timeStamp === 'number' && event.timeStamp >= 0;
            event.stopImmediatePropagation();
        });
        div.addEventListener('click', function() {
            calls++;
        });
        div.click();
        tsOk && calls === 1;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentCreateEventInitEventAndDispatchWorks) {
    auto result = runtime_->Eval(R"(
        var observedType = '';
        var observedPhase = 0;
        var observedBubbles = false;
        var observedCancelable = false;
        document.body.addEventListener('phase5-doc-create', function(event) {
            observedType = event.type;
            observedPhase = event.eventPhase;
            observedBubbles = event.bubbles;
            observedCancelable = event.cancelable;
        });
        var event = document.createEvent('Event');
        event.initEvent('phase5-doc-create', true, true);
        document.dispatchEvent(event);
        observedType === 'phase5-doc-create' && observedPhase === 2 && observedBubbles && observedCancelable;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, EventCaptureTargetAndBubbleOrderWorks) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('button');
        parent.appendChild(child);
        document.body.appendChild(parent);

        var order = [];
        parent.addEventListener('phase5-flow', function(event) {
            order.push('parent-capture:' + event.eventPhase + ':' + (event.currentTarget === parent) + ':' + (event.target === child));
        }, true);
        child.addEventListener('phase5-flow', function(event) {
            order.push('target-capture:' + event.eventPhase + ':' + (event.currentTarget === child) + ':' + (event.target === child));
        }, true);
        child.addEventListener('phase5-flow', function(event) {
            order.push('target-bubble:' + event.eventPhase + ':' + (event.currentTarget === child) + ':' + (event.target === child));
        });
        parent.addEventListener('phase5-flow', function(event) {
            order.push('parent-bubble:' + event.eventPhase + ':' + (event.currentTarget === parent) + ':' + (event.target === child));
        });

        var event = document.createEvent('Event');
        event.initEvent('phase5-flow', true, true);
        child.dispatchEvent(event);

        order.length === 4 &&
        order[0] === 'parent-capture:1:true:true' &&
        order[1] === 'target-capture:2:true:true' &&
        order[2] === 'target-bubble:2:true:true' &&
        order[3] === 'parent-bubble:3:true:true';
    )");

    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentDispatchEventDelegatesToBody) {
    auto result = runtime_->Eval(R"(
        var calls = 0;
        document.body.addEventListener('phase5-doc-dispatch', function(event) {
            calls++;
        });
        var event = document.createEvent('Event');
        event.initEvent('phase5-doc-dispatch', false, false);
        document.dispatchEvent(event);
        calls === 1;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, WindowEventListenerDelegatesToDocument) {
    auto result = runtime_->Eval(R"(
        var calls = 0;
        window.addEventListener('phase5-window', function(event) {
            calls++;
        });
        var event = document.createEvent('Event');
        event.initEvent('phase5-window', false, false);
        window.dispatchEvent(event);
        calls === 1;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, QueueMicrotaskRunsAfterProcessMicrotasks) {
    runtime_->Eval(R"(
        globalThis.__phase5MicrotaskFlag = false;
        queueMicrotask(function() {
            globalThis.__phase5MicrotaskFlag = true;
        });
    )");

    EXPECT_EQ(runtime_->Eval("globalThis.__phase5MicrotaskFlag"), false);
    runtime_->ProcessMicrotasks();
    EXPECT_EQ(runtime_->Eval("globalThis.__phase5MicrotaskFlag"), true);
}

TEST_F(DOMBindingsTest, PerformanceNowIsNumberAndMonotonic) {
    auto result = runtime_->Eval(R"(
        var a = performance.now();
        var b = performance.now();
        typeof a === 'number' && typeof b === 'number' && b >= a;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, CustomEventConstructorExposesDetail) {
    auto result = runtime_->Eval(R"(
        var event = new CustomEvent('phase5-custom-ctor', {
            detail: { value: 42, label: 'ok' },
            bubbles: true,
            cancelable: true
        });
        event.type === 'phase5-custom-ctor'
            && event.bubbles === true
            && event.cancelable === true
            && event.detail
            && event.detail.value === 42
            && event.detail.label === 'ok';
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, CustomEventInitCustomEventWorks) {
    auto result = runtime_->Eval(R"(
        var event = document.createEvent('CustomEvent');
        event.initCustomEvent('phase5-custom-init', true, false, { nested: { count: 2 } });
        event.type === 'phase5-custom-init'
            && event.bubbles === true
            && event.cancelable === false
            && event.detail
            && event.detail.nested
            && event.detail.nested.count === 2;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, DocumentCreateCustomEventAndDispatchWorks) {
    auto result = runtime_->Eval(R"(
        var observed = null;
        document.body.addEventListener('phase5-custom-dispatch', function(event) {
            observed = event.detail && event.detail.payload;
        });
        var event = document.createEvent('CustomEvent');
        event.initCustomEvent('phase5-custom-dispatch', true, true, { payload: 'kept' });
        document.dispatchEvent(event);
        observed === 'kept';
    )");
    EXPECT_EQ(result, true);
}



// ========== innerHTML/textContent 测试 ==========

TEST_F(DOMBindingsTest, InnerHTML) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span>Hello</span>';
        div.children.length;
    )");
    EXPECT_EQ(result, 1);
}

TEST_F(DOMBindingsTest, TextContent) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.innerHTML = '<span>Hello</span> <span>World</span>';
        div.textContent;
    )");
    EXPECT_EQ(result, "Hello World");
}

// ========== 节点属性测试 ==========

TEST_F(DOMBindingsTest, ParentNode) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var child = document.createElement('span');
        parent.appendChild(child);
        child.parentNode === parent;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, ChildNodes) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.appendChild(document.createElement('span'));
        div.appendChild(document.createTextNode('text'));
        div.childNodes.length;
    )");
    EXPECT_EQ(result, 2);
}

TEST_F(DOMBindingsTest, FirstChild) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        div.appendChild(span);
        div.firstChild === span;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, FirstChildNextSiblingAndChildNodesStayInSameOrder) {
    auto result = runtime_->Eval(R"(
        var parent = document.createElement('div');
        var comment = document.createComment('head');
        var span = document.createElement('span');
        var text = document.createTextNode('tail');

        parent.appendChild(text);
        parent.insertBefore(comment, text);
        parent.insertBefore(span, text);

        parent.firstChild === comment &&
        comment.nextSibling === span &&
        span.nextSibling === text &&
        text.nextSibling === null &&
        parent.childNodes.length === 3 &&
        parent.childNodes[0] === comment &&
        parent.childNodes[1] === span &&
        parent.childNodes[2] === text;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, CloneNodeDeepClonesOrdinaryNodesAndPreservesOrder) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        span.textContent = 'hello';
        div.appendChild(span);
        div.appendChild(document.createComment('ok'));
        div.appendChild(document.createTextNode('tail'));

        var clone = div.cloneNode(true);

        clone !== div &&
        clone.tagName === 'div' &&
        clone.childNodes.length === 3 &&
        clone.firstChild !== span &&
        clone.firstChild.tagName === 'span' &&
        clone.firstChild.textContent === 'hello' &&
        clone.firstChild.nextSibling.nodeType === 8 &&
        clone.firstChild.nextSibling.data === 'ok' &&
        clone.childNodes[2].nodeType === 3 &&
        clone.childNodes[2].data === 'tail';
    )");
    EXPECT_EQ(result, true);
}


TEST_F(DOMBindingsTest, LastChild) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        div.appendChild(document.createElement('span'));
        var p = document.createElement('p');
        div.appendChild(p);
        div.lastChild === p;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, NextSibling) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        var p = document.createElement('p');
        div.appendChild(span);
        div.appendChild(p);
        span.nextSibling === p;
    )");
    EXPECT_EQ(result, true);
}

TEST_F(DOMBindingsTest, CleanupClearsDocumentAndLegacyCleanupIsNoop) {
    window_bindings_->Cleanup();
    EXPECT_EQ(runtime_->Eval("typeof document"), "undefined");
    EXPECT_NO_THROW(DOMBindings::Cleanup(nullptr));
    window_bindings_.reset();
}

TEST_F(DOMBindingsTest, PreviousSibling) {
    auto result = runtime_->Eval(R"(
        var div = document.createElement('div');
        var span = document.createElement('span');
        var p = document.createElement('p');
        div.appendChild(span);
        div.appendChild(p);
        p.previousSibling === span;
    )");
    EXPECT_EQ(result, true);
}

} // namespace test
} // namespace mbink
