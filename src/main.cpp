#include "dataGenerator.hpp"

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

DataGenerator::DataGenerator(const std::string& outputDir) : outputDirectory(outputDir) { ensureOutputDirectory(); }

void DataGenerator::ensureOutputDirectory() { std::filesystem::create_directory(outputDirectory); }

std::string readFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	std::ostringstream content;
	content << file.rdbuf();
	return content.str();
}

json DataGenerator::loadJsonFile(const std::string& filename) {
	try {
		std::string content = readFile(filename);
		return json::parse(content);
	} catch (const std::exception& e) {
		throw std::runtime_error("Failed to load JSON file: " + filename + " - " + e.what());
	}
}

void DataGenerator::generateAll() {
	json registriesJson = DataGenerator::loadJsonFile("generated/reports/registries.json");

	// Parse registries and build the data structure
	std::unordered_map<std::string, Registry> registries;

	for (auto& [registryName, registryData] : registriesJson.items()) {
		if (!registryData.is_object() || !registryData.contains("entries")) {
			continue; // Skip invalid entries
		}

		Registry registry;

		// Get registry protocol_id if it exists
		if (registryData.contains("protocol_id")) {
			registry.protocol_id = registryData["protocol_id"].get<int>();
		} else {
			registry.protocol_id = 0; // Default value
		}

		// Parse entries
		auto entries = registryData["entries"];
		for (auto& [entryName, entryData] : entries.items()) {
			if (!entryData.is_object() || !entryData.contains("protocol_id")) {
				continue; // Skip invalid entries
			}

			RegistryEntry entry;
			entry.name		  = entryName;
			entry.protocol_id = entryData["protocol_id"].get<int>();
			registry.entries.push_back(entry);
		}

		registries[registryName] = registry;
	}

	// Add variant registries from data/minecraft folders
	addVariantRegistries(registries);

	// Generate the header file
	generateRegistryHeader(registries);

	// Parse and generate tags
	parseTagsDirectory();
}

void DataGenerator::addVariantRegistries(std::unordered_map<std::string, Registry>& registries) {
	std::string dataPath = "generated/data/minecraft";

	try {
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			if (!entry.is_directory()) continue;

			std::string folderName = entry.path().filename().string();

			// Check if folder name ends with "_variant"
			if (folderName.length() > 8 && folderName.substr(folderName.length() - 8) == "_variant") {
				Registry registry;
				registry.protocol_id = 0; // Default protocol_id for variant registries

				// Scan for JSON files in this variant folder
				int entryProtocolId = 0;
				for (const auto& jsonEntry : std::filesystem::directory_iterator(entry.path())) {
					if (jsonEntry.is_regular_file() && jsonEntry.path().extension() == ".json") {
						RegistryEntry registryEntry;
						registryEntry.name		  = jsonEntry.path().stem().string();
						registryEntry.protocol_id = entryProtocolId++;
						registry.entries.push_back(registryEntry);
					}
				}

				// Only add registry if it has entries
				if (!registry.entries.empty()) {
					std::string registryName = "minecraft:" + folderName;
					registries[registryName] = registry;
					std::cout << "Added variant registry: " << registryName << " with " << registry.entries.size() << " entries\n";
				}
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Warning: Failed to scan variant folders: " << e.what() << "\n";
	}
}

void DataGenerator::generateRegistryHeader(const std::unordered_map<std::string, Registry>& registries) {
	std::string	  headerPath = outputDirectory + "/minecraft_registries.h";
	std::ofstream out(headerPath);

	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}

	// Write header
	out << "#pragma once\n";
	out << "#include <unordered_map>\n";
	out << "#include <vector>\n";
	out << "#include <string>\n\n";

	// Write structures
	out << "struct RegistryEntry {\n";
	out << "    std::string name;\n";
	out << "    int protocol_id;\n";
	out << "};\n\n";

	out << "struct Registry {\n";
	out << "    std::vector<RegistryEntry> entries;\n";
	out << "    int protocol_id;\n";
	out << "};\n\n";

	// Write the main hashmap
	out << "const std::unordered_map<std::string, Registry> MINECRAFT_REGISTRIES = {\n";

	auto regIt = registries.begin();
	for (size_t i = 0; i < registries.size(); ++i, ++regIt) {
		out << "    {\"" << regIt->first << "\", {\n";

		// Write entries array
		out << "        {\n";
		const auto& entries = regIt->second.entries;
		for (size_t j = 0; j < entries.size(); ++j) {
			out << "            {\"" << entries[j].name << "\", " << entries[j].protocol_id << "}";
			if (j < entries.size() - 1) out << ",";
			out << "\n";
		}
		out << "        },\n";

		// Write registry protocol_id
		out << "        " << regIt->second.protocol_id << "\n";
		out << "    }}";

		if (i < registries.size() - 1) out << ",";
		out << "\n";
	}
	out << "};\n\n";

	// Write helper functions
	writeHelperFunctions(out);

	out.close();
	std::cout << "Generated " << headerPath << " with " << registries.size() << " registries\n";
}

