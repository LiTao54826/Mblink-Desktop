/**
 * @file test_innerHTML_debug.cpp
 * @brief Debug innerHTML functionality
 */

#include <iostream>
#include "core/dom/element.h"
#include "core/dom/document.h"
#include "core/dom/text.h"

using namespace lightui;

int main() {
    // Test 1: Element without document
    std::cout << "Test 1: Element without document\n";
    auto div1 = std::make_shared<Element>("div");
    std::cout << "  Owner document: " << (div1->GetOwnerDocument() ? "exists" : "nullptr") << "\n";
    div1->SetInnerHTML("<p>Hello</p>");
    std::cout << "  Child count: " << div1->GetChildNodes().size() << "\n";
    if (div1->GetChildNodes().size() > 0) {
        auto child = div1->GetChildNodes()[0];
        std::cout << "  Child type: " << static_cast<int>(child->GetNodeType()) << "\n";
    }
    
    // Test 2: Element with document
    std::cout << "\nTest 2: Element with document\n";
    auto doc = std::make_shared<Document>();
    auto div2 = doc->CreateElement("div");
    std::cout << "  Owner document: " << (div2->GetOwnerDocument() ? "exists" : "nullptr") << "\n";
    div2->SetInnerHTML("<p>Hello</p>");
    std::cout << "  Child count: " << div2->GetChildNodes().size() << "\n";
    if (div2->GetChildNodes().size() > 0) {
        auto child = div2->GetChildNodes()[0];
        std::cout << "  Child type: " << static_cast<int>(child->GetNodeType()) << "\n";
        if (child->GetNodeType() == NodeType::ELEMENT_NODE) {
            auto elem = std::dynamic_pointer_cast<Element>(child);
            std::cout << "  Child tag: " << elem->GetTagName() << "\n";
            std::cout << "  Child text: " << elem->GetTextContent() << "\n";
        }
    }
    
    return 0;
}

