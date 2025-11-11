/**
 * @file html_loading_example.cpp
 * @brief HTML加载功能演示
 *
 * 演示MBink如何加载和解析HTML文档
 */

#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/lexbor/lexbor_document.h"
#include <iostream>
#include <fstream>

using namespace lightui;

void PrintElement(std::shared_ptr<Element> element, int indent = 0) {
    if (!element) return;
    
    std::string indent_str(indent * 2, ' ');
    std::cout << indent_str << "<" << element->GetTagName();
    
    // 打印属性
    if (element->HasAttribute("id")) {
        std::cout << " id=\"" << element->GetAttribute("id") << "\"";
    }
    if (element->HasAttribute("class")) {
        std::cout << " class=\"" << element->GetAttribute("class") << "\"";
    }
    
    std::cout << ">" << std::endl;
    
    // 打印子节点
    auto children = element->GetChildNodes();
    for (auto& child : children) {
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto child_elem = std::static_pointer_cast<Element>(child);
            PrintElement(child_elem, indent + 1);
        } else if (child->GetNodeType() == NodeType::TEXT_NODE) {
            auto text_node = std::static_pointer_cast<Text>(child);
            std::string text = text_node->GetData();
            // 去除空白
            text.erase(0, text.find_first_not_of(" \n\r\t"));
            text.erase(text.find_last_not_of(" \n\r\t") + 1);
            if (!text.empty()) {
                std::cout << indent_str << "  \"" << text << "\"" << std::endl;
            }
        }
    }
    
    std::cout << indent_str << "</" << element->GetTagName() << ">" << std::endl;
}

int main() {
    std::cout << "=== MBink HTML Loading Example ===" << std::endl << std::endl;
    
    // ========== 示例1: 从字符串加载HTML ==========
    std::cout << "示例1: 从字符串加载HTML" << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    auto doc1 = std::make_shared<Document>();
    
    std::string html1 = R"(
        <!DOCTYPE html>
        <html>
            <head>
                <title>MBink Example</title>
            </head>
            <body>
                <div id="header" class="container">
                    <h1>Welcome to MBink!</h1>
                    <p>A lightweight desktop application framework</p>
                </div>
                <div id="content" class="container">
                    <h2>Features</h2>
                    <ul>
                        <li>HTML/CSS parsing with Lexbor</li>
                        <li>JavaScript runtime with QuickJS</li>
                        <li>Skia rendering engine</li>
                        <li>React ecosystem support (coming soon)</li>
                    </ul>
                </div>
                <div id="footer">
                    <p>© 2025 MBink Team</p>
                </div>
            </body>
        </html>
    )";
    
    if (doc1->LoadHTML(html1)) {
        std::cout << "✅ HTML加载成功!" << std::endl << std::endl;
        
        // 打印文档结构
        std::cout << "文档结构:" << std::endl;
        auto html_elem = doc1->GetDocumentElement();
        if (html_elem) {
            PrintElement(html_elem);
        }
        
        std::cout << std::endl;
        
        // 查询元素
        std::cout << "查询元素:" << std::endl;
        auto header = doc1->GetElementById("header");
        if (header) {
            std::cout << "  找到 #header: " << header->GetTagName() 
                      << " (class=" << header->GetAttribute("class") << ")" << std::endl;
        }
        
        auto content = doc1->GetElementById("content");
        if (content) {
            std::cout << "  找到 #content: " << content->GetTagName() 
                      << " (class=" << content->GetAttribute("class") << ")" << std::endl;
        }
        
        auto footer = doc1->GetElementById("footer");
        if (footer) {
            std::cout << "  找到 #footer: " << footer->GetTagName() << std::endl;
        }
    } else {
        std::cout << "❌ HTML加载失败!" << std::endl;
    }
    
    std::cout << std::endl << std::endl;
    
    // ========== 示例2: 从文件加载HTML ==========
    std::cout << "示例2: 从文件加载HTML" << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    // 创建临时HTML文件
    std::string file_path = "example_page.html";
    std::string html2 = R"(
        <!DOCTYPE html>
        <html lang="zh-CN">
            <head>
                <meta charset="UTF-8">
                <title>Todo App</title>
            </head>
            <body>
                <div id="app">
                    <h1>Todo List</h1>
                    <input type="text" id="todo-input" placeholder="Enter a task...">
                    <button id="add-btn">Add</button>
                    <ul id="todo-list">
                        <li class="todo-item">
                            <input type="checkbox" id="task1">
                            <label for="task1">Learn MBink</label>
                        </li>
                        <li class="todo-item">
                            <input type="checkbox" id="task2">
                            <label for="task2">Build an app</label>
                        </li>
                    </ul>
                </div>
            </body>
        </html>
    )";
    
    // 写入文件
    std::ofstream file(file_path);
    file << html2;
    file.close();
    
    auto doc2 = std::make_shared<Document>();
    
    if (doc2->LoadHTMLFile(file_path)) {
        std::cout << "✅ HTML文件加载成功!" << std::endl << std::endl;
        
        // 查询元素
        std::cout << "查询元素:" << std::endl;
        auto app = doc2->GetElementById("app");
        if (app) {
            std::cout << "  找到 #app: " << app->GetTagName() << std::endl;
            
            // 统计子元素
            auto children = app->GetChildNodes();
            int element_count = 0;
            for (auto& child : children) {
                if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
                    element_count++;
                }
            }
            std::cout << "  #app 包含 " << element_count << " 个子元素" << std::endl;
        }
        
        auto input = doc2->GetElementById("todo-input");
        if (input) {
            std::cout << "  找到 #todo-input: " << input->GetTagName() 
                      << " (placeholder=" << input->GetAttribute("placeholder") << ")" << std::endl;
        }
        
        auto btn = doc2->GetElementById("add-btn");
        if (btn) {
            std::cout << "  找到 #add-btn: " << btn->GetTagName() << std::endl;
        }
        
        auto list = doc2->GetElementById("todo-list");
        if (list) {
            std::cout << "  找到 #todo-list: " << list->GetTagName() << std::endl;
            
            // 统计列表项
            auto items = list->GetChildNodes();
            int item_count = 0;
            for (auto& item : items) {
                if (item->GetNodeType() == NodeType::ELEMENT_NODE) {
                    item_count++;
                }
            }
            std::cout << "  #todo-list 包含 " << item_count << " 个列表项" << std::endl;
        }
    } else {
        std::cout << "❌ HTML文件加载失败!" << std::endl;
    }
    
    // 清理临时文件
    std::remove(file_path.c_str());
    
    std::cout << std::endl << std::endl;
    
    // ========== 示例3: 保存HTML ==========
    std::cout << "示例3: 保存HTML" << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    auto doc3 = std::make_shared<Document>();
    
    std::string html3 = R"(
        <html>
            <body>
                <h1>Hello MBink</h1>
                <p>This is a simple page.</p>
            </body>
        </html>
    )";
    
    if (doc3->LoadHTML(html3)) {
        std::cout << "✅ HTML加载成功!" << std::endl;
        
        // 保存为HTML字符串
        std::string saved_html = doc3->SaveHTML();
        std::cout << std::endl << "保存的HTML:" << std::endl;
        std::cout << saved_html << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    
    return 0;
}

