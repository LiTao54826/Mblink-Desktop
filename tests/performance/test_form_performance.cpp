#include <gtest/gtest.h>
#include "core/lexbor/lexbor_document.h"
#include "performance_utils.h"
#include <iostream>
#include <regex>

using namespace lightui;
using namespace mbink::performance;

class FormPerformanceTest : public ::testing::Test {
protected:
    PerformanceReporter reporter_;
    
    void SetUp() override {
        std::cout << "\n=== Form Performance Tests ===\n";
    }
    
    void TearDown() override {
        reporter_.PrintSummary();
        reporter_.GenerateReport("docs/form_performance_report.md");
        reporter_.GenerateCSV("docs/form_performance_data.csv");
    }
};

// 测试大量表单元素的解析性能
TEST_F(FormPerformanceTest, ParseLargeForm) {
    std::string html = HTMLGenerator::GenerateFormHTML(5000);
    
    PerformanceTimer timer;
    MemoryMonitor memory;
    
    memory.Start();
    timer.Start();
    
    LexborDocument doc;
    bool success = doc.ParseHTML(html);
    
    timer.Stop();
    memory.Stop();
    
    ASSERT_TRUE(success);
    
    double elapsed = timer.GetElapsedMs();
    double memoryMB = memory.GetMemoryIncreaseMB();
    
    reporter_.AddResult("Parse 5000 Form Inputs", elapsed, "ms", "Form Parsing");
    reporter_.AddResult("Parse 5000 Form Inputs (memory)", memoryMB, "MB", "Memory Usage");
    
    std::cout << "Parse 5000 inputs: " << elapsed << " ms, " << memoryMB << " MB\n";
    
    EXPECT_LT(elapsed, 100.0) << "Parsing large form should be fast (< 100ms)";
}

// 测试表单数据收集性能
TEST_F(FormPerformanceTest, CollectFormData) {
    std::string html = HTMLGenerator::GenerateFormHTML(1000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("form");
    ASSERT_NE(form, nullptr);
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 收集所有表单数据
    std::map<std::string, std::string> data;
    auto inputs = form->QuerySelectorAll("input");
    for (auto* input : inputs) {
        std::string name = input->GetAttribute("name");
        std::string value = input->GetAttribute("value");
        if (!name.empty()) {
            data[name] = value;
        }
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Collect 1000 Form Fields", elapsed, "ms", "Form Operations");
    
    std::cout << "Collect form data: " << elapsed << " ms, " << data.size() << " fields\n";
    
    EXPECT_LT(elapsed, 50.0) << "Collecting form data should be fast (< 50ms)";
    EXPECT_EQ(data.size(), 1000);
}

// 测试表单验证性能
TEST_F(FormPerformanceTest, FormValidation) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
        <form id="testForm">
    )";
    
    // 创建 1000 个需要验证的输入
    for (int i = 0; i < 1000; i++) {
        html += "<input type=\"email\" name=\"email" + std::to_string(i) + 
                "\" value=\"test" + std::to_string(i) + "@example.com\" required>\n";
    }
    
    html += "</form></body></html>";
    
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("form");
    ASSERT_NE(form, nullptr);
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 验证所有输入
    auto inputs = form->QuerySelectorAll("input[required]");
    int validCount = 0;
    std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    
    for (auto* input : inputs) {
        std::string value = input->GetAttribute("value");
        if (!value.empty() && std::regex_match(value, emailRegex)) {
            validCount++;
        }
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Validate 1000 Email Inputs", elapsed, "ms", "Form Validation");
    
    std::cout << "Validate 1000 emails: " << elapsed << " ms, " << validCount << " valid\n";
    
    EXPECT_LT(elapsed, 100.0) << "Form validation should be reasonably fast (< 100ms)";
    EXPECT_EQ(validCount, 1000);
}

// 测试复选框和单选按钮查询性能
TEST_F(FormPerformanceTest, CheckboxRadioQuery) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
        <form id="testForm">
    )";
    
    // 创建 500 个复选框和 500 个单选按钮
    for (int i = 0; i < 500; i++) {
        html += "<input type=\"checkbox\" name=\"check" + std::to_string(i) + 
                "\" value=\"" + std::to_string(i) + "\"";
        if (i % 2 == 0) html += " checked";
        html += ">\n";
        
        html += "<input type=\"radio\" name=\"radio" + std::to_string(i / 10) + 
                "\" value=\"" + std::to_string(i) + "\"";
        if (i % 10 == 0) html += " checked";
        html += ">\n";
    }
    
    html += "</form></body></html>";
    
    LexborDocument doc;
    doc.ParseHTML(html);
    
    PerformanceTimer timer1, timer2;
    
    // 查询所有选中的复选框
    timer1.Start();
    auto checkedBoxes = doc.QuerySelectorAll("input[type='checkbox']:checked");
    timer1.Stop();
    
    // 查询所有选中的单选按钮
    timer2.Start();
    auto checkedRadios = doc.QuerySelectorAll("input[type='radio']:checked");
    timer2.Stop();
    
    double elapsed1 = timer1.GetElapsedMs();
    double elapsed2 = timer2.GetElapsedMs();
    
    reporter_.AddResult("Query Checked Checkboxes (500 total)", elapsed1, "ms", "Form Query");
    reporter_.AddResult("Query Checked Radios (500 total)", elapsed2, "ms", "Form Query");
    
    std::cout << "Query checked checkboxes: " << elapsed1 << " ms, found " << checkedBoxes.size() << "\n";
    std::cout << "Query checked radios: " << elapsed2 << " ms, found " << checkedRadios.size() << "\n";
    
    EXPECT_LT(elapsed1, 50.0) << "Checkbox query should be fast (< 50ms)";
    EXPECT_LT(elapsed2, 50.0) << "Radio query should be fast (< 50ms)";
}

