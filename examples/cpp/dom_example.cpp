// LightUI DOM API C++ 示例
// 演示如何使用 DOM API 构建和操作 DOM 树

#include <iostream>
#include <memory>
#include "core/dom/document.h"
#include "core/dom/element.h"
#include "core/dom/text.h"
#include "core/dom/event.h"

using namespace lightui;

// ========== 示例 1: 创建简单的 DOM 结构 ==========

void Example1_CreateSimpleDOM() {
    std::cout << "=== 示例 1: 创建简单的 DOM 结构 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    
    // 创建容器
    auto container = doc->CreateElement("div");
    container->SetAttribute("id", "container");
    container->SetAttribute("class", "main-container");
    
    // 创建标题
    auto title = doc->CreateElement("h1");
    title->SetAttribute("id", "title");
    title->AppendChild(doc->CreateTextNode("Welcome to LightUI"));
    
    // 创建段落
    auto paragraph = doc->CreateElement("p");
    paragraph->SetAttribute("class", "intro");
    paragraph->AppendChild(doc->CreateTextNode("This is a lightweight UI framework."));
    
    // 组装 DOM 树
    container->AppendChild(title);
    container->AppendChild(paragraph);
    
    std::cout << "Container ID: " << container->GetAttribute("id") << std::endl;
    std::cout << "Container class: " << container->GetAttribute("class") << std::endl;
    std::cout << std::endl;
}

// ========== 示例 2: 属性操作 ==========

void Example2_AttributeOperations() {
    std::cout << "=== 示例 2: 属性操作 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    auto button = doc->CreateElement("button");
    
    // 设置属性
    button->SetAttribute("id", "myButton");
    button->SetAttribute("class", "btn btn-primary");
    button->SetAttribute("data-action", "submit");
    button->SetAttribute("data-target", "#form");
    
    // 读取属性
    std::cout << "Button ID: " << button->GetAttribute("id") << std::endl;
    std::cout << "Button class: " << button->GetAttribute("class") << std::endl;
    std::cout << "Data action: " << button->GetAttribute("data-action") << std::endl;
    std::cout << "Data target: " << button->GetAttribute("data-target") << std::endl;
    
    // 检查属性
    if (button->HasAttribute("id")) {
        std::cout << "Button has ID attribute" << std::endl;
    }
    
    // 删除属性
    button->RemoveAttribute("data-target");
    std::cout << "After removing data-target: " << button->GetAttribute("data-target") << std::endl;
    std::cout << std::endl;
}

// ========== 示例 3: 构建列表 ==========

void Example3_BuildList() {
    std::cout << "=== 示例 3: 构建列表 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    auto list = doc->CreateElement("ul");
    list->SetAttribute("id", "itemList");
    list->SetAttribute("class", "list");
    
    // 创建 5 个列表项
    for (int i = 1; i <= 5; i++) {
        auto item = doc->CreateElement("li");
        item->SetAttribute("class", "list-item");
        item->SetAttribute("id", "item-" + std::to_string(i));
        item->AppendChild(doc->CreateTextNode("Item " + std::to_string(i)));
        list->AppendChild(item);
    }
    
    std::cout << "Created list with " << list->GetChildNodes().size() << " items" << std::endl;
    std::cout << std::endl;
}

// ========== 示例 4: 查询元素 ==========

void Example4_QueryElements() {
    std::cout << "=== 示例 4: 查询元素 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    
    // 创建一些元素
    auto container = doc->CreateElement("div");
    container->SetAttribute("id", "container");
    
    auto title = doc->CreateElement("h1");
    title->SetAttribute("id", "title");
    title->SetAttribute("class", "heading");
    container->AppendChild(title);
    
    auto para1 = doc->CreateElement("p");
    para1->SetAttribute("class", "text");
    container->AppendChild(para1);
    
    auto para2 = doc->CreateElement("p");
    para2->SetAttribute("class", "text");
    container->AppendChild(para2);
    
    // 通过 ID 查询（最快）
    auto foundTitle = doc->GetElementById("title");
    if (foundTitle) {
        std::cout << "Found title by ID: " << foundTitle->GetAttribute("id") << std::endl;
    }
    
    // 通过 QuerySelector 查询
    auto foundHeading = container->QuerySelector(".heading");
    if (foundHeading) {
        std::cout << "Found heading by class" << std::endl;
    }
    
    // 查询所有段落
    auto paragraphs = container->QuerySelectorAll(".text");
    std::cout << "Found " << paragraphs.size() << " paragraphs" << std::endl;
    std::cout << std::endl;
}

