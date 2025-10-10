#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

class Utils {
public:
  // Convert string to UpperCamelCase (similar to the Rust function)
  static std::string convertToUpperCamelCase(const std::string &input);

  // Helper function to read file contents
  static std::string readFile(const std::string &filename);

  // Helper function to split string by delimiter
  static std::vector<std::string> split(const std::string &str, char delimiter);

  // Helper function to check if string is numeric
  static bool isNumeric(const std::string &str);

  // Helper function to replace all occurrences in string
  static std::string replace(const std::string &str, const std::string &from,
                             const std::string &to);
};

#endif // UTILS_HPP
