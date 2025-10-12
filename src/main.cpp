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
#include <vector>

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
	// Define the registries to include
	std::unordered_set<std::string> includedRegistries = {"minecraft:worldgen/biome", "minecraft:chat_type",		"minecraft:trim_pattern",
														  "minecraft:trim_material",  "minecraft:wolf_variant",		"minecraft:wolf_sound_variant",
														  "minecraft:pig_variant",	  "minecraft:frog_variant",		"minecraft:cat_variant",
														  "minecraft:cow_variant",	  "minecraft:chicken_variant",	"minecraft:painting_variant",
														  "minecraft:dimension_type", "minecraft:damage_type",		"minecraft:banner_pattern",
														  "minecraft:enchantment",	  "minecraft:jukebox_song",		"minecraft:instrument",
														  "minecraft:dialog",		  "minecraft:test_environment", "minecraft:test_instance"};

	std::unordered_map<std::string, Registry> filteredRegistries;

	// Load main registries from registries.json
	json registriesJson = DataGenerator::loadJsonFile("generated/reports/registries.json");

	for (auto& [registryName, registryData] : registriesJson.items()) {
		// Only process included registries
		if (includedRegistries.find(registryName) == includedRegistries.end()) {
			continue;
		}

		if (!registryData.is_object() || !registryData.contains("entries")) {
			continue;
		}

		Registry registry;

		if (registryData.contains("protocol_id")) {
			registry.protocol_id = registryData["protocol_id"].get<int>();
		} else {
			registry.protocol_id = 0;
		}

		auto entries = registryData["entries"];
		for (auto& [entryName, entryData] : entries.items()) {
			if (!entryData.is_object() || !entryData.contains("protocol_id")) {
				continue;
			}

			RegistryEntry entry;
			entry.name		  = entryName;
			entry.protocol_id = entryData["protocol_id"].get<int>();
			registry.entries.push_back(entry);
		}

		filteredRegistries[registryName] = registry;
		std::cout << "Loaded registry: " << registryName << " with " << registry.entries.size() << " entries\n";
	}

	// Add variant registries from data/minecraft folders
	addVariantRegistries(filteredRegistries, includedRegistries);

	// Generate the header file
	generateRegistryHeader(filteredRegistries);

	std::cout << "Generated " << filteredRegistries.size() << " registries total\n";
}

void DataGenerator::addVariantRegistries(std::unordered_map<std::string, Registry>& registries,
										 const std::unordered_set<std::string>&		includedRegistries) {
	std::string dataPath = "generated/data/minecraft";

	try {
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			if (!entry.is_directory()) continue;

			std::string folderName	 = entry.path().filename().string();
			std::string registryName = "minecraft:" + folderName;

			// Only process included registries
			if (includedRegistries.find(registryName) == includedRegistries.end()) {
				continue;
			}

			Registry registry;
			registry.protocol_id = 0;

			int entryProtocolId = 0;
			for (const auto& jsonEntry : std::filesystem::directory_iterator(entry.path())) {
				if (jsonEntry.is_regular_file() && jsonEntry.path().extension() == ".json") {
					RegistryEntry registryEntry;
					registryEntry.name		  = jsonEntry.path().stem().string();
					registryEntry.protocol_id = entryProtocolId++;
					registry.entries.push_back(registryEntry);
				}
			}

			if (!registry.entries.empty()) {
				registries[registryName] = registry;
				std::cout << "Added variant registry: " << registryName << " with " << registry.entries.size() << " entries\n";
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Warning: Failed to scan variant folders: " << e.what() << "\n";
	}
}

void DataGenerator::generateRegistryHeader(const std::unordered_map<std::string, Registry>& registries) {
	std::string	  headerPath = outputDirectory + "/minecraftRegistries.hpp";
	std::ofstream out(headerPath);

	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}

	// Write header
	out << "#pragma once\n";
	out << "// Minecraft Registry Data\n";
	out << "// Generated automatically - do not modify\n\n";
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

	// Write the registry list
	out << "// Complete list of registries\n";
	out << "const std::vector<std::string> REGISTRY_LIST = {\n";

	std::vector<std::string> sortedRegistryNames;
	for (const auto& [name, registry] : registries) {
		sortedRegistryNames.push_back(name);
	}
	std::sort(sortedRegistryNames.begin(), sortedRegistryNames.end());

	for (size_t i = 0; i < sortedRegistryNames.size(); ++i) {
		out << "    \"" << sortedRegistryNames[i] << "\"";
		if (i < sortedRegistryNames.size() - 1) out << ",";
		out << "\n";
	}
	out << "};\n\n";

	// Write the main registry data hashmap
	out << "const std::unordered_map<std::string, Registry> REGISTRIES = {\n";

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
	out << "// Helper functions for registries\n";

	out << "inline const Registry* getRegistry(const std::string& name) {\n";
	out << "    auto it = REGISTRIES.find(name);\n";
	out << "    return (it != REGISTRIES.end()) ? &it->second : nullptr;\n";
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
	out << "    return REGISTRY_LIST;\n";
	out << "}\n\n";

	out << "inline size_t getRegistryCount() {\n";
	out << "    return REGISTRY_LIST.size();\n";
	out << "}\n\n";

	out << "inline bool hasRegistry(const std::string& name) {\n";
	out << "    return REGISTRIES.find(name) != REGISTRIES.end();\n";
	out << "}\n\n";

	out << "inline size_t getTotalEntryCount() {\n";
	out << "    size_t count = 0;\n";
	out << "    for (const auto& [name, registry] : REGISTRIES) {\n";
	out << "        count += registry.entries.size();\n";
	out << "    }\n";
	out << "    return count;\n";
	out << "}\n";
}

int main(int argc, char* argv[]) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: " << argv[0] << " <output_directory>\n";
			std::cerr << "Example: " << argv[0] << " ./out\n";
			return 1;
		}

		std::string outputDir = argv[1];

		std::cout << "Starting registry generation...\n";
		std::cout << "Output directory: " << outputDir << "\n";

		DataGenerator generator(outputDir);
		generator.generateAll();

		std::cout << "Registry generation completed successfully!\n";
		return 0;

	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	} catch (...) {
		std::cerr << "Unknown error occurred\n";
		return 1;
	}
}
