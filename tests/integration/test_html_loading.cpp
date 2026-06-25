/**
 * @file test_html_loading.cpp
 * @brief HTML 加载集成测试
 */

#include <gtest/gtest.h>
#include "test_utils/test_helpers.h"
#include "dom/document.h"
#include "dom/element.h"

namespace mblink {
namespace test {

class HTMLLoadingTest : public DOMTestBase {};

// ========== 基本 HTML 加载 ==========

TEST_F(HTMLLoadingTest, LoadBasicHTML) {
    auto doc = CreateDocument();
    bool result = doc->LoadHTML(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Test Page</title>
        </head>
        <body>
            <h1>Hello World</h1>
        </body>
        </html>
    )");

    EXPECT_TRUE(result);
    EXPECT_NE(doc->GetBody(), nullptr);
    EXPECT_NE(doc->GetHead(), nullptr);
}

// ========== 复杂 HTML 结构 ==========

TEST_F(HTMLLoadingTest, LoadComplexHTML) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Complex Page</title>
            <style>
                .container { max-width: 1200px; margin: 0 auto; }
                .header { background: #333; color: white; }
            </style>
        </head>
        <body>
            <header class="header">
                <nav>
                    <ul>
                        <li><a href="/">Home</a></li>
                        <li><a href="/about">About</a></li>
                        <li><a href="/contact">Contact</a></li>
                    </ul>
                </nav>
            </header>
            <main class="container">
                <article>
                    <h1>Article Title</h1>
                    <p>First paragraph.</p>
                    <p>Second paragraph.</p>
                </article>
                <aside>
                    <h2>Sidebar</h2>
                    <ul>
                        <li>Link 1</li>
                        <li>Link 2</li>
                    </ul>
                </aside>
            </main>
            <footer>
                <p>&copy; 2024 Test Site</p>
            </footer>
        </body>
        </html>
    )");

    // 验证结构
    auto nav = doc->GetBody()->QuerySelector("nav");
    EXPECT_NE(nav, nullptr);

    auto links = doc->GetBody()->QuerySelectorAll("nav a");
    EXPECT_EQ(links.size(), 3);

    auto article = doc->GetBody()->QuerySelector("article");
    EXPECT_NE(article, nullptr);

    auto paragraphs = article->QuerySelectorAll("p");
    EXPECT_EQ(paragraphs.size(), 2);
}

// ========== 表单元素 ==========

TEST_F(HTMLLoadingTest, LoadFormElements) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <form id="login-form" action="/login" method="post">
                <div class="form-group">
                    <label for="username">Username:</label>
                    <input type="text" id="username" name="username" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" required>
                </div>
                <div class="form-group">
                    <input type="checkbox" id="remember" name="remember">
                    <label for="remember">Remember me</label>
                </div>
                <div class="form-group">
                    <select name="role">
                        <option value="user">User</option>
                        <option value="admin">Admin</option>
                    </select>
                </div>
                <button type="submit">Login</button>
            </form>
        </body>
        </html>
    )");

    auto form = doc->GetElementById("login-form");
    EXPECT_NE(form, nullptr);
    EXPECT_EQ(form->GetAttribute("action"), "/login");

    auto inputs = form->QuerySelectorAll("input");
    EXPECT_EQ(inputs.size(), 3);

    auto select = form->QuerySelector("select");
    EXPECT_NE(select, nullptr);

    auto options = select->QuerySelectorAll("option");
    EXPECT_EQ(options.size(), 2);
}

// ========== 表格 ==========

TEST_F(HTMLLoadingTest, LoadTable) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <table id="data-table">
                <thead>
                    <tr>
                        <th>ID</th>
                        <th>Name</th>
                        <th>Email</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>1</td>
                        <td>Alice</td>
                        <td>alice@example.com</td>
                    </tr>
                    <tr>
                        <td>2</td>
                        <td>Bob</td>
                        <td>bob@example.com</td>
                    </tr>
                    <tr>
                        <td>3</td>
                        <td>Charlie</td>
                        <td>charlie@example.com</td>
                    </tr>
                </tbody>
            </table>
        </body>
        </html>
    )");

    auto table = doc->GetElementById("data-table");
    EXPECT_NE(table, nullptr);

    auto headerCells = table->QuerySelectorAll("thead th");
    EXPECT_EQ(headerCells.size(), 3);

    auto rows = table->QuerySelectorAll("tbody tr");
    EXPECT_EQ(rows.size(), 3);

    auto cells = table->QuerySelectorAll("tbody td");
    EXPECT_EQ(cells.size(), 9);
}

// ========== 内联样式 ==========

