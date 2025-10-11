#include "utils.hpp"

#include <fstream>
#include <stdexcept>

std::string Utils::convertToUpperCamelCase(const std::string& input) {
	std::string result;
	bool		foundUnderscore = false;
	std::string cleanInput		= replace(input, "minecraft:", "");

	for (size_t i = 0; i < cleanInput.length(); ++i) {
		char c = cleanInput[i];
		if (c == '_') {
			foundUnderscore = true;
			continue; // Skip underscores
		}

		if (i == 0 || foundUnderscore) {
			result += std::toupper(c);
			foundUnderscore = false;
		} else {
			result += c;
		}
	}

	return result;
}

std::string Utils::convertToCamelCase(const std::string& input) {
	std::string upperCamel = convertToUpperCamelCase(input);
	if (!upperCamel.empty()) {
		upperCamel[0] = std::tolower(upperCamel[0]);
	}
	return upperCamel;
}

std::string Utils::readFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	std::ostringstream content;
	content << file.rdbuf();
	return content.str();
}

std::vector<std::string> Utils::split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream		 ss(str);
	std::string				 token;

	while (std::getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

bool Utils::isNumeric(const std::string& str) {
	if (str.empty()) return false;

	for (char c : str) {
		if (!std::isdigit(c)) {
			return false;
		}
	}

	try {
		int value = std::stoi(str);
		return value >= 0 && value <= 255; // Check if it's in uint8 range
	} catch (...) {
		return false;
	}
}

std::string Utils::replace(const std::string& str, const std::string& from, const std::string& to) {
	std::string result = str;
	size_t		pos	   = 0;

	while ((pos = result.find(from, pos)) != std::string::npos) {
		result.replace(pos, from.length(), to);
		pos += to.length();
	}

	return result;
}
