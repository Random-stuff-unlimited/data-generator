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

	// Generate the header file
	generateRegistryHeader(registries);
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