void DataGenerator::writeHelperFunctions(std::ofstream& out) {
	out << "// Helper functions\n";
	out << "inline const Registry* getRegistry(const std::string& name) {\n";
	out << "    auto it = MINECRAFT_REGISTRIES.find(name);\n";
	out << "    return (it != MINECRAFT_REGISTRIES.end()) ? &it->second : nullptr;\n";
	out << "}\n\n";

	out << "inline int getEntryProtocolId(const std::string& registry, const std::string& entry) {\n";
	out << "    const Registry* reg = getRegistry(registry);\n";
	out << "    if (!reg) return -1;\n";
	out << "    \n";
	out << "    for (const auto& e : reg->entries) {\n";
	out << "        if (e.name == entry) return e.protocol_id;\n";
	out << "    }\n";
	out << "    return -1;\n";
	out << "}\n\n";

	out << "inline std::vector<std::string> getRegistryNames() {\n";
	out << "    std::vector<std::string> names;\n";
	out << "    for (const auto& [name, registry] : MINECRAFT_REGISTRIES) {\n";
	out << "        names.push_back(name);\n";
	out << "    }\n";
	out << "    return names;\n";
	out << "}\n\n";

	out << "inline size_t getRegistryCount() {\n";
	out << "    return MINECRAFT_REGISTRIES.size();\n";
	out << "}\n";
}

void DataGenerator::parseTagsDirectory() {
	std::vector<TagCategory> tagCategories;
	std::string				 tagsPath = "generated/data/minecraft/tags";

	// Excluded categories as specified by user
	std::unordered_set<std::string> excludedCategories = {"block", "item", "fluid", "entity_type", "game_event"};

	try {
		for (const auto& categoryEntry : std::filesystem::directory_iterator(tagsPath)) {
			if (!categoryEntry.is_directory()) continue;

			std::string categoryName = categoryEntry.path().filename().string();

			// Skip excluded categories
			if (excludedCategories.find(categoryName) != excludedCategories.end()) {
				continue;
			}

			TagCategory category;
			category.categoryName = categoryName;

			// Recursively parse all tag files in this category
			parseTagsInDirectory(categoryEntry.path(), category, "");

			if (!category.tags.empty()) {
				tagCategories.push_back(category);
			}
		}

		generateTagsHeader(tagCategories);

	} catch (const std::exception& e) {
		throw std::runtime_error("Failed to parse tags directory: " + std::string(e.what()));
	}
}

void DataGenerator::parseTagsInDirectory(const std::filesystem::path& dirPath, TagCategory& category, const std::string& prefix) {
	for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
		if (entry.is_directory()) {
			// Recursively parse subdirectory
			std::string newPrefix = prefix.empty() ? entry.path().filename().string() : prefix + "/" + entry.path().filename().string();
			parseTagsInDirectory(entry.path(), category, newPrefix);
		} else if (entry.is_regular_file() && entry.path().extension() == ".json") {
			// Parse tag file
			try {
				json tagJson = loadJsonFile(entry.path().string());

				if (tagJson.contains("values") && tagJson["values"].is_array()) {
					Tag tag;
					tag.name = prefix.empty() ? entry.path().stem().string() : prefix + "/" + entry.path().stem().string();

					for (const auto& value : tagJson["values"]) {
						if (value.is_string()) {
							tag.values.push_back(value.get<std::string>());
						}
					}

					category.tags.push_back(tag);
				}
			} catch (const std::exception& e) {
				std::cerr << "Warning: Failed to parse tag file " << entry.path() << ": " << e.what() << "\n";
			}
		}
	}
}

