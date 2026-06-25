/**
 * @file text_transform.h
 * @brief CSS text-transform property implementation
 * 
 * Provides text transformation functions for CSS text-transform property:
 * - none: no transformation
 * - uppercase: convert all characters to uppercase
 * - lowercase: convert all characters to lowercase
 * - capitalize: capitalize the first character of each word
 * 
 * Handles UTF-8 text properly.
 */

#pragma once

#include <string>
#include <cstdint>

namespace mblink {

/**
 * @brief Transform text according to CSS text-transform property value
 * 
 * @param text The input text (UTF-8 encoded)
 * @param transform The text-transform value: "none", "uppercase", "lowercase", "capitalize"
 * @return Transformed text (UTF-8 encoded)
 * 
 * Note: This function transforms text for rendering purposes only.
 * The original DOM text content should remain unchanged.
 */
std::string TransformText(const std::string& text, const std::string& transform);

/**
 * @brief Convert text to uppercase
 * 
 * @param text The input text (UTF-8 encoded)
 * @return Uppercase text (UTF-8 encoded)
 * 
 * Note: For ASCII characters, this preserves string length.
 * For non-ASCII characters, the behavior depends on the character.
 */
std::string ToUpperCase(const std::string& text);

/**
 * @brief Convert text to lowercase
 * 
 * @param text The input text (UTF-8 encoded)
 * @return Lowercase text (UTF-8 encoded)
 * 
 * Note: For ASCII characters, this preserves string length.
 * For non-ASCII characters, the behavior depends on the character.
 */
std::string ToLowerCase(const std::string& text);

/**
 * @brief Capitalize the first character of each word
 * 
 * A word is defined as a sequence of characters following whitespace
 * or at the beginning of the string.
 * 
 * @param text The input text (UTF-8 encoded)
 * @return Capitalized text (UTF-8 encoded)
 */
std::string Capitalize(const std::string& text);

} // namespace mblink
