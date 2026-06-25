/**
 * @file test_text_transform_properties.cpp
 * @brief Property-based tests for CSS text-transform property
 * 
 * This file implements property-based testing for text-transform functionality.
 * Each property test runs 100 iterations with randomly generated inputs.
 * 
 * **Validates: Requirements 2.1-2.5**
 */

#include <gtest/gtest.h>
#include "core/render/text/text_transform.h"
#include "core/render/objects/render_object.h"
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

using namespace mblink;

// Random number generator for property tests
class TextTransformPropertyTestRng {
public:
    TextTransformPropertyTestRng() : gen_(std::random_device{}()) {}
    
    int randInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen_);
    }
    
    char randAsciiChar() {
        // Generate printable ASCII characters (32-126)
        return static_cast<char>(randInt(32, 126));
    }
    
    char randAsciiLetter() {
        // Generate ASCII letters only (a-z, A-Z)
        int choice = randInt(0, 51);
        if (choice < 26) {
            return static_cast<char>('a' + choice);
        } else {
            return static_cast<char>('A' + (choice - 26));
        }
    }
    
    std::string randAsciiString(int min_len, int max_len) {
        int len = randInt(min_len, max_len);
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result += randAsciiChar();
        }
        return result;
    }

    std::string randAsciiLetterString(int min_len, int max_len) {
        int len = randInt(min_len, max_len);
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result += randAsciiLetter();
        }
        return result;
    }
    
    std::string randWordsString(int min_words, int max_words) {
        int num_words = randInt(min_words, max_words);
        std::string result;
        for (int i = 0; i < num_words; ++i) {
            if (i > 0) {
                // Add whitespace separator
                int ws_type = randInt(0, 2);
                if (ws_type == 0) result += ' ';
                else if (ws_type == 1) result += '\t';
                else result += "  ";  // Multiple spaces
            }
            // Add a word (lowercase letters)
            int word_len = randInt(1, 10);
            for (int j = 0; j < word_len; ++j) {
                result += static_cast<char>('a' + randInt(0, 25));
            }
        }
        return result;
    }
    
private:
    std::mt19937 gen_;
};

class TextTransformPropertyTest : public ::testing::Test {
protected:
    TextTransformPropertyTestRng rng_;
    static constexpr int NUM_ITERATIONS = 100;
};

/**
 * **Feature: css-basic-interaction-properties, Property 3: Text Transform Preserves Length for Uppercase/Lowercase**
 * 
 * For any ASCII text string, applying uppercase or lowercase text-transform
 * should produce a string of the same length as the original.
 * 
 * **Validates: Requirements 2.1, 2.2**
 */
TEST_F(TextTransformPropertyTest, UppercaseLowercasePreservesLength) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiString(0, 100);
        
        std::string upper = TransformText(input, "uppercase");
        std::string lower = TransformText(input, "lowercase");
        
        EXPECT_EQ(upper.length(), input.length())
            << "Uppercase should preserve length for ASCII text. Input: \"" << input << "\"";
        EXPECT_EQ(lower.length(), input.length())
            << "Lowercase should preserve length for ASCII text. Input: \"" << input << "\"";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 3: Text Transform Preserves Length for Uppercase/Lowercase**
 * 
 * Additional test: Uppercase should convert all lowercase letters to uppercase.
 * 
 * **Validates: Requirements 2.1**
 */
TEST_F(TextTransformPropertyTest, UppercaseConvertsAllLowercase) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiLetterString(1, 50);
        
        std::string upper = TransformText(input, "uppercase");
        
        // Check that all letters are uppercase
        for (size_t j = 0; j < upper.length(); ++j) {
            char c = upper[j];
            if (std::isalpha(static_cast<unsigned char>(c))) {
                EXPECT_TRUE(std::isupper(static_cast<unsigned char>(c)))
                    << "Character at position " << j << " should be uppercase. "
                    << "Input: \"" << input << "\", Output: \"" << upper << "\"";
            }
        }
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 3: Text Transform Preserves Length for Uppercase/Lowercase**
 * 
 * Additional test: Lowercase should convert all uppercase letters to lowercase.
 * 
 * **Validates: Requirements 2.2**
 */
