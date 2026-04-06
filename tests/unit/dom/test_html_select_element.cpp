#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "test_utils/mock_objects.h"
#include "dom/elements/html_select_element.h"
#include "dom/elements/html_option_element.h"

namespace mbink {
namespace test {

class HTMLSelectElementTest : public DOMTestBase {};

TEST_F(HTMLSelectElementTest, SetValueSelectsExistingOption) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));

    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    select->SetValue("b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
    EXPECT_FALSE(optionA->GetSelected());
}

TEST_F(HTMLSelectElementTest, ValueAttributeSelectsExistingOption) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));

    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    select->SetAttribute("value", "b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
}

TEST_F(HTMLSelectElementTest, PendingValueAppliedWhenOptionAppendedLater) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    select->SetAttribute("value", "b");

    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");

    select->AppendChild(optionA);
    EXPECT_EQ(select->GetValue(), "");
    EXPECT_EQ(select->GetSelectedIndex(), -1);

    select->AppendChild(optionB);

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 1);
    EXPECT_TRUE(optionB->GetSelected());
}

TEST_F(HTMLSelectElementTest, ProgrammaticSetValueDoesNotTriggerChange) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);

    MockEventListener listener;
    select->AddEventListener("change", listener.GetListener());

    select->SetValue("b");
    EXPECT_EQ(listener.GetCallCount(), 0);

    select->SetAttribute("value", "a");
    EXPECT_EQ(listener.GetCallCount(), 0);
}

TEST_F(HTMLSelectElementTest, UserSelectionHelpersTriggerChange) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto optionA = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    auto optionB = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    optionA->SetAttribute("value", "a");
    optionB->SetAttribute("value", "b");
    select->AppendChild(optionA);
    select->AppendChild(optionB);
    select->SetSelectedIndex(0);

    MockEventListener listener;
    select->AddEventListener("change", listener.GetListener());

    select->SelectNextOption();
    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(listener.GetCallCount(), 1);

    select->SetHoveredIndex(0);
    select->SelectHoveredOption();
    EXPECT_EQ(select->GetValue(), "a");
    EXPECT_EQ(listener.GetCallCount(), 2);
}

TEST_F(HTMLSelectElementTest, PendingValueReappliedWhenOptionValueChanges) {
    auto select = std::dynamic_pointer_cast<HTMLSelectElement>(CreateElement("select"));
    auto option = std::dynamic_pointer_cast<HTMLOptionElement>(CreateElement("option"));
    option->SetAttribute("value", "a");
    select->AppendChild(option);

    select->SetAttribute("value", "b");
    EXPECT_EQ(select->GetValue(), "");
    EXPECT_EQ(select->GetSelectedIndex(), -1);

    option->SetAttribute("value", "b");

    EXPECT_EQ(select->GetValue(), "b");
    EXPECT_EQ(select->GetSelectedIndex(), 0);
    EXPECT_TRUE(option->GetSelected());
}

} // namespace test
} // namespace mbink
