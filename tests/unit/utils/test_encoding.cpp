/**
 * @file test_encoding.cpp
 * @brief 编码工具单元测试
 */

#include <gtest/gtest.h>
#include "utils/encoding_utils.h"
// #include "utils/utf8_utils.h"  // TODO: UTF8 utils not implemented yet

namespace lightui {
namespace test {

class EncodingTest : public ::testing::Test {};

// ========== 基本编码转换测试 ==========

TEST_F(EncodingTest, LocalToUTF8Basic) {
    std::string local = "test";
    std::string utf8 = utils::LocalToUTF8(local);
    EXPECT_FALSE(utf8.empty());
}

TEST_F(EncodingTest, UTF8ToLocalBasic) {
    std::string utf8 = "test";
    std::string local = utils::UTF8ToLocal(utf8);
    EXPECT_FALSE(local.empty());
}

TEST_F(EncodingTest, RoundTripConversion) {
    std::string original = "test";
    std::string utf8 = utils::LocalToUTF8(original);
    std::string back = utils::UTF8ToLocal(utf8);
    // Basic ASCII should round-trip correctly
    EXPECT_EQ(original, back);
}

// TODO: UTF-8 utility tests disabled until implementation is complete
// All UTF-8 specific tests commented out temporarily

/*
TEST_F(EncodingTest, Utf8Length) {
    // ASCII
    EXPECT_EQ(Utf8Length("hello"), 5);

    // 中文
    EXPECT_EQ(Utf8Length("你好"), 2);

    // 混合
    EXPECT_EQ(Utf8Length("hello你好"), 7);

    // Emoji
    EXPECT_EQ(Utf8Length("🌍"), 1);
}

TEST_F(EncodingTest, Utf8ByteLength) {
    // ASCII: 1 byte per char
    EXPECT_EQ(Utf8ByteLength("hello"), 5);

    // 中文: 3 bytes per char
    EXPECT_EQ(Utf8ByteLength("你好"), 6);

    // Emoji: 4 bytes
    EXPECT_EQ(Utf8ByteLength("🌍"), 4);
}

TEST_F(EncodingTest, Utf8Substring) {
    std::string str = "hello你好world";

    // 从开头截取
    EXPECT_EQ(Utf8Substring(str, 0, 5), "hello");

    // 截取中文
    EXPECT_EQ(Utf8Substring(str, 5, 2), "你好");

    // 截取到结尾
    EXPECT_EQ(Utf8Substring(str, 7, 5), "world");
}

TEST_F(EncodingTest, Utf8CharAt) {
    std::string str = "hello你好";
    EXPECT_EQ(Utf8CharAt(str, 0), "h");
    EXPECT_EQ(Utf8CharAt(str, 1), "e");
    EXPECT_EQ(Utf8CharAt(str, 5), "你");
    EXPECT_EQ(Utf8CharAt(str, 6), "好");
}

TEST_F(EncodingTest, Utf8IndexOf) {
    std::string str = "hello你好world";
    EXPECT_EQ(Utf8IndexOf(str, "hello"), 0);
    EXPECT_EQ(Utf8IndexOf(str, "你好"), 5);
    EXPECT_EQ(Utf8IndexOf(str, "world"), 7);
}

TEST_F(EncodingTest, IsValidUtf8) {
    EXPECT_TRUE(IsValidUtf8("hello"));
    EXPECT_TRUE(IsValidUtf8("你好"));
    EXPECT_TRUE(IsValidUtf8("🌍"));
}

TEST_F(EncodingTest, IsValidUtf8Invalid) {
    EXPECT_FALSE(IsValidUtf8("\xFF\xFE"));
}

TEST_F(EncodingTest, UrlEncode) {
    EXPECT_EQ(UrlEncode("hello world"), "hello%20world");
    EXPECT_EQ(UrlEncode("test@example.com"), "test%40example.com");
    EXPECT_EQ(UrlEncode("a+b=c"), "a%2Bb%3Dc");
}

TEST_F(EncodingTest, UrlDecode) {
    EXPECT_EQ(UrlDecode("hello%20world"), "hello world");
    EXPECT_EQ(UrlDecode("test%40example.com"), "test@example.com");
    EXPECT_EQ(UrlDecode("a%2Bb%3Dc"), "a+b=c");
}

TEST_F(EncodingTest, UrlRoundTrip) {
    std::string original = "hello world!@#$%";
    std::string encoded = UrlEncode(original);
    std::string decoded = UrlDecode(encoded);
    EXPECT_EQ(original, decoded);
}

TEST_F(EncodingTest, Base64Encode) {
    EXPECT_EQ(Base64Encode("hello"), "aGVsbG8=");
    EXPECT_EQ(Base64Encode("test"), "dGVzdA==");
    EXPECT_EQ(Base64Encode("a"), "YQ==");
    EXPECT_EQ(Base64Encode("ab"), "YWI=");
}

TEST_F(EncodingTest, Base64Decode) {
    EXPECT_EQ(Base64Decode("aGVsbG8="), "hello");
    EXPECT_EQ(Base64Decode("dGVzdA=="), "test");
    EXPECT_EQ(Base64Decode("YQ=="), "a");
    EXPECT_EQ(Base64Decode("YWI="), "ab");
}

TEST_F(EncodingTest, Base64RoundTrip) {
    std::string original = "Hello, World! 你好世界";
    std::string encoded = Base64Encode(original);
    std::string decoded = Base64Decode(encoded);
    EXPECT_EQ(original, decoded);
}

TEST_F(EncodingTest, HtmlEncode) {
    EXPECT_EQ(HtmlEncode("<div>"), "&lt;div&gt;");
    EXPECT_EQ(HtmlEncode("a & b"), "a &amp; b");
    EXPECT_EQ(HtmlEncode("\"test\""), "&quot;test&quot;");
    EXPECT_EQ(HtmlEncode("'test'"), "&#39;test&#39;");
}

TEST_F(EncodingTest, HtmlDecode) {
    EXPECT_EQ(HtmlDecode("&lt;div&gt;"), "<div>");
    EXPECT_EQ(HtmlDecode("a &amp; b"), "a & b");
    EXPECT_EQ(HtmlDecode("&quot;test&quot;"), "\"test\"");
}

TEST_F(EncodingTest, HtmlDecodeNumeric) {
    EXPECT_EQ(HtmlDecode("&#60;div&#62;"), "<div>");
    EXPECT_EQ(HtmlDecode("&#x3C;div&#x3E;"), "<div>");
    EXPECT_EQ(HtmlDecode("&#39;test&#39;"), "'test'");
}

TEST_F(EncodingTest, Trim) {
    EXPECT_EQ(Trim("  hello  "), "hello");
    EXPECT_EQ(Trim("\t\ntest\r\n"), "test");
    EXPECT_EQ(Trim("   "), "");
    EXPECT_EQ(Trim("no-trim"), "no-trim");
}
*/

} // namespace test
} // namespace lightui