TEST_F(HTMLLoadingTest, LoadInlineStyles) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <div id="styled" style="color: red; font-size: 16px; margin: 10px 20px;">
                Styled content
            </div>
        </body>
        </html>
    )");

    auto elem = doc->GetElementById("styled");
    EXPECT_NE(elem, nullptr);

    EXPECT_EQ(elem->GetStyle("color"), "red");
    EXPECT_EQ(elem->GetStyle("font-size"), "16px");
}

// ========== data 属性 ==========

TEST_F(HTMLLoadingTest, LoadDataAttributes) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <div id="data-elem"
                 data-user-id="123"
                 data-role="admin"
                 data-active="true">
                Data element
            </div>
        </body>
        </html>
    )");

    auto elem = doc->GetElementById("data-elem");
    EXPECT_NE(elem, nullptr);

    EXPECT_EQ(elem->GetAttribute("data-user-id"), "123");
    EXPECT_EQ(elem->GetAttribute("data-role"), "admin");
    EXPECT_EQ(elem->GetAttribute("data-active"), "true");
}

// ========== 特殊字符 ==========

TEST_F(HTMLLoadingTest, LoadSpecialCharacters) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <div id="special">
                &lt;div&gt; &amp; &quot;quotes&quot; &apos;apostrophe&apos;
            </div>
            <div id="unicode">你好世界 🌍 🌎 🌏</div>
        </body>
        </html>
    )");

    auto special = doc->GetElementById("special");
    EXPECT_NE(special, nullptr);
    auto text = special->GetTextContent();
    EXPECT_TRUE(text.find("<div>") != std::string::npos);
    EXPECT_TRUE(text.find("&") != std::string::npos);

    auto unicode = doc->GetElementById("unicode");
    EXPECT_NE(unicode, nullptr);
    EXPECT_TRUE(unicode->GetTextContent().find("你好") != std::string::npos);
}

// ========== 自闭合标签 ==========

TEST_F(HTMLLoadingTest, LoadSelfClosingTags) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <img src="image.png" alt="Test Image" />
            <br />
            <hr />
            <input type="text" />
            <meta name="test" content="value" />
        </body>
        </html>
    )");

    auto img = doc->GetBody()->QuerySelector("img");
    EXPECT_NE(img, nullptr);
    EXPECT_EQ(img->GetAttribute("src"), "image.png");

    auto input = doc->GetBody()->QuerySelector("input");
    EXPECT_NE(input, nullptr);
}

// ========== 注释 ==========

TEST_F(HTMLLoadingTest, LoadWithComments) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <!-- This is a comment -->
            <div id="content">
                <!-- Another comment -->
                Content here
            </div>
            <!-- Final comment -->
        </body>
        </html>
    )");

    auto content = doc->GetElementById("content");
    EXPECT_NE(content, nullptr);
    // 注释不应该影响 DOM 结构
}

// ========== 格式错误的 HTML ==========

TEST_F(HTMLLoadingTest, LoadMalformedHTML) {
    auto doc = CreateDocument();

    // 缺少闭合标签
    bool result1 = doc->LoadHTML("<html><body><div><p>Unclosed");
    EXPECT_TRUE(result1);  // HTML5 解析器应该能处理

    // 多余的闭合标签
    bool result2 = doc->LoadHTML("<html><body><div></div></div></div></body></html>");
    EXPECT_TRUE(result2);

    // 嵌套错误
    bool result3 = doc->LoadHTML("<html><body><p><div>Wrong nesting</p></div></body></html>");
    EXPECT_TRUE(result3);
}

// ========== 空 HTML ==========

TEST_F(HTMLLoadingTest, LoadEmptyHTML) {
    auto doc = CreateDocument();
    bool result = doc->LoadHTML("");

    // 空 HTML 应该创建基本结构
    EXPECT_NE(doc->GetBody(), nullptr);
}

// ========== 保存 HTML ==========

TEST_F(HTMLLoadingTest, SaveHTML) {
    auto doc = CreateDocument();
    doc->LoadHTML(R"(
        <html>
        <body>
            <div id="test" class="container">
                <p>Hello World</p>
            </div>
        </body>
        </html>
    )");

    // 修改 DOM
    auto div = doc->GetElementById("test");
    div->AddClass("modified");
    div->SetAttribute("data-modified", "true");

    // 保存 HTML
    std::string html = doc->SaveHTML();

    // 验证修改被保存
    EXPECT_TRUE(html.find("modified") != std::string::npos);
    EXPECT_TRUE(html.find("data-modified") != std::string::npos);
}

} // namespace test
} // namespace mblink
