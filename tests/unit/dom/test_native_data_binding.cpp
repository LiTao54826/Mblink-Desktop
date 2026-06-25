/**
 * @file test_native_data_binding.cpp
 * @brief Stage 1 MVP 原生数据绑定 prototype 验收
 */

#include <gtest/gtest.h>

#include "core/bridge/state_manager.h"
#include "core/dom/bindings/native_data_binding.h"
#include "core/dom/event.h"
#include "tests/test_utils/test_helpers.h"

namespace mblink {
namespace test {

class NativeDataBindingTest : public DOMTestBase {};

TEST_F(NativeDataBindingTest, TextBindingFlushUpdatesOnlyTargetTextNode) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("profile", json{{"name", "Alice"}}), MBlinkError::Ok);

    auto host = CreateElement("span");
    auto sibling = CreateElement("em");
    auto text = CreateTextNode("");
    host->AppendChild(text);
    host->AppendChild(sibling);
    doc_->GetBody()->AppendChild(host);

    auto bindingId = runtime.createTextBinding(host, text, "profile.name");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_EQ(text->GetData(), "Alice");

    auto hostBefore = host.get();
    auto siblingBefore = sibling.get();
    ASSERT_TRUE(state.set("profile.name", "Bob"));
    runtime.flush();

    EXPECT_EQ(text->GetData(), "Bob");
    EXPECT_EQ(host.get(), hostBefore);
    EXPECT_EQ(sibling.get(), siblingBefore);
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, VisibleBindingTogglesHiddenWithoutRebuild) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("panel", json{{"visible", true}}), MBlinkError::Ok);

    auto panel = CreateElement("div");
    panel->AppendChild(CreateTextNode("content"));
    doc_->GetBody()->AppendChild(panel);

    auto bindingId = runtime.createVisibleBinding(panel, panel, "panel.visible");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_FALSE(panel->HasAttribute("hidden"));

    auto panelBefore = panel.get();
    ASSERT_TRUE(state.set("panel.visible", false));
    runtime.flush();
    EXPECT_TRUE(panel->HasAttribute("hidden"));

    ASSERT_TRUE(state.set("panel.visible", true));
    runtime.flush();
    EXPECT_FALSE(panel->HasAttribute("hidden"));
    EXPECT_EQ(panel.get(), panelBefore);
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, AttrBindingSetsAndRemovesAttribute) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("profile", json{{"role", "admin"}}), MBlinkError::Ok);

    auto element = CreateElement("div");
    doc_->GetBody()->AppendChild(element);

    auto bindingId = runtime.createAttrBinding(element, element, "data-role", "profile.role");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_EQ(element->GetAttribute("data-role"), "admin");

    ASSERT_TRUE(state.set("profile.role", nullptr));
    runtime.flush();
    EXPECT_FALSE(element->HasAttribute("data-role"));
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, ModelBindingValueSupportsDomWriteBackAndGraphSync) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("form", json{{"username", "tom"}}), MBlinkError::Ok);

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(CreateElement("input"));
    ASSERT_NE(input, nullptr);
    doc_->GetBody()->AppendChild(input);

    auto bindingId = runtime.createModelValueBinding(input, input, "form.username");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_EQ(input->GetValue(), "tom");

    input->SetValue("jerry", false);
    input->DispatchEvent(std::make_shared<Event>("input"));
    runtime.flush();
    EXPECT_EQ(state.get("form.username"), json("jerry"));
    EXPECT_EQ(input->GetValue(), "jerry");

    ASSERT_TRUE(state.set("form.username", "rose"));
    runtime.flush();
    EXPECT_EQ(input->GetValue(), "rose");
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, ModelBindingCheckedSupportsDomWriteBackAndGraphSync) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("form", json{{"agreed", true}}), MBlinkError::Ok);

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetInputType(InputType::Checkbox);
    doc_->GetBody()->AppendChild(input);

    auto bindingId = runtime.createModelCheckedBinding(input, input, "form.agreed");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_TRUE(input->GetChecked());

    input->SetChecked(false, false);
    input->DispatchEvent(std::make_shared<Event>("change"));
    runtime.flush();
    EXPECT_EQ(state.get("form.agreed"), json(false));
    EXPECT_FALSE(input->GetChecked());

    ASSERT_TRUE(state.set("form.agreed", true));
    runtime.flush();
    EXPECT_TRUE(input->GetChecked());
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, ReadonlyScopeBlocksModelWriteBack) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("page", json{{"form", json{{"user", json{{"name", "Alice"}}}}}}), MBlinkError::Ok);

    auto host = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(CreateElement("input"));
    ASSERT_NE(input, nullptr);
    host->AppendChild(input);
    doc_->GetBody()->AppendChild(host);

    runtime.createScope(host, {{"user", ScopeSlot{"page.form.user", true}}}, false);
    auto bindingId = runtime.createModelValueBinding(host, input, "user.name");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();
    EXPECT_EQ(input->GetValue(), "Alice");

    input->SetValue("Bob", false);
    input->DispatchEvent(std::make_shared<Event>("input"));
    runtime.flush();
    EXPECT_EQ(state.get("page.form.user.name"), json("Alice"));
    EXPECT_FALSE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, DeclarativeBindingsSupportTextVisibleAndModel) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("profile", json{{"name", "Alice"}}), MBlinkError::Ok);
    ASSERT_EQ(state.createJson("panel", json{{"visible", true}}), MBlinkError::Ok);
    ASSERT_EQ(state.createJson("form", json{{"username", "tom"}}), MBlinkError::Ok);

    auto host = CreateElement("div");
    auto text = CreateElement("span");
    auto panel = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(CreateElement("input"));
    ASSERT_NE(input, nullptr);

    text->SetAttribute("mb-text", "profile.name");
    panel->SetAttribute("mb-visible", "panel.visible");
    input->SetAttribute("mb-model", "form.username");
    host->AppendChild(text);
    host->AppendChild(panel);
    host->AppendChild(input);
    doc_->GetBody()->AppendChild(host);

    EXPECT_EQ(runtime.mountDeclarative(host), 3u);
    runtime.flush();
    EXPECT_EQ(text->GetTextContent(), "Alice");
    EXPECT_FALSE(panel->HasAttribute("hidden"));
    EXPECT_EQ(input->GetValue(), "tom");

    ASSERT_TRUE(state.set("profile.name", "Bob"));
    ASSERT_TRUE(state.set("panel.visible", false));
    ASSERT_TRUE(state.set("form.username", "rose"));
    runtime.flush();
    EXPECT_EQ(text->GetTextContent(), "Bob");
    EXPECT_TRUE(panel->HasAttribute("hidden"));
    EXPECT_EQ(input->GetValue(), "rose");

    input->SetValue("jerry", false);
    input->DispatchEvent(std::make_shared<Event>("input"));
    runtime.flush();
    EXPECT_EQ(state.get("form.username"), json("jerry"));
    EXPECT_EQ(input->GetValue(), "jerry");
    EXPECT_TRUE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, DeclarativeScopeResolvesAliasAndReadonlyWriteBackFails) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("page", json{{"form", json{{"user", json{{"name", "Alice"}}}}}}), MBlinkError::Ok);

    auto host = CreateElement("div");
    auto text = CreateElement("span");
    auto readonlyHost = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(CreateElement("input"));
    ASSERT_NE(input, nullptr);

    host->SetAttribute("mb-scope:user", "page.form.user");
    text->SetAttribute("mb-text", "user.name");
    readonlyHost->SetAttribute("mb-scope-ro:user", "page.form.user");
    input->SetAttribute("mb-model", "user.name");
    host->AppendChild(text);
    readonlyHost->AppendChild(input);
    host->AppendChild(readonlyHost);
    doc_->GetBody()->AppendChild(host);

    EXPECT_EQ(runtime.mountDeclarative(host), 2u);
    runtime.flush();
    EXPECT_EQ(text->GetTextContent(), "Alice");
    EXPECT_EQ(input->GetValue(), "Alice");

    ASSERT_TRUE(state.set("page.form.user.name", "Carol"));
    runtime.flush();
    EXPECT_EQ(text->GetTextContent(), "Carol");
    EXPECT_EQ(input->GetValue(), "Carol");

    input->SetValue("Eve", false);
    input->DispatchEvent(std::make_shared<Event>("input"));
    runtime.flush();
    EXPECT_EQ(state.get("page.form.user.name"), json("Carol"));
    EXPECT_FALSE(runtime.errors().empty());
}

TEST_F(NativeDataBindingTest, StateBatchCoalescesExactPathFlush) {
    StateManager state;
    NativeDataBindingRuntime runtime(state);
    ASSERT_EQ(state.createJson("profile", json{{"name", "Alice"}}), MBlinkError::Ok);

    auto host = CreateElement("span");
    auto text = CreateTextNode("");
    host->AppendChild(text);
    doc_->GetBody()->AppendChild(host);

    int watchCount = 0;
    state.watch("profile.name", [&](const std::string&, const json&) { ++watchCount; });

    auto bindingId = runtime.createTextBinding(host, text, "profile.name");
    ASSERT_TRUE(runtime.mountBinding(bindingId));
    runtime.flush();

    state.beginBatch();
    ASSERT_TRUE(state.set("profile.name", "Bob"));
    ASSERT_TRUE(state.set("profile.name", "Carol"));
    state.endBatch();
    runtime.flush();

    EXPECT_EQ(text->GetData(), "Carol");
    EXPECT_EQ(watchCount, 1);
}

} // namespace test
} // namespace mblink