// ========== 示例 5: 事件处理 ==========

void Example5_EventHandling() {
    std::cout << "=== 示例 5: 事件处理 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    auto button = doc->CreateElement("button");
    button->SetAttribute("id", "myButton");
    
    // 添加点击事件监听器
    button->AddEventListener("click", [](std::shared_ptr<Event> e) {
        std::cout << "Button clicked!" << std::endl;
        std::cout << "Event type: " << e->GetType() << std::endl;
    });
    
    // 添加鼠标悬停事件
    button->AddEventListener("mouseover", [](std::shared_ptr<Event> e) {
        std::cout << "Mouse over button" << std::endl;
    });
    
    // 触发点击事件
    auto clickEvent = std::make_shared<Event>("click", true);
    button->DispatchEvent(clickEvent);
    
    // 触发鼠标悬停事件
    auto mouseEvent = std::make_shared<Event>("mouseover", true);
    button->DispatchEvent(mouseEvent);
    
    std::cout << std::endl;
}

// ========== 示例 6: 事件冒泡 ==========

void Example6_EventBubbling() {
    std::cout << "=== 示例 6: 事件冒泡 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    
    // 创建嵌套结构
    auto outer = doc->CreateElement("div");
    outer->SetAttribute("id", "outer");
    
    auto middle = doc->CreateElement("div");
    middle->SetAttribute("id", "middle");
    
    auto inner = doc->CreateElement("div");
    inner->SetAttribute("id", "inner");
    
    middle->AppendChild(inner);
    outer->AppendChild(middle);
    
    // 在每一层添加事件监听器
    outer->AddEventListener("click", [](std::shared_ptr<Event> e) {
        auto target = std::dynamic_pointer_cast<Element>(e->GetCurrentTarget());
        std::cout << "Outer div clicked, current target: " << target->GetAttribute("id") << std::endl;
    });
    
    middle->AddEventListener("click", [](std::shared_ptr<Event> e) {
        auto target = std::dynamic_pointer_cast<Element>(e->GetCurrentTarget());
        std::cout << "Middle div clicked, current target: " << target->GetAttribute("id") << std::endl;
    });
    
    inner->AddEventListener("click", [](std::shared_ptr<Event> e) {
        auto target = std::dynamic_pointer_cast<Element>(e->GetCurrentTarget());
        std::cout << "Inner div clicked, current target: " << target->GetAttribute("id") << std::endl;
    });
    
    // 在最内层触发事件
    auto event = std::make_shared<Event>("click", true);  // bubbles = true
    inner->DispatchEvent(event);
    
    std::cout << std::endl;
}

// ========== 示例 7: 卡片网格 ==========