// 测试 Select 元素性能
TEST_F(FormPerformanceTest, SelectElements) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <body>
        <form id="testForm">
    )";
    
    // 创建 100 个 select，每个有 50 个 option
    for (int i = 0; i < 100; i++) {
        html += "<select name=\"select" + std::to_string(i) + "\">\n";
        for (int j = 0; j < 50; j++) {
            html += "<option value=\"" + std::to_string(j) + "\"";
            if (j == 25) html += " selected";
            html += ">Option " + std::to_string(j) + "</option>\n";
        }
        html += "</select>\n";
    }
    
    html += "</form></body></html>";
    
    LexborDocument doc;
    doc.ParseHTML(html);
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 收集所有 select 的选中值
    auto selects = doc.QuerySelectorAll("select");
    std::map<std::string, std::string> selectedValues;
    
    for (auto* select : selects) {
        std::string name = select->GetAttribute("name");
        auto selected = select->QuerySelector("option[selected]");
        if (selected) {
            selectedValues[name] = selected->GetAttribute("value");
        }
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Process 100 Select Elements (50 options each)", elapsed, "ms", "Select Operations");
    
    std::cout << "Process selects: " << elapsed << " ms, " << selectedValues.size() << " values\n";
    
    EXPECT_LT(elapsed, 100.0) << "Select processing should be reasonably fast (< 100ms)";
    EXPECT_EQ(selectedValues.size(), 100);
}

// 测试表单字段批量更新性能
TEST_F(FormPerformanceTest, BulkFieldUpdate) {
    std::string html = HTMLGenerator::GenerateFormHTML(1000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto inputs = doc.QuerySelectorAll("input");
    ASSERT_GT(inputs.size(), 0);
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 批量更新所有输入的值
    for (size_t i = 0; i < inputs.size(); i++) {
        inputs[i]->SetAttribute("value", "updated_" + std::to_string(i));
        inputs[i]->AddClass("updated");
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    double avgPerField = timer.GetElapsedUs() / inputs.size();
    
    reporter_.AddResult("Bulk Update 1000 Fields (total)", elapsed, "ms", "Bulk Operations");
    reporter_.AddResult("Bulk Update (avg per field)", avgPerField, "μs", "Bulk Operations");
    
    std::cout << "Bulk update: " << elapsed << " ms for " << inputs.size() << " fields\n";
    std::cout << "Avg per field: " << avgPerField << " μs\n";
    
    EXPECT_LT(avgPerField, 50.0) << "Bulk update should be efficient (< 50μs per field)";
}

// 测试表单提交数据准备性能
TEST_F(FormPerformanceTest, FormSubmitPreparation) {
    std::string html = HTMLGenerator::GenerateFormHTML(2000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("form");
    ASSERT_NE(form, nullptr);
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 模拟表单提交数据准备
    std::map<std::string, std::string> submitData;
    
    // 文本输入
    auto textInputs = form->QuerySelectorAll("input[type='text'], input[type='email'], input[type='number']");
    for (auto* input : textInputs) {
        std::string name = input->GetAttribute("name");
        std::string value = input->GetAttribute("value");
        if (!name.empty()) {
            submitData[name] = value;
        }
    }
    
    // 复选框
    auto checkboxes = form->QuerySelectorAll("input[type='checkbox']:checked");
    for (auto* checkbox : checkboxes) {
        std::string name = checkbox->GetAttribute("name");
        if (!name.empty()) {
            submitData[name] = "true";
        }
    }
    
    // 单选按钮
    auto radios = form->QuerySelectorAll("input[type='radio']:checked");
    for (auto* radio : radios) {
        std::string name = radio->GetAttribute("name");
        std::string value = radio->GetAttribute("value");
        if (!name.empty()) {
            submitData[name] = value;
        }
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Form Submit Preparation (2000 fields)", elapsed, "ms", "Form Submit");
    
    std::cout << "Submit preparation: " << elapsed << " ms, " << submitData.size() << " fields\n";
    
    EXPECT_LT(elapsed, 150.0) << "Form submit preparation should be fast (< 150ms)";
}

// 测试表单重置性能
TEST_F(FormPerformanceTest, FormReset) {
    std::string html = HTMLGenerator::GenerateFormHTML(1000);
    LexborDocument doc;
    doc.ParseHTML(html);
    
    auto form = doc.QuerySelector("form");
    ASSERT_NE(form, nullptr);
    
    // 先修改所有字段
    auto inputs = form->QuerySelectorAll("input");
    for (auto* input : inputs) {
        input->SetAttribute("value", "modified");
    }
    
    PerformanceTimer timer;
    
    timer.Start();
    
    // 重置所有字段
    for (auto* input : inputs) {
        std::string type = input->GetAttribute("type");
        if (type == "checkbox" || type == "radio") {
            input->RemoveAttribute("checked");
        } else {
            input->SetAttribute("value", "");
        }
    }
    
    timer.Stop();
    
    double elapsed = timer.GetElapsedMs();
    
    reporter_.AddResult("Form Reset (1000 fields)", elapsed, "ms", "Form Reset");
    
    std::cout << "Form reset: " << elapsed << " ms\n";
    
    EXPECT_LT(elapsed, 50.0) << "Form reset should be fast (< 50ms)";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