TEST_F(TextTransformPropertyTest, LowercaseConvertsAllUppercase) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiLetterString(1, 50);
        
        std::string lower = TransformText(input, "lowercase");
        
        // Check that all letters are lowercase
        for (size_t j = 0; j < lower.length(); ++j) {
            char c = lower[j];
            if (std::isalpha(static_cast<unsigned char>(c))) {
                EXPECT_TRUE(std::islower(static_cast<unsigned char>(c)))
                    << "Character at position " << j << " should be lowercase. "
                    << "Input: \"" << input << "\", Output: \"" << lower << "\"";
            }
        }
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 4: Text Transform Capitalize First Characters**
 * 
 * For any text string with words separated by whitespace, applying capitalize
 * text-transform should result in each word's first character being uppercase
 * (if it's a letter).
 * 
 * **Validates: Requirements 2.3**
 */
TEST_F(TextTransformPropertyTest, CapitalizeFirstCharacters) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randWordsString(1, 10);
        
        std::string capitalized = TransformText(input, "capitalize");
        
        // Verify first character of each word is uppercase
        bool expect_capital = true;  // First char should be capitalized
        for (size_t j = 0; j < capitalized.length(); ++j) {
            char c = capitalized[j];
            bool is_whitespace = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
            
            if (is_whitespace) {
                expect_capital = true;  // Next non-whitespace should be capitalized
            } else if (expect_capital && std::isalpha(static_cast<unsigned char>(c))) {
                EXPECT_TRUE(std::isupper(static_cast<unsigned char>(c)))
                    << "First letter of word should be uppercase at position " << j
                    << ". Input: \"" << input << "\", Output: \"" << capitalized << "\"";
                expect_capital = false;
            } else {
                expect_capital = false;
            }
        }
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 5: Text Transform None is Identity**
 * 
 * For any text string, applying text-transform: none should produce output
 * identical to the input.
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(TextTransformPropertyTest, NoneIsIdentity) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiString(0, 100);
        
        std::string result = TransformText(input, "none");
        
        EXPECT_EQ(result, input)
            << "text-transform: none should return identical string. "
            << "Input: \"" << input << "\", Output: \"" << result << "\"";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 5: Text Transform None is Identity**
 * 
 * Additional test: Empty transform value should also be identity.
 * 
 * **Validates: Requirements 2.4**
 */
TEST_F(TextTransformPropertyTest, EmptyTransformIsIdentity) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiString(0, 100);
        
        std::string result = TransformText(input, "");
        
        EXPECT_EQ(result, input)
            << "Empty text-transform should return identical string. "
            << "Input: \"" << input << "\", Output: \"" << result << "\"";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 6: Text Transform Does Not Modify DOM**
 * 
 * For any element with text-transform applied, the DOM textContent should
 * remain unchanged after rendering. This test verifies that the TransformText
 * function does not modify its input string.
 * 
 * **Validates: Requirements 2.5**
 */
TEST_F(TextTransformPropertyTest, DoesNotModifyInput) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string original = rng_.randAsciiString(0, 100);
        std::string input = original;  // Copy for comparison
        
        // Apply various transforms
        TransformText(input, "uppercase");
        EXPECT_EQ(input, original)
            << "TransformText should not modify input string (uppercase)";
        
        TransformText(input, "lowercase");
        EXPECT_EQ(input, original)
            << "TransformText should not modify input string (lowercase)";
        
        TransformText(input, "capitalize");
        EXPECT_EQ(input, original)
            << "TransformText should not modify input string (capitalize)";
    }
}

/**
 * **Feature: css-basic-interaction-properties, Property 6: Text Transform Does Not Modify DOM**
 * 
 * Additional test: Verify that RenderText stores original text and only
 * transforms during rendering. This tests the ComputedStyle integration.
 * 
 * **Validates: Requirements 2.5**
 */
TEST_F(TextTransformPropertyTest, RenderTextPreservesOriginalText) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string original = rng_.randAsciiString(1, 50);
        
        // Create a RenderText object
        RenderText render_text(original);
        
        // Set text-transform in computed style
        ComputedStyle& style = render_text.GetComputedStyle();
        style.text_transform = "uppercase";
        
        // Verify the original text is preserved
        EXPECT_EQ(render_text.GetText(), original)
            << "RenderText should preserve original text content. "
            << "Original: \"" << original << "\", GetText(): \"" << render_text.GetText() << "\"";
    }
}

/**
 * Additional property test: Uppercase and lowercase are inverses for ASCII letters.
 * 
 * For any ASCII letter string, uppercase(lowercase(x)) should equal uppercase(x)
 * and lowercase(uppercase(x)) should equal lowercase(x).
 * 
 * **Validates: Requirements 2.1, 2.2**
 */
TEST_F(TextTransformPropertyTest, UpperLowerIdempotence) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::string input = rng_.randAsciiLetterString(0, 50);
        
        std::string upper = TransformText(input, "uppercase");
        std::string lower = TransformText(input, "lowercase");
        
        // uppercase(lowercase(x)) == uppercase(x)
        std::string upper_lower = TransformText(lower, "uppercase");
        EXPECT_EQ(upper_lower, upper)
            << "uppercase(lowercase(x)) should equal uppercase(x). "
            << "Input: \"" << input << "\"";
        
        // lowercase(uppercase(x)) == lowercase(x)
        std::string lower_upper = TransformText(upper, "lowercase");
        EXPECT_EQ(lower_upper, lower)
            << "lowercase(uppercase(x)) should equal lowercase(x). "
            << "Input: \"" << input << "\"";
    }
}