void Example7_CardGrid() {
    std::cout << "=== 示例 7: 卡片网格 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    auto grid = doc->CreateElement("div");
    grid->SetAttribute("id", "cardGrid");
    grid->SetAttribute("class", "grid");
    
    // 创建 3x3 卡片网格
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            auto card = doc->CreateElement("div");
            card->SetAttribute("class", "card");
            card->SetAttribute("id", "card-" + std::to_string(row) + "-" + std::to_string(col));
            
            auto cardTitle = doc->CreateElement("h3");
            cardTitle->AppendChild(doc->CreateTextNode("Card " + std::to_string(row * 3 + col + 1)));
            
            auto cardBody = doc->CreateElement("p");
            cardBody->AppendChild(doc->CreateTextNode("This is card content"));
            
            card->AppendChild(cardTitle);
            card->AppendChild(cardBody);
            grid->AppendChild(card);
        }
    }
    
    std::cout << "Created 3x3 card grid with " << grid->GetChildNodes().size() << " cards" << std::endl;
    
    // 查询所有卡片
    auto allCards = grid->QuerySelectorAll(".card");
    std::cout << "Found " << allCards.size() << " cards using QuerySelectorAll" << std::endl;
    
    // 查询特定卡片
    auto specificCard = grid->QuerySelector("#card-1-1");
    if (specificCard) {
        std::cout << "Found specific card: " << specificCard->GetAttribute("id") << std::endl;
    }
    
    std::cout << std::endl;
}

// ========== 示例 8: innerHTML ==========

void Example8_InnerHTML() {
    std::cout << "=== 示例 8: innerHTML ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    auto div = doc->CreateElement("div");
    
    auto h1 = doc->CreateElement("h1");
    h1->AppendChild(doc->CreateTextNode("Title"));
    
    auto p = doc->CreateElement("p");
    p->SetAttribute("class", "intro");
    p->AppendChild(doc->CreateTextNode("Introduction"));
    
    div->AppendChild(h1);
    div->AppendChild(p);
    
    // 获取 HTML 字符串
    std::string html = div->GetInnerHTML();
    std::cout << "innerHTML: " << html << std::endl;
    std::cout << std::endl;
}

// ========== 示例 9: 克隆节点 ==========

void Example9_CloneNode() {
    std::cout << "=== 示例 9: 克隆节点 ===" << std::endl;
    
    auto doc = std::make_shared<Document>();
    
    // 创建一个复杂的元素
    auto original = doc->CreateElement("div");
    original->SetAttribute("id", "original");
    original->SetAttribute("class", "container");
    
    auto child1 = doc->CreateElement("span");
    child1->AppendChild(doc->CreateTextNode("Child 1"));
    
    auto child2 = doc->CreateElement("span");
    child2->AppendChild(doc->CreateTextNode("Child 2"));
    
    original->AppendChild(child1);
    original->AppendChild(child2);
    
    // 浅克隆（不包括子节点）
    auto shallowClone = std::dynamic_pointer_cast<Element>(original->CloneNode(false));
    shallowClone->SetAttribute("id", "shallowClone");
    std::cout << "Shallow clone children: " << shallowClone->GetChildNodes().size() << std::endl;
    
    // 深克隆（包括所有子节点）
    auto deepClone = std::dynamic_pointer_cast<Element>(original->CloneNode(true));
    deepClone->SetAttribute("id", "deepClone");
    std::cout << "Deep clone children: " << deepClone->GetChildNodes().size() << std::endl;
    
    std::cout << std::endl;
}

// ========== 主函数 ==========

int main() {
    std::cout << "LightUI DOM API C++ 示例" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << std::endl;
    
    Example1_CreateSimpleDOM();
    Example2_AttributeOperations();
    Example3_BuildList();
    Example4_QueryElements();
    Example5_EventHandling();
    Example6_EventBubbling();
    Example7_CardGrid();
    Example8_InnerHTML();
    Example9_CloneNode();
    
    std::cout << "=== 所有示例完成 ===" << std::endl;
    std::cout << "演示了以下功能:" << std::endl;
    std::cout << "1. 创建元素和文本节点" << std::endl;
    std::cout << "2. 设置和获取属性" << std::endl;
    std::cout << "3. 构建 DOM 树" << std::endl;
    std::cout << "4. 查询元素 (GetElementById, QuerySelector, QuerySelectorAll)" << std::endl;
    std::cout << "5. 事件处理和事件冒泡" << std::endl;
    std::cout << "6. 创建网格布局" << std::endl;
    std::cout << "7. innerHTML 操作" << std::endl;
    std::cout << "8. 克隆节点" << std::endl;
    
    return 0;
}

