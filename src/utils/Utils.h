#pragma once

/**
 * @brief Convert a string to uppercase
 * 
 * @param str Input string
 * @return char* Uppercase string (must be freed)
 */
char* ToUpper(const char* str);

/**
 * @brief Convert a string to lowercase
 * 
 * @param str Input string
 * @return char* Lowercase string (must be freed)
 */
char* ToLower(const char* str);

/**
 * @brief Trim whitespace from both ends of a string
 * 
 * @param str Input string
 * @return char* Trimmed string (must be freed)
 */
char* Trim(const char* str);

/**
 * @brief Generate a simple hash from a string
 * 
 * @param str Input string
 * @return char* Hash string (must be freed)
 */
char* HashString(const char* str);
