#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <sys/resource.h>
#endif

namespace mbink {
namespace performance {

// 性能计时器
class PerformanceTimer {
public:
    PerformanceTimer() : running_(false) {}

    void Start() {
        start_time_ = std::chrono::high_resolution_clock::now();
        running_ = true;
    }

    void Stop() {
        end_time_ = std::chrono::high_resolution_clock::now();
        running_ = false;
    }

    double GetElapsedMs() const {
        auto duration = running_ 
            ? std::chrono::high_resolution_clock::now() - start_time_
            : end_time_ - start_time_;
        return std::chrono::duration<double, std::milli>(duration).count();
    }

    double GetElapsedUs() const {
        auto duration = running_ 
            ? std::chrono::high_resolution_clock::now() - start_time_
            : end_time_ - start_time_;
        return std::chrono::duration<double, std::micro>(duration).count();
    }

    double GetElapsedSeconds() const {
        return GetElapsedMs() / 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point end_time_;
    bool running_;
};

// 内存监控器
class MemoryMonitor {
public:
    MemoryMonitor() : start_memory_(0), peak_memory_(0) {}

    void Start() {
        start_memory_ = GetCurrentMemoryUsage();
        peak_memory_ = start_memory_;
    }

    void Stop() {
        size_t current = GetCurrentMemoryUsage();
        if (current > peak_memory_) {
            peak_memory_ = current;
        }
    }

    size_t GetPeakMemoryUsage() const {
        return peak_memory_;
    }

    size_t GetCurrentMemoryUsage() const {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                 (PROCESS_MEMORY_COUNTERS*)&pmc, 
                                 sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss * 1024; // Convert KB to bytes
#endif
    }

    size_t GetMemoryIncrease() const {
        return GetCurrentMemoryUsage() - start_memory_;
    }

    double GetMemoryIncreaseMB() const {
        return GetMemoryIncrease() / (1024.0 * 1024.0);
    }

private:
    size_t start_memory_;
    size_t peak_memory_;
};

// 性能测试结果
struct PerformanceResult {
    std::string name;
    double value;
    std::string unit;
    std::string category;

    PerformanceResult(const std::string& n, double v, const std::string& u, 
                     const std::string& c = "")
        : name(n), value(v), unit(u), category(c) {}
};

// 性能报告生成器
class PerformanceReporter {
public:
    void AddResult(const std::string& name, double value, 
                   const std::string& unit = "ms", 
                   const std::string& category = "") {
        results_.emplace_back(name, value, unit, category);
    }

    void AddResult(const PerformanceResult& result) {
        results_.push_back(result);
    }

    void PrintSummary() const {
        std::cout << "\n=== Performance Test Summary ===\n\n";
        
        std::string current_category;
        for (const auto& result : results_) {
            if (!result.category.empty() && result.category != current_category) {
                current_category = result.category;
                std::cout << "\n[" << current_category << "]\n";
            }
            
            std::cout << std::left << std::setw(40) << result.name 
                     << std::right << std::setw(12) << std::fixed 
                     << std::setprecision(2) << result.value 
                     << " " << result.unit << "\n";
        }
        
        std::cout << "\n================================\n";
    }

    void GenerateReport(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        file << "# Performance Test Report\n\n";
        file << "> **Generated**: " << GetCurrentTimestamp() << "\n\n";
        file << "---\n\n";

        std::string current_category;
        for (const auto& result : results_) {
            if (!result.category.empty() && result.category != current_category) {
                current_category = result.category;
                file << "## " << current_category << "\n\n";
                file << "| Test Name | Value | Unit |\n";
                file << "|-----------|-------|------|\n";
            }
            
            file << "| " << result.name << " | " 
                 << std::fixed << std::setprecision(2) << result.value 
                 << " | " << result.unit << " |\n";
        }

        file << "\n---\n\n";
        file << "**Total Tests**: " << results_.size() << "\n";
        
        file.close();
        std::cout << "Report generated: " << filename << std::endl;
    }

    void GenerateCSV(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        file << "Category,Name,Value,Unit\n";
        for (const auto& result : results_) {
            file << result.category << "," 
                 << result.name << "," 
                 << result.value << "," 
                 << result.unit << "\n";
        }

        file.close();
        std::cout << "CSV generated: " << filename << std::endl;
    }

    void Clear() {
        results_.clear();
    }