void DataGenerator::generateTagsHeader(const std::vector<TagCategory>& tagCategories) {
	std::string	  headerPath = outputDirectory + "/minecraftTags.hpp";
	std::ofstream out(headerPath);

	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}

	// Write header
	out << "#pragma once\n";
	out << "#include <unordered_map>\n";
	out << "#include <vector>\n";
	out << "#include <string>\n";
	out << "#include <unordered_set>\n\n";

	// Write structures
	out << "struct MinecraftTag {\n";
	out << "    std::string name;\n";
	out << "    std::vector<std::string> values;\n";
	out << "};\n\n";

	out << "struct MinecraftTagCategory {\n";
	out << "    std::string categoryName;\n";
	out << "    std::vector<MinecraftTag> tags;\n";
	out << "};\n\n";

	// Write the main data structure
	out << "const std::vector<MinecraftTagCategory> MINECRAFT_TAGS = {\n";

	for (size_t i = 0; i < tagCategories.size(); ++i) {
		const auto& category = tagCategories[i];
		out << "    {\n";
		out << "        \"" << category.categoryName << "\",\n";
		out << "        {\n";

		for (size_t j = 0; j < category.tags.size(); ++j) {
			const auto& tag = category.tags[j];
			out << "            {\n";
			out << "                \"" << tag.name << "\",\n";
			out << "                {\n";

			for (size_t k = 0; k < tag.values.size(); ++k) {
				out << "                    \"" << tag.values[k] << "\"";
				if (k < tag.values.size() - 1) out << ",";
				out << "\n";
			}

			out << "                }\n";
			out << "            }";
			if (j < category.tags.size() - 1) out << ",";
			out << "\n";
		}

		out << "        }\n";
		out << "    }";
		if (i < tagCategories.size() - 1) out << ",";
		out << "\n";
	}

	out << "};\n\n";

	// Write helper functions
	writeTagHelperFunctions(out);

	out.close();

	size_t totalTags = 0;
	for (const auto& category : tagCategories) {
		totalTags += category.tags.size();
	}

	std::cout << "Generated " << headerPath << " with " << tagCategories.size() << " categories and " << totalTags << " tags\n";
}

void DataGenerator::writeTagHelperFunctions(std::ofstream& out) {
	out << "// Helper functions for Minecraft tags\n";

	// Function to get a specific tag category
	out << "inline const MinecraftTagCategory* getTagCategory(const std::string& categoryName) {\n";
	out << "    for (const auto& category : MINECRAFT_TAGS) {\n";
	out << "        if (category.categoryName == categoryName) {\n";
	out << "            return &category;\n";
	out << "        }\n";
	out << "    }\n";
	out << "    return nullptr;\n";
	out << "}\n\n";

	// Function to get a specific tag
	out << "inline const MinecraftTag* getTag(const std::string& categoryName, const std::string& tagName) {\n";
	out << "    const MinecraftTagCategory* category = getTagCategory(categoryName);\n";
	out << "    if (!category) return nullptr;\n";
	out << "    \n";
	out << "    for (const auto& tag : category->tags) {\n";
	out << "        if (tag.name == tagName) {\n";
	out << "            return &tag;\n";
	out << "        }\n";
	out << "    }\n";
	out << "    return nullptr;\n";
	out << "}\n\n";

	// Function to check if an item is in a tag
	out << "inline bool isInTag(const std::string& categoryName, const std::string& tagName, const std::string& item) {\n";
	out << "    const MinecraftTag* tag = getTag(categoryName, tagName);\n";
	out << "    if (!tag) return false;\n";
	out << "    \n";
	out << "    for (const auto& value : tag->values) {\n";
	out << "        if (value == item) return true;\n";
	out << "    }\n";
	out << "    return false;\n";
	out << "}\n\n";

	// Function to get all category names
	out << "inline std::vector<std::string> getTagCategoryNames() {\n";
	out << "    std::vector<std::string> names;\n";
	out << "    for (const auto& category : MINECRAFT_TAGS) {\n";
	out << "        names.push_back(category.categoryName);\n";
	out << "    }\n";
	out << "    return names;\n";
	out << "}\n\n";

	// Function to get all tag names in a category
	out << "inline std::vector<std::string> getTagNames(const std::string& categoryName) {\n";
	out << "    std::vector<std::string> names;\n";
	out << "    const MinecraftTagCategory* category = getTagCategory(categoryName);\n";
	out << "    if (category) {\n";
	out << "        for (const auto& tag : category->tags) {\n";
	out << "            names.push_back(tag.name);\n";
	out << "        }\n";
	out << "    }\n";
	out << "    return names;\n";
	out << "}\n\n";

	// Function to get total count of categories and tags
	out << "inline size_t getTagCategoryCount() {\n";
	out << "    return MINECRAFT_TAGS.size();\n";
	out << "}\n\n";

	out << "inline size_t getTotalTagCount() {\n";
	out << "    size_t count = 0;\n";
	out << "    for (const auto& category : MINECRAFT_TAGS) {\n";
	out << "        count += category.tags.size();\n";
	out << "    }\n";
	out << "    return count;\n";
	out << "}\n";
}

int main(int argc, char* argv[]) {
	try {
		// Check for correct number of arguments
		if (argc != 2) {
			std::cerr << "Usage: " << argv[0] << " <output_directory>\n";
			std::cerr << "Example: " << argv[0] << " ./generated\n";
			return 1;
		}

		std::string outputDir = argv[1];

		std::cout << "Starting data generation...\n";
		std::cout << "Output directory: " << outputDir << "\n";

		// Create generator and generate all files
		DataGenerator generator(outputDir);
		generator.generateAll();

		std::cout << "Data generation completed successfully!\n";
		return 0;

	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	} catch (...) {
		std::cerr << "Unknown error occurred\n";
		return 1;
	}
}
