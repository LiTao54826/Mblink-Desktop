/**
 * @file test_element.cpp
 * @brief Element 类单元测试
 *
 * 测试内容：
 * - 属性操作 (setAttribute, getAttribute, hasAttribute, removeAttribute)
 * - 样式操作 (className, classList, style)
 * - 事件监听器
 * - 查询选择器
 * - innerHTML/outerHTML
 * - 伪类支持
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "test_utils/mock_objects.h"
#include "dom/element.h"
#include "dom/document.h"
#include "dom/event.h"
#include "dom/file_selection_policy.h"
#include "dom/elements/html_input_element.h"
#include "dom/elements/html_textarea_element.h"
#include "event/types/data_transfer.h"
#include "event/input/focus_manager.h"
#include "dom/utils/dom_token_list.h"
#include "dom/style/css_style_declaration.h"
#include "lexbor/style_manager.h"
#include "render/css/style_resolver.h"
#include "window/window.h"
#include <filesystem>
#include <fstream>
#ifdef GetClassName
#undef GetClassName
#endif

namespace mblink {
namespace test {

class ElementTest : public DOMTestBase {};

// ========== 基本属性测试 ==========

TEST_F(ElementTest, GetTagName) {
    auto div = CreateElement("div");
    EXPECT_EQ(div->GetTagName(), "div");

    auto span = CreateElement("SPAN");
    // 标签名通常转换为小写
    EXPECT_TRUE(span->GetTagName() == "span" || span->GetTagName() == "SPAN");
}

// ========== 属性操作测试 ==========

TEST_F(ElementTest, SetAndGetAttribute) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "test-id");

    EXPECT_EQ(elem->GetAttribute("id"), "test-id");
}

TEST_F(ElementTest, HasAttribute) {
    auto elem = CreateElement("div");

    EXPECT_FALSE(elem->HasAttribute("id"));

    elem->SetAttribute("id", "test");
    EXPECT_TRUE(elem->HasAttribute("id"));
}

TEST_F(ElementTest, RemoveAttribute) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "test");
    elem->SetAttribute("class", "foo");

    elem->RemoveAttribute("id");

    EXPECT_FALSE(elem->HasAttribute("id"));
    EXPECT_TRUE(elem->HasAttribute("class"));
}

TEST_F(ElementTest, GetAttributeNonExistent) {
    auto elem = CreateElement("div");
    EXPECT_EQ(elem->GetAttribute("nonexistent"), "");
}

TEST_F(ElementTest, SetAttributeOverwrite) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "old");
    elem->SetAttribute("id", "new");

    EXPECT_EQ(elem->GetAttribute("id"), "new");
}

// ========== className 测试 ==========

TEST_F(ElementTest, SetAndGetClassName) {
    auto elem = CreateElement("div");
    elem->SetClassName("foo bar baz");

    EXPECT_EQ(elem->GetClassName(), "foo bar baz");
}

TEST_F(ElementTest, AddClass) {
    auto elem = CreateElement("div");
    elem->AddClass("foo");
    elem->AddClass("bar");

    EXPECT_TRUE(elem->HasClass("foo"));
    EXPECT_TRUE(elem->HasClass("bar"));
}

TEST_F(ElementTest, AddClassDuplicate) {
    auto elem = CreateElement("div");
    elem->AddClass("foo");
    elem->AddClass("foo");

    // 不应该有重复
    auto className = elem->GetClassName();
    size_t count = 0;
    size_t pos = 0;
    while ((pos = className.find("foo", pos)) != std::string::npos) {
        count++;
        pos += 3;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(ElementTest, RemoveClass) {
    auto elem = CreateElement("div");
    elem->SetClassName("foo bar baz");
    elem->RemoveClass("bar");

    EXPECT_TRUE(elem->HasClass("foo"));
    EXPECT_FALSE(elem->HasClass("bar"));
    EXPECT_TRUE(elem->HasClass("baz"));
}

TEST_F(ElementTest, ToggleClass) {
    auto elem = CreateElement("div");

    // 添加
    bool result1 = elem->ToggleClass("active");
    EXPECT_TRUE(result1);
    EXPECT_TRUE(elem->HasClass("active"));

    // 移除
    bool result2 = elem->ToggleClass("active");
    EXPECT_FALSE(result2);
    EXPECT_FALSE(elem->HasClass("active"));
}

TEST_F(ElementTest, HasClass) {
    auto elem = CreateElement("div");
    elem->SetClassName("foo bar");

    EXPECT_TRUE(elem->HasClass("foo"));
    EXPECT_TRUE(elem->HasClass("bar"));
    EXPECT_FALSE(elem->HasClass("baz"));
    EXPECT_FALSE(elem->HasClass("fo"));  // 部分匹配不算
}

// ========== classList 测试 ==========

TEST_F(ElementTest, ClassListAdd) {
    auto elem = CreateElement("div");
    auto classList = elem->GetClassList();

    classList->Add("foo");
    classList->Add("bar");

    EXPECT_TRUE(classList->Contains("foo"));
    EXPECT_TRUE(classList->Contains("bar"));
}

TEST_F(ElementTest, ClassListRemove) {
    auto elem = CreateElement("div");
    elem->SetClassName("foo bar baz");
    auto classList = elem->GetClassList();

    classList->Remove("bar");

    EXPECT_TRUE(classList->Contains("foo"));
    EXPECT_FALSE(classList->Contains("bar"));
    EXPECT_TRUE(classList->Contains("baz"));
}

TEST_F(ElementTest, ClassListToggle) {
    auto elem = CreateElement("div");
    auto classList = elem->GetClassList();

    classList->Toggle("active");
    EXPECT_TRUE(classList->Contains("active"));

    classList->Toggle("active");
    EXPECT_FALSE(classList->Contains("active"));
}

TEST_F(ElementTest, ClassListLength) {
    auto elem = CreateElement("div");
    elem->SetClassName("a b c");
    auto classList = elem->GetClassList();

    EXPECT_EQ(classList->Length(), 3);
}

// ========== style 测试 ==========

TEST_F(ElementTest, SetAndGetStyle) {
    auto elem = CreateElement("div");
    elem->SetStyle("color", "red");
    elem->SetStyle("font-size", "16px");

    EXPECT_EQ(elem->GetStyle("color"), "red");
    EXPECT_EQ(elem->GetStyle("font-size"), "16px");
}

TEST_F(ElementTest, StyleDeclaration) {
    auto elem = CreateElement("div");
    auto style = elem->GetStyleDeclaration();

    style->SetProperty("background-color", "blue");
    EXPECT_EQ(style->GetPropertyValue("background-color"), "blue");
}

TEST_F(ElementTest, StyleDeclarationRemove) {
    auto elem = CreateElement("div");
    auto style = elem->GetStyleDeclaration();

    style->SetProperty("color", "red");
    style->RemoveProperty("color");

    EXPECT_EQ(style->GetPropertyValue("color"), "");
}

// ========== 事件监听器测试 ==========

TEST_F(ElementTest, AddEventListener) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    elem->AddEventListener("click", listener.GetListener());

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_TRUE(listener.WasCalled());
    EXPECT_EQ(listener.GetCallCount(), 1);
}

TEST_F(ElementTest, RemoveEventListener) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    uint64_t id = elem->AddEventListener("click", listener.GetListener());
    elem->RemoveEventListener("click", id);

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_FALSE(listener.WasCalled());
}

TEST_F(ElementTest, MultipleEventListeners) {
    auto elem = CreateElement("div");
    MockEventListener listener1;
    MockEventListener listener2;

    elem->AddEventListener("click", listener1.GetListener());
    elem->AddEventListener("click", listener2.GetListener());

    auto event = std::make_shared<Event>("click");
    elem->DispatchEvent(event);

    EXPECT_TRUE(listener1.WasCalled());
    EXPECT_TRUE(listener2.WasCalled());
}

TEST_F(ElementTest, EventListenerOnce) {
    auto elem = CreateElement("div");
    MockEventListener listener;

    elem->AddEventListener("click", listener.GetListener(), false, true);  // once=true

    auto event1 = std::make_shared<Event>("click");
    auto event2 = std::make_shared<Event>("click");

    elem->DispatchEvent(event1);
    elem->DispatchEvent(event2);

    EXPECT_EQ(listener.GetCallCount(), 1);  // 只执行一次
}

// ========== 伪类测试 ==========

TEST_F(ElementTest, SetPseudoClass) {
    auto elem = CreateElement("div");

    elem->SetPseudoClass("hover", true);
    EXPECT_TRUE(elem->HasPseudoClass("hover"));

    elem->SetPseudoClass("hover", false);
    EXPECT_FALSE(elem->HasPseudoClass("hover"));
}

TEST_F(ElementTest, PseudoClassAcceptsSelectorColonPrefix) {
    auto elem = CreateElement("button");

    elem->SetPseudoClass(":disabled", true);

    EXPECT_TRUE(elem->HasPseudoClass("disabled"));
    EXPECT_TRUE(elem->HasPseudoClass(":disabled"));
    EXPECT_TRUE(elem->Matches("button:disabled"));

    auto pseudo_classes = elem->GetActivePseudoClasses();
    EXPECT_EQ(pseudo_classes.size(), 1);
    EXPECT_EQ(pseudo_classes[0], "disabled");

    elem->SetPseudoClass("disabled", false);
    EXPECT_FALSE(elem->HasPseudoClass("disabled"));
    EXPECT_FALSE(elem->HasPseudoClass(":disabled"));
}

TEST_F(ElementTest, ButtonDisabledPseudoClassMatchesCssRule) {
    auto button = doc_->CreateElement("button");
    doc_->GetBody()->AppendChild(button);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        button { opacity: 1; color: rgb(0, 0, 0); }
        button:disabled { opacity: 0.45; color: rgb(153, 153, 153); }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    button->SetPseudoClass(":disabled", true);
    auto style = resolver.ResolveStyle(button, nullptr);

    EXPECT_FLOAT_EQ(style.opacity, 0.45f);
    EXPECT_EQ(style.color, "rgb(153, 153, 153)");
}

TEST_F(ElementTest, ButtonDisabledAttributeMatchesPseudoClassCssRule) {
    auto button = doc_->CreateElement("button");
    doc_->GetBody()->AppendChild(button);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        button { background-color: #2563eb; color: #ffffff; }
        button:disabled { background-color: #e5e7eb; color: #9ca3af; }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    button->SetAttribute("disabled", "true");

    EXPECT_TRUE(button->HasPseudoClass("disabled"));
    EXPECT_FALSE(button->HasPseudoClass("enabled"));
    EXPECT_TRUE(button->Matches("button:disabled"));

    auto style = resolver.ResolveStyle(button, nullptr);
    EXPECT_EQ(style.background_color, "#e5e7eb");
    EXPECT_EQ(style.color, "#9ca3af");
}

TEST_F(ElementTest, ButtonDisabledAttributeMatchesClassPseudoClassCssRule) {
    auto button = doc_->CreateElement("button");
    button->SetClassName("primary-button");
    doc_->GetBody()->AppendChild(button);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .primary-button { background-color: #2563eb; color: #ffffff; }
        .primary-button:disabled { background-color: #e5e7eb; color: #9ca3af; }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    button->SetAttribute("disabled", "true");

    auto style = resolver.ResolveStyle(button, nullptr);
    EXPECT_EQ(style.background_color, "#e5e7eb");
    EXPECT_EQ(style.color, "#9ca3af");
}

TEST_F(ElementTest, HoverPseudoClassDoesNotDirtyAncestor) {
    auto parent = CreateElement("div");
    auto child = CreateElement("div");
    parent->AppendChild(child);

    parent->ClearDirty();
    child->ClearDirty();

    child->SetPseudoClass("hover", true);

    EXPECT_TRUE(child->HasPseudoClass("hover"));
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(parent->IsStyleDirty());
    EXPECT_FALSE(parent->IsPaintDirty());
}

TEST_F(ElementTest, BuiltinButtonHoverStyleStillResolves) {
    auto button = CreateElement("button");
    button->SetStyle("background-color", "#808080");

    StyleResolver resolver;
    auto normal_style = resolver.ResolveStyle(button, nullptr);

    button->SetPseudoClass("hover", true);
    auto hover_style = resolver.ResolveStyle(button, nullptr);

    EXPECT_NE(normal_style.background_color, hover_style.background_color);
    EXPECT_EQ(hover_style.background_color, "#6C6C6C");
}

TEST_F(ElementTest, DescendantHoverSelectorStillResolves) {
    auto parent = CreateElement("div");
    parent->SetClassName("parent");
    auto child = CreateElement("span");
    child->SetClassName("child");
    parent->AppendChild(child);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .parent:hover .child { color: rgb(255, 0, 0); }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    auto normal_style = resolver.ResolveStyle(child, nullptr);
    parent->SetPseudoClass("hover", true);
    auto hover_style = resolver.ResolveStyle(child, nullptr);

    EXPECT_NE(normal_style.color, hover_style.color);
    EXPECT_EQ(hover_style.color, "rgb(255, 0, 0)");
}

TEST_F(ElementTest, FontFamilyInheritKeepsParentFontList) {
    auto parent = CreateElement("div");
    parent->SetClassName("font-parent");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .font-parent { font-family: "Segoe UI", "Microsoft YaHei", sans-serif; }
        input { font-family: inherit; }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    auto parent_style = resolver.ResolveStyle(parent, nullptr);
    auto input_style = resolver.ResolveStyle(input, &parent_style);

    EXPECT_EQ(parent_style.font_family, "Segoe UI, Microsoft YaHei, sans-serif");
    EXPECT_EQ(input_style.font_family, parent_style.font_family);
}

TEST_F(ElementTest, TextInputDefaultColorUsesFieldText) {
    auto parent = CreateElement("div");
    parent->SetClassName("dark-parent");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "text");
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .dark-parent { color: #f5f5fa; }
    )"));

    StyleResolver resolver;
    resolver.SetStyleManager(doc_->GetStyleManager());

    auto parent_style = resolver.ResolveStyle(parent, nullptr);
    auto input_style = resolver.ResolveStyle(input, &parent_style);

    EXPECT_EQ(parent_style.color, "#f5f5fa");
    EXPECT_EQ(input_style.color, "#000000");
}

TEST_F(ElementTest, FileInputMaintainsBrowserStyleFileList) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-test";
    std::filesystem::create_directories(temp_dir);
    const auto first_path = temp_dir / "first.txt";
    const auto second_path = temp_dir / "second.json";
    {
        std::ofstream(first_path) << "hello";
        std::ofstream(second_path) << "{}";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");

    int input_events = 0;
    int change_events = 0;
    input->AddEventListener("input", [&](std::shared_ptr<Event>) { ++input_events; });
    input->AddEventListener("change", [&](std::shared_ptr<Event>) { ++change_events; });

    input->SetFilesFromPaths({first_path.string(), second_path.string()}, true);
    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "first.txt");
    EXPECT_EQ(input->GetFiles()[0].type, "text/plain");
    EXPECT_EQ(input->GetFiles()[0].size, 5u);
    EXPECT_EQ(input->GetValue(), "C:\\fakepath\\first.txt");
    EXPECT_EQ(input_events, 1);
    EXPECT_EQ(change_events, 1);

    input->SetAttribute("multiple", "");
    input->SetFilesFromPaths({first_path.string(), second_path.string()}, true);
    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_EQ(input->GetFiles()[1].name, "second.json");
    EXPECT_EQ(input->GetFiles()[1].type, "application/json");
    EXPECT_EQ(input->GetValue(), "C:\\fakepath\\first.txt");
    EXPECT_EQ(input_events, 2);
    EXPECT_EQ(change_events, 2);

    input->SetValue("C:/fake/path.txt", true);
    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_EQ(input_events, 2);
    EXPECT_EQ(change_events, 2);

    input->SetValue("", true);
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());
    EXPECT_EQ(input_events, 3);
    EXPECT_EQ(change_events, 3);

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputValueAttributeCannotForgeSelection) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-value-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "picked.txt";
    {
        std::ofstream(file_path) << "value";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetFilesFromPaths({file_path.string()}, false);
    ASSERT_EQ(input->GetFiles().size(), 1u);

    input->SetAttribute("value", "C:/fake/path.txt");
    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "picked.txt");
    EXPECT_EQ(input->GetValue(), "C:\\fakepath\\picked.txt");

    input->SetAttribute("value", "");
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputClearsSelectionAcrossTypeChanges) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-type-change-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "picked.txt";
    {
        std::ofstream(file_path) << "value";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetFilesFromPaths({file_path.string()}, false);
    ASSERT_EQ(input->GetFiles().size(), 1u);

    input->SetAttribute("type", "text");
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());

    input->SetAttribute("type", "file");
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());

    input->SetAttribute("type", "text");
    input->SetValue("plain text", false);
    input->SetAttribute("type", "file");
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());

    input->SetInputType(InputType::File);
    input->SetFilesFromPaths({file_path.string()}, false);
    ASSERT_EQ(input->GetFiles().size(), 1u);
    input->SetInputType(InputType::Text);
    EXPECT_TRUE(input->GetFiles().empty());
    EXPECT_TRUE(input->GetValue().empty());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputAppliesAcceptAndDirectorySelectionPolicy) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-policy-test";
    std::filesystem::create_directories(temp_dir);
    const auto text_path = temp_dir / "note.TXT";
    const auto json_path = temp_dir / "data.json";
    const auto image_path = temp_dir / "photo.png";
    const auto directory_path = temp_dir / "folder";
    const auto nested_path = directory_path / "nested";
    const auto directory_file_path = nested_path / "inside.txt";
    std::filesystem::create_directories(directory_path);
    std::filesystem::create_directories(nested_path);
    {
        std::ofstream(text_path) << "text";
        std::ofstream(json_path) << "{}";
        std::ofstream(image_path) << "png";
        std::ofstream(directory_file_path) << "inside";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("multiple", "");
    input->SetAttribute("accept", ".txt,image/*,application/json;ignored");
    input->SetFilesFromPaths({json_path.string(), text_path.string(), image_path.string(), directory_path.string()}, false);

    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_EQ(input->GetFiles()[0].name, "note.TXT");
    EXPECT_EQ(input->GetFiles()[1].name, "photo.png");

    input->SetAttribute("directory", "");
    input->SetAttribute("accept", "");
    input->SetFilesFromPaths({directory_path.string(), text_path.string()}, false);
    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_FALSE(input->GetFiles()[0].is_directory);
    EXPECT_EQ(input->GetFiles()[0].name, "inside.txt");
    EXPECT_EQ(input->GetFiles()[0].webkit_relative_path, "folder/nested/inside.txt");
    EXPECT_EQ(input->GetFiles()[1].name, "note.TXT");

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputDirectorySelectionKeepsAllFilesWithoutMultiple) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-directory-single-test";
    const auto directory_path = temp_dir / "folder";
    const auto nested_path = directory_path / "nested";
    const auto first_path = directory_path / "a.txt";
    const auto second_path = nested_path / "b.txt";
    std::filesystem::create_directories(nested_path);
    {
        std::ofstream(first_path) << "a";
        std::ofstream(second_path) << "bb";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("webkitdirectory", "");
    input->SetFilesFromPaths({directory_path.string()}, false);

    ASSERT_EQ(input->GetFiles().size(), 2u);
    EXPECT_EQ(input->GetFiles()[0].name, "a.txt");
    EXPECT_EQ(input->GetFiles()[0].webkit_relative_path, "folder/a.txt");
    EXPECT_EQ(input->GetFiles()[1].name, "b.txt");
    EXPECT_EQ(input->GetFiles()[1].webkit_relative_path, "folder/nested/b.txt");
    EXPECT_EQ(input->GetValue(), "C:\\fakepath\\a.txt");

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputAcceptSupportsExactMimeAndEmptyFilteredSelection) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-file-input-accept-test";
    std::filesystem::create_directories(temp_dir);
    const auto text_path = temp_dir / "note.txt";
    const auto json_path = temp_dir / "data.json";
    {
        std::ofstream(text_path) << "text";
        std::ofstream(json_path) << "{}";
    }

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    input->SetAttribute("type", "file");
    input->SetAttribute("multiple", "");
    input->SetAttribute("accept", "application/json");
    input->SetFilesFromPaths({text_path.string(), json_path.string()}, false);

    ASSERT_EQ(input->GetFiles().size(), 1u);
    EXPECT_EQ(input->GetFiles()[0].name, "data.json");
    EXPECT_EQ(input->GetFiles()[0].type, "application/json");

    input->SetAttribute("accept", "application/pdf");
    input->SetFilesFromPaths({text_path.string(), json_path.string()}, false);
    EXPECT_TRUE(input->GetFiles().empty());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, FileInputBuildsNativeDialogFiltersFromAccept) {
    const auto filters = BuildFileDialogAcceptFilters(".txt, image/*, application/json, image/jpeg, application/unknown");

    ASSERT_EQ(filters.size(), 4u);
    EXPECT_EQ(filters[0].name, ".txt");
    EXPECT_EQ(filters[0].pattern, "txt");
    EXPECT_EQ(filters[1].name, "image/*");
    EXPECT_EQ(filters[1].pattern, "png;jpg;jpeg;gif;webp;svg");
    EXPECT_EQ(filters[2].name, "application/json");
    EXPECT_EQ(filters[2].pattern, "json");
    EXPECT_EQ(filters[3].name, "image/jpeg");
    EXPECT_EQ(filters[3].pattern, "jpg;jpeg");
}

TEST_F(ElementTest, DataTransferExposesFilesTypeWhenFilesArePresent) {
    const auto temp_dir = std::filesystem::temp_directory_path() / "mblink-data-transfer-file-test";
    std::filesystem::create_directories(temp_dir);
    const auto file_path = temp_dir / "dragged.txt";
    const auto directory_path = temp_dir / "folder";
    const auto nested_path = directory_path / "nested";
    const auto directory_file_path = nested_path / "inside.txt";
    std::filesystem::create_directories(nested_path);
    {
        std::ofstream(file_path) << "drop";
        std::ofstream(directory_file_path) << "inside";
    }

    DataTransfer transfer;
    transfer.SetData("text/plain", "payload");
    transfer.SetFilesFromPaths({file_path.string()});

    const auto types = transfer.GetTypes();
    EXPECT_NE(std::find(types.begin(), types.end(), "text/plain"), types.end());
    EXPECT_NE(std::find(types.begin(), types.end(), "Files"), types.end());
    ASSERT_EQ(transfer.GetFiles().size(), 1u);
    EXPECT_EQ(transfer.GetFiles()[0].name, "dragged.txt");
    EXPECT_EQ(transfer.GetFiles()[0].size, 4u);

    transfer.SetFilesFromPaths({directory_path.string()});
    ASSERT_EQ(transfer.GetFiles().size(), 1u);
    EXPECT_FALSE(transfer.GetFiles()[0].is_directory);
    EXPECT_EQ(transfer.GetFiles()[0].name, "inside.txt");
    EXPECT_EQ(transfer.GetFiles()[0].webkit_relative_path, "folder/nested/inside.txt");

    transfer.ClearData();
    EXPECT_TRUE(transfer.GetFiles().empty());
    EXPECT_TRUE(transfer.GetTypes().empty());

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(ElementTest, HoverPseudoClassRestylesDescendantRenderObject) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    parent->SetClassName("parent");
    auto child = CreateElement("span");
    child->SetClassName("child");
    parent->AppendChild(child);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .parent:hover .child { color: rgb(255, 0, 0); }
    )"));

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(child->GetRenderObject(), nullptr);
    EXPECT_NE(child->GetRenderObject()->GetComputedStyle().color, "rgb(255, 0, 0)");

    parent->SetPseudoClass("hover", true);

    EXPECT_TRUE(parent->HasPseudoClass("hover"));
    EXPECT_EQ(child->GetRenderObject()->GetComputedStyle().color, "rgb(255, 0, 0)");
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::PseudoClass);
}

TEST_F(ElementTest, DisabledAttributeRestylesExistingButtonRenderObject) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto button = CreateElement("button");
    button->SetClassName("primary-button");
    doc_->GetBody()->AppendChild(button);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .primary-button { background-color: #2563eb; color: #ffffff; }
        .primary-button:disabled { background-color: #e5e7eb; color: #9ca3af; }
    )"));

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(button->GetRenderObject(), nullptr);

    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().background_color, "#2563eb");
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().color, "#ffffff");

    button->SetAttribute("disabled", "true");

    EXPECT_TRUE(button->HasPseudoClass("disabled"));
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().background_color, "#e5e7eb");
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().color, "#9ca3af");
    EXPECT_TRUE(button->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::DOMMutation);

    button->RemoveAttribute("disabled");

    EXPECT_FALSE(button->HasPseudoClass("disabled"));
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().background_color, "#2563eb");
    EXPECT_EQ(button->GetRenderObject()->GetComputedStyle().color, "#ffffff");
}

TEST_F(ElementTest, HoverPseudoClassExitRestylesDescendantRenderObject) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    parent->SetClassName("parent");
    auto child = CreateElement("span");
    child->SetClassName("child");
    parent->AppendChild(child);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .parent:hover .child { color: rgb(255, 0, 0); }
    )"));

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(child->GetRenderObject(), nullptr);

    parent->SetPseudoClass("hover", true);
    EXPECT_EQ(child->GetRenderObject()->GetComputedStyle().color, "rgb(255, 0, 0)");

    parent->SetPseudoClass("hover", false);
    EXPECT_FALSE(parent->HasPseudoClass("hover"));
    EXPECT_NE(child->GetRenderObject()->GetComputedStyle().color, "rgb(255, 0, 0)");
}

TEST_F(ElementTest, LayoutHoverStyleMarksDescendantForLayout) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    parent->SetClassName("parent");
    auto child = CreateElement("span");
    child->SetClassName("child");
    parent->AppendChild(child);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .parent:hover .child { padding-left: 12px; }
    )"));

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(child->GetRenderObject(), nullptr);
    child->GetRenderObject()->ClearNeedsLayout();
    child->GetRenderObject()->ClearNeedsPaint();

    parent->SetPseudoClass("hover", true);

    const auto& child_style = child->GetRenderObject()->GetComputedStyle();
    EXPECT_EQ(child_style.padding_left.ToPx(0.0f, child_style.font_size), 12.0f);
    EXPECT_TRUE(child->GetRenderObject()->NeedsLayout());
    EXPECT_TRUE(child->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::PseudoClass);
}

TEST_F(ElementTest, HoverPseudoClassRestylesInheritedTextRenderObject) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    parent->SetClassName("parent");
    auto text = CreateTextNode("label");
    parent->AppendChild(text);
    doc_->GetBody()->AppendChild(parent);

    ASSERT_TRUE(doc_->GetStyleManager()->ParseCSSString(R"(
        .parent:hover { color: rgb(0, 128, 255); }
    )"));

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(text->GetRenderObject(), nullptr);
    EXPECT_NE(text->GetRenderObject()->GetComputedStyle().color, "rgb(0, 128, 255)");

    parent->SetPseudoClass("hover", true);

    EXPECT_EQ(parent->GetRenderObject()->GetComputedStyle().color, "rgb(0, 128, 255)");
    EXPECT_EQ(text->GetRenderObject()->GetComputedStyle().color, "rgb(0, 128, 255)");
}

TEST_F(ElementTest, FocusPseudoClassDoesNotDirtyAncestor) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(input->GetRenderObject(), nullptr);

    doc_->GetBody()->ClearDirty();
    parent->ClearDirty();
    input->ClearDirty();
    input->GetRenderObject()->ClearNeedsPaint();

    input->SetPseudoClass("focus", true);

    EXPECT_TRUE(input->HasPseudoClass("focus"));
    EXPECT_FALSE(doc_->GetBody()->IsDirty());
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(input->IsDirty());
    EXPECT_TRUE(input->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::PseudoClass);
}

TEST_F(ElementTest, FocusPseudoClassRestylesInputRenderObject) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    doc_->GetBody()->AppendChild(input);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(input->GetRenderObject(), nullptr);

    input->SetPseudoClass("focus", true);

    const auto& focused_style = input->GetRenderObject()->GetComputedStyle();
    EXPECT_EQ(focused_style.outline_style, "solid");
    EXPECT_EQ(focused_style.outline_width.ToPx(0.0f, focused_style.font_size), 2.0f);
    EXPECT_TRUE(input->GetRenderObject()->NeedsPaint());
}

TEST_F(ElementTest, FocusManagerOnlyMarksFocusedElementPseudoClass) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);

    FocusManager focus_manager;
    focus_manager.SetWindow(window.get());
    ASSERT_TRUE(focus_manager.SetFocus(input, false));

    EXPECT_TRUE(input->HasPseudoClass("focus"));
    EXPECT_FALSE(parent->HasPseudoClass("focus"));
    EXPECT_FALSE(doc_->GetBody()->HasPseudoClass("focus"));
}

TEST_F(ElementTest, FocusManagerMarksOnlyFocusedElementDirtyRegion) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(input->GetRenderObject(), nullptr);

    doc_->GetBody()->ClearDirty();
    parent->ClearDirty();
    input->ClearDirty();
    input->GetRenderObject()->ClearNeedsPaint();
    window->ClearDirtyRects();

    FocusManager focus_manager;
    focus_manager.SetWindow(window.get());
    ASSERT_TRUE(focus_manager.SetFocus(input, false));

    EXPECT_FALSE(doc_->GetBody()->IsDirty());
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(input->IsDirty());
    EXPECT_TRUE(input->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::Focus);
    EXPECT_LE(window->GetDirtyRects().size(), 1u);
}

TEST_F(ElementTest, InputRepaintDoesNotDirtyAncestor) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    auto input = std::dynamic_pointer_cast<HTMLInputElement>(doc_->CreateElement("input"));
    ASSERT_NE(input, nullptr);
    parent->AppendChild(input);
    doc_->GetBody()->AppendChild(parent);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(input->GetRenderObject(), nullptr);

    doc_->GetBody()->ClearDirty();
    parent->ClearDirty();
    input->ClearDirty();
    input->GetRenderObject()->ClearNeedsPaint();

    input->SetValue("a", false);

    EXPECT_EQ(input->GetValue(), "a");
    EXPECT_FALSE(doc_->GetBody()->IsDirty());
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(input->IsDirty());
    EXPECT_TRUE(input->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::KeyboardInput);
}

TEST_F(ElementTest, TextAreaRepaintDoesNotDirtyAncestor) {
    WindowConfig config;
    config.hidden = true;
    config.headless = true;
    config.backend = RenderBackend::CPU;
    auto window = std::make_shared<Window>(config);
    window->SetDocument(doc_);

    auto parent = CreateElement("div");
    auto textarea = std::dynamic_pointer_cast<HTMLTextAreaElement>(doc_->CreateElement("textarea"));
    ASSERT_NE(textarea, nullptr);
    parent->AppendChild(textarea);
    doc_->GetBody()->AppendChild(parent);

    RenderTreeBuilder builder;
    builder.SetDocument(doc_.get());
    auto render_root = builder.BuildRenderTree(doc_->GetBody());
    ASSERT_NE(render_root, nullptr);
    ASSERT_NE(textarea->GetRenderObject(), nullptr);

    doc_->GetBody()->ClearDirty();
    parent->ClearDirty();
    textarea->ClearDirty();
    textarea->GetRenderObject()->ClearNeedsPaint();

    textarea->HandleTextInput("a");

    EXPECT_EQ(textarea->GetValue(), "a");
    EXPECT_FALSE(doc_->GetBody()->IsDirty());
    EXPECT_FALSE(parent->IsDirty());
    EXPECT_FALSE(textarea->IsDirty());
    EXPECT_TRUE(textarea->GetRenderObject()->NeedsPaint());
    EXPECT_TRUE(window->NeedsRepaint());
    EXPECT_EQ(window->GetLastRepaintReason(), RepaintReason::KeyboardInput);
}

TEST_F(ElementTest, MultiplePseudoClasses) {
    auto elem = CreateElement("div");

    elem->SetPseudoClass("hover", true);
    elem->SetPseudoClass("active", true);
    elem->SetPseudoClass("focus", true);

    auto pseudoClasses = elem->GetActivePseudoClasses();
    EXPECT_EQ(pseudoClasses.size(), 3);
}

// ========== 查询选择器测试 ==========

TEST_F(ElementTest, QuerySelectorById) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");

    child1->SetAttribute("id", "target");
    parent->AppendChild(child1);
    parent->AppendChild(child2);

    auto result = parent->QuerySelector("#target");
    EXPECT_EQ(result, child1);
}

TEST_F(ElementTest, QuerySelectorByClass) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");

    child1->AddClass("highlight");
    parent->AppendChild(child1);
    parent->AppendChild(child2);

    auto result = parent->QuerySelector(".highlight");
    EXPECT_EQ(result, child1);
}

TEST_F(ElementTest, QuerySelectorByTagName) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("p");

    parent->AppendChild(child1);
    parent->AppendChild(child2);

    auto result = parent->QuerySelector("p");
    EXPECT_EQ(result, child2);
}

TEST_F(ElementTest, QuerySelectorAll) {
    auto parent = CreateElement("div");
    auto child1 = CreateElement("span");
    auto child2 = CreateElement("span");
    auto child3 = CreateElement("p");

    parent->AppendChild(child1);
    parent->AppendChild(child2);
    parent->AppendChild(child3);

    auto results = parent->QuerySelectorAll("span");
    EXPECT_EQ(results.size(), 2);
}

TEST_F(ElementTest, Matches) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "test");
    elem->AddClass("foo");

    EXPECT_TRUE(elem->Matches("div"));
    EXPECT_TRUE(elem->Matches("#test"));
    EXPECT_TRUE(elem->Matches(".foo"));
    EXPECT_TRUE(elem->Matches("div.foo"));
    EXPECT_FALSE(elem->Matches("span"));
}

TEST_F(ElementTest, Closest) {
    auto grandparent = CreateElement("div");
    auto parent = CreateElement("section");
    auto child = CreateElement("span");

    grandparent->AddClass("container");
    grandparent->AppendChild(parent);
    parent->AppendChild(child);

    auto result = child->Closest(".container");
    EXPECT_EQ(result, grandparent);
}

// ========== innerHTML/outerHTML 测试 ==========

TEST_F(ElementTest, GetInnerHTML) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");
    auto text = CreateTextNode("Hello");

    child->AppendChild(text);
    parent->AppendChild(child);

    auto innerHTML = parent->GetInnerHTML();
    EXPECT_TRUE(innerHTML.find("span") != std::string::npos);
    EXPECT_TRUE(innerHTML.find("Hello") != std::string::npos);
}

TEST_F(ElementTest, SetInnerHTML) {
    auto elem = CreateElement("div");
    elem->SetInnerHTML("<span>Test</span><p>Content</p>");

    EXPECT_EQ(elem->GetChildNodes().size(), 2);
}

TEST_F(ElementTest, GetOuterHTML) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "test");
    elem->AppendChild(CreateTextNode("Content"));

    auto outerHTML = elem->GetOuterHTML();
    EXPECT_TRUE(outerHTML.find("<div") != std::string::npos);
    EXPECT_TRUE(outerHTML.find("id=\"test\"") != std::string::npos ||
                outerHTML.find("id='test'") != std::string::npos);
}

// ========== 文本内容测试 ==========

TEST_F(ElementTest, GetTextContent) {
    auto parent = CreateElement("div");
    auto child = CreateElement("span");

    parent->AppendChild(CreateTextNode("Hello "));
    child->AppendChild(CreateTextNode("World"));
    parent->AppendChild(child);

    EXPECT_EQ(parent->GetTextContent(), "Hello World");
}

TEST_F(ElementTest, SetTextContent) {
    auto elem = CreateElement("div");
    elem->AppendChild(CreateElement("span"));
    elem->AppendChild(CreateElement("p"));

    elem->SetTextContent("New Content");

    EXPECT_EQ(elem->GetChildNodes().size(), 1);
    EXPECT_EQ(elem->GetTextContent(), "New Content");
}

// ========== 克隆测试 ==========

TEST_F(ElementTest, CloneNodeShallow) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "original");
    elem->AddClass("foo");
    elem->AppendChild(CreateElement("span"));

    auto clone = std::dynamic_pointer_cast<Element>(elem->CloneNode(false));

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetAttribute("id"), "original");
    EXPECT_TRUE(clone->HasClass("foo"));
    EXPECT_EQ(clone->GetChildNodes().size(), 0);  // 浅克隆不包含子节点
}

TEST_F(ElementTest, CloneNodeDeep) {
    auto elem = CreateElement("div");
    elem->SetAttribute("id", "original");
    elem->AppendChild(CreateElement("span"));

    auto clone = std::dynamic_pointer_cast<Element>(elem->CloneNode(true));

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->GetAttribute("id"), "original");
    EXPECT_EQ(clone->GetChildNodes().size(), 1);  // 深克隆包含子节点
}

} // namespace test
} // namespace mblink