    size_t GetResultCount() const {
        return results_.size();
    }

private:
    std::vector<PerformanceResult> results_;

    std::string GetCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// HTML 生成工具
class HTMLGenerator {
public:
    // 生成简单的 HTML（平铺结构）
    static std::string GenerateSimpleHTML(size_t elementCount) {
        std::stringstream ss;
        ss << "<!DOCTYPE html>\n<html>\n<head><title>Test</title></head>\n<body>\n";
        
        for (size_t i = 0; i < elementCount; i++) {
            ss << "<div id=\"elem" << i << "\" class=\"item item-" << (i % 10) 
               << "\" data-index=\"" << i << "\">Element " << i << "</div>\n";
        }
        
        ss << "</body>\n</html>";
        return ss.str();
    }

    // 生成嵌套的 HTML（树形结构）
    static std::string GenerateNestedHTML(size_t depth, size_t width) {
        std::stringstream ss;
        ss << "<!DOCTYPE html>\n<html>\n<head><title>Test</title></head>\n<body>\n";
        GenerateNestedHTMLRecursive(ss, depth, width, 0);
        ss << "</body>\n</html>";
        return ss.str();
    }

    // 生成表单 HTML
    static std::string GenerateFormHTML(size_t inputCount) {
        std::stringstream ss;
        ss << "<!DOCTYPE html>\n<html>\n<head><title>Form Test</title></head>\n<body>\n";
        ss << "<form id=\"testForm\">\n";
        
        for (size_t i = 0; i < inputCount; i++) {
            std::string type = (i % 5 == 0) ? "email" : 
                              (i % 5 == 1) ? "number" : 
                              (i % 5 == 2) ? "checkbox" : 
                              (i % 5 == 3) ? "radio" : "text";
            
            ss << "<input type=\"" << type << "\" name=\"field" << i 
               << "\" id=\"input" << i << "\" value=\"value" << i << "\"";
            
            if (i % 3 == 0) {
                ss << " required";
            }
            
            ss << ">\n";
        }
        
        ss << "</form>\n</body>\n</html>";
        return ss.str();
    }

    // 生成表格 HTML
    static std::string GenerateTableHTML(size_t rows, size_t cols) {
        std::stringstream ss;
        ss << "<!DOCTYPE html>\n<html>\n<head><title>Table Test</title></head>\n<body>\n";
        ss << "<table id=\"testTable\">\n";
        
        for (size_t r = 0; r < rows; r++) {
            ss << "<tr>\n";
            for (size_t c = 0; c < cols; c++) {
                ss << "<td id=\"cell-" << r << "-" << c << "\">Cell " << r << "," << c << "</td>\n";
            }
            ss << "</tr>\n";
        }
        
        ss << "</table>\n</body>\n</html>";
        return ss.str();
    }

    // 生成复杂的 HTML（混合结构）
    static std::string GenerateComplexHTML(size_t elementCount) {
        std::stringstream ss;
        ss << "<!DOCTYPE html>\n<html>\n<head><title>Complex Test</title></head>\n<body>\n";
        
        for (size_t i = 0; i < elementCount / 10; i++) {
            ss << "<section id=\"section" << i << "\" class=\"section\">\n";
            ss << "<h2>Section " << i << "</h2>\n";
            ss << "<ul class=\"list\">\n";
            
            for (size_t j = 0; j < 10; j++) {
                ss << "<li class=\"item\" data-value=\"" << (i * 10 + j) << "\">";
                ss << "<a href=\"#link" << (i * 10 + j) << "\">Link " << j << "</a>";
                ss << "</li>\n";
            }
            
            ss << "</ul>\n</section>\n";
        }
        
        ss << "</body>\n</html>";
        return ss.str();
    }

private:
    static void GenerateNestedHTMLRecursive(std::stringstream& ss, 
                                           size_t depth, 
                                           size_t width, 
                                           size_t level) {
        if (level >= depth) {
            return;
        }
        
        for (size_t i = 0; i < width; i++) {
            ss << "<div class=\"level-" << level << "\" data-level=\"" << level 
               << "\" data-index=\"" << i << "\">\n";
            ss << "Level " << level << " Item " << i << "\n";
            GenerateNestedHTMLRecursive(ss, depth, width, level + 1);
            ss << "</div>\n";
        }
    }
};

} // namespace performance
} // namespace mbink

