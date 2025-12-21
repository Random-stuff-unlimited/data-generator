#include "dataGenerator.hpp"

#include <algorithm>
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

std::string toCppFunctionName(const std::string& registryName) {
	std::string result = registryName;
	
	// Remove minecraft: prefix
	if (result.find("minecraft:") == 0) {
		result = result.substr(10);
	}
	
	// Convert to CamelCase
	std::string camelCase;
	bool capitalizeNext = true;
	
	for (char c : result) {
		if (c == '/' || c == '_' || c == ':') {
			capitalizeNext = true;
		} else if (capitalizeNext) {
			camelCase += std::toupper(c);
			capitalizeNext = false;
		} else {
			camelCase += c;
		}
	}
	
	return "get" + camelCase;
}

void DataGenerator::generateAll() {
	std::cout << "Starting data generation for Minecraft 1.21.1...\n";
	
	// Load main registries from registries.json
	json registriesJson = DataGenerator::loadJsonFile("generated/reports/registries.json");
	
	std::unordered_map<std::string, Registry> allRegistries;
	
	// Process all registries from registries.json
	for (auto& [registryName, registryData] : registriesJson.items()) {
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
		
		allRegistries[registryName] = registry;
		std::cout << "Loaded registry: " << registryName << " with " << registry.entries.size() << " entries\n";
	}
	
	// Add variant registries from data/minecraft folders
	std::unordered_map<std::string, Registry> dataRegistries;
	addDataRegistries(dataRegistries);
	
	// Generate the 3 output files
	generateRegistryIdsHeader(allRegistries);
	generateMinecraftRegistriesHeader(dataRegistries);
	generateRegistriesTagsHeader(dataRegistries);
	
	std::cout << "\n=== Generation Summary ===\n";
	std::cout << "Total registries in RegistryIds.hpp: " << allRegistries.size() << "\n";
	std::cout << "Total registries in minecraftRegistries.hpp: " << dataRegistries.size() << "\n";
	std::cout << "All files generated successfully!\n";
}

void DataGenerator::addDataRegistries(std::unordered_map<std::string, Registry>& registries) {
	std::string dataPath = "generated/data/minecraft";
	
	if (!std::filesystem::exists(dataPath) || !std::filesystem::is_directory(dataPath)) {
		std::cerr << "Warning: data path not found: " << dataPath << "\n";
		return;
	}
	
	try {
		for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
			if (!entry.is_directory()) continue;
			
			std::string folderName	 = entry.path().filename().string();
			
			// Skip certain folders
			if (folderName == "advancement" || folderName == "loot_table" || 
			    folderName == "recipe" || folderName == "tags" || folderName == "datapacks") {
				continue;
			}
			
			std::string registryName = "minecraft:" + folderName;
			
			// Handle worldgen/biome special case
			if (folderName == "worldgen") {
				addWorldgenBiomeRegistry(registries);
				continue;
			}
			
			Registry registry;
			registry.protocol_id = 0;
			
			int entryProtocolId = 0;
			for (const auto& jsonEntry : std::filesystem::directory_iterator(entry.path())) {
				if (jsonEntry.is_regular_file() && jsonEntry.path().extension() == ".json") {
					RegistryEntry registryEntry;
					registryEntry.name		  = "minecraft:" + jsonEntry.path().stem().string();
					registryEntry.protocol_id = entryProtocolId++;
					registry.entries.push_back(registryEntry);
				}
			}
			
			if (!registry.entries.empty()) {
				registries[registryName] = registry;
				std::cout << "Added data registry: " << registryName << " with " << registry.entries.size() << " entries\n";
			}
		}
		
	} catch (const std::exception& e) {
		std::cerr << "Warning: Failed to scan data folders: " << e.what() << "\n";
	}
}

void DataGenerator::addWorldgenBiomeRegistry(std::unordered_map<std::string, Registry>& registries) {
	std::string biomePath = "generated/data/minecraft/worldgen/biome";
	if (!std::filesystem::exists(biomePath) || !std::filesystem::is_directory(biomePath)) {
		std::cerr << "Warning: biome path not found: " << biomePath << "\n";
		return;
	}
	
	Registry registry;
	registry.protocol_id = 0;
	int protocolId		 = 0;
	
	try {
		for (const auto& file : std::filesystem::directory_iterator(biomePath)) {
			if (file.is_regular_file() && file.path().extension() == ".json") {
				RegistryEntry entry;
				entry.name		  = "minecraft:" + file.path().stem().string();
				entry.protocol_id = protocolId++;
				registry.entries.push_back(entry);
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Warning: Failed to scan biome registry: " << e.what() << "\n";
	}
	
	if (!registry.entries.empty()) {
		registries["minecraft:worldgen/biome"] = registry;
		std::cout << "Added worldgen biome registry with " << registry.entries.size() << " entries\n";
	}
}

void DataGenerator::generateRegistryIdsHeader(const std::unordered_map<std::string, Registry>& registries) {
	std::string	  headerPath = outputDirectory + "/RegistryIds.hpp";
	std::ofstream out(headerPath);
	
	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}
	
	out << "#ifndef REGISTRY_IDS_HPP\n";
	out << "#define REGISTRY_IDS_HPP\n\n";
	out << "#include <cstdint>\n";
	out << "#include <map>\n";
	out << "#include <string>\n";
	out << "#include <vector>\n\n";
	out << "class RegistryIds {\n";
	out << "  public:\n";
	
	// Sort registries by name for consistent output
	std::vector<std::string> sortedNames;
	for (const auto& [name, _] : registries) {
		sortedNames.push_back(name);
	}
	std::sort(sortedNames.begin(), sortedNames.end());
	
	// Generate individual getter functions for each registry
	for (const auto& registryName : sortedNames) {
		const auto& registry = registries.at(registryName);
		std::string funcName = toCppFunctionName(registryName);
		
		out << "\tstatic std::map<std::string, uint32_t> " << funcName << "() {\n";
		out << "\t\tstatic std::map<std::string, uint32_t> registry = {\n";
		
		// Sort entries for consistent output
		std::vector<RegistryEntry> sortedEntries = registry.entries;
		std::sort(sortedEntries.begin(), sortedEntries.end(), 
		          [](const RegistryEntry& a, const RegistryEntry& b) {
			          return a.name < b.name;
		          });
		
		// Write entries in a compact format
		for (size_t i = 0; i < sortedEntries.size(); ++i) {
			if (i % 4 == 0) out << "\t\t\t\t";
			out << "{\"" << sortedEntries[i].name << "\", " << sortedEntries[i].protocol_id << "}";
			if (i < sortedEntries.size() - 1) out << ",";
			if ((i + 1) % 4 == 0 || i == sortedEntries.size() - 1) {
				out << "\n";
			} else {
				out << " ";
			}
		}
		
		out << "\t\t};\n";
		out << "\t\treturn registry;\n";
		out << "\t}\n\n";
	}
	
	// Generate the getRegistryId function with all registries
	out << "\tuint32_t getRegistryId(const std::string& registry, const std::string& key) {\n";
	out << "\t\tstatic std::map<std::string, std::map<std::string, uint32_t>> allRegistries = {\n";
	
	for (size_t i = 0; i < sortedNames.size(); ++i) {
		std::string funcName = toCppFunctionName(sortedNames[i]);
		out << "\t\t\t\t{\"" << sortedNames[i] << "\", " << funcName << "()}";
		if (i < sortedNames.size() - 1) out << ",";
		out << "\n";
	}
	
	out << "\t\t};\n\n";
	out << "\t\tauto registryIt = allRegistries.find(registry);\n";
	out << "\t\tif (registryIt == allRegistries.end()) {\n";
	out << "\t\t\treturn 0;\n";
	out << "\t\t}\n\n";
	out << "\t\tauto keyIt = registryIt->second.find(key);\n";
	out << "\t\tif (keyIt == registryIt->second.end()) {\n";
	out << "\t\t\treturn 0;\n";
	out << "\t\t}\n\n";
	out << "\t\treturn keyIt->second;\n";
	out << "\t}\n\n";
	
	// Generate getAvailableRegistries function
	out << "\tstd::vector<std::string> getAvailableRegistries() {\n";
	out << "\t\treturn {\n";
	for (size_t i = 0; i < sortedNames.size(); ++i) {
		out << "\t\t\t\t\"" << sortedNames[i] << "\"";
		if (i < sortedNames.size() - 1) out << ",";
		out << "\n";
	}
	out << "\t\t};\n";
	out << "\t}\n";
	
	out << "};\n\n";
	out << "#endif // REGISTRY_IDS_HPP\n";
	
	out.close();
	std::cout << "Generated " << headerPath << " with " << registries.size() << " registries\n";
}

void DataGenerator::generateMinecraftRegistriesHeader(const std::unordered_map<std::string, Registry>& registries) {
	std::string	  headerPath = outputDirectory + "/minecraftRegistries.hpp";
	std::ofstream out(headerPath);
	
	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}
	
	out << "#pragma once\n";
	out << "// Minecraft Registry Data\n";
	out << "// Generated automatically - do not modify\n\n";
	out << "#include <unordered_map>\n";
	out << "#include <vector>\n";
	out << "#include <string>\n\n";
	
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
	
	for (size_t i = 0; i < sortedRegistryNames.size(); ++i) {
		const auto& regName = sortedRegistryNames[i];
		const auto& registry = registries.at(regName);
		
		out << "    {\"" << regName << "\", {\n";
		out << "        {\n";
		
		const auto& entries = registry.entries;
		for (size_t j = 0; j < entries.size(); ++j) {
			out << "            {\"" << entries[j].name << "\", " << entries[j].protocol_id << "}";
			if (j < entries.size() - 1) out << ",";
			out << "\n";
		}
		
		out << "        },\n";
		out << "        " << registry.protocol_id << "\n";
		out << "    }}";
		
		if (i < sortedRegistryNames.size() - 1) out << ",";
		out << "\n";
	}
	out << "};\n\n";
	
	// Write helper functions
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
	
	out.close();
	std::cout << "Generated " << headerPath << " with " << registries.size() << " registries\n";
}

void DataGenerator::generateRegistriesTagsHeader(const std::unordered_map<std::string, Registry>& registries) {
	std::string	  headerPath = outputDirectory + "/RegistriesTag.hpp";
	std::ofstream out(headerPath);
	
	if (!out.is_open()) {
		throw std::runtime_error("Failed to create header file: " + headerPath);
	}
	
	out << "#pragma once\n\n";
	out << "#include <string>\n";
	out << "#include <unordered_map>\n";
	out << "#include <vector>\n\n";
	
	out << "struct Tag {\n";
	out << "\tstd::string\t\t name;\n";
	out << "\tstd::vector<int> entries;\n";
	out << "};\n\n";
	
	out << "const std::unordered_map<std::string, std::vector<Tag>> RegistriesTags = {\n";
	
	std::string tagsBasePath = "generated/data/minecraft/tags";
	
	if (!std::filesystem::exists(tagsBasePath)) {
		std::cerr << "Warning: tags path not found: " << tagsBasePath << "\n";
		out << "};\n";
		out.close();
		return;
	}
	
	std::vector<std::string> processedRegistries;
	
	// Process each registry that has tags
	for (const auto& entry : std::filesystem::directory_iterator(tagsBasePath)) {
		if (!entry.is_directory()) continue;
		
		std::string tagFolder = entry.path().filename().string();
		std::string registryName = "minecraft:" + tagFolder;
		
		// Handle worldgen/biome special case
		if (tagFolder == "worldgen") {
			std::string biomePath = entry.path().string() + "/biome";
			if (std::filesystem::exists(biomePath)) {
				processTagsForRegistry(out, biomePath, "minecraft:worldgen/biome", registries);
				processedRegistries.push_back("minecraft:worldgen/biome");
			}
			continue;
		}
		
		processTagsForRegistry(out, entry.path().string(), registryName, registries);
		processedRegistries.push_back(registryName);
	}
	
	out << "};\n";
	
	out.close();
	std::cout << "Generated " << headerPath << " with tags for " << processedRegistries.size() << " registries\n";
}

void DataGenerator::processTagsForRegistry(std::ofstream& out, const std::string& tagPath, 
                                           const std::string& registryName,
                                           const std::unordered_map<std::string, Registry>& registries) {
	
	if (!std::filesystem::exists(tagPath) || !std::filesystem::is_directory(tagPath)) {
		return;
	}
	
	// Check if this registry exists in our registries map
	auto regIt = registries.find(registryName);
	if (regIt == registries.end()) {
		return;
	}
	
	const Registry& registry = regIt->second;
	
	// Build a map of entry names to protocol IDs
	std::unordered_map<std::string, int> entryToId;
	for (const auto& entry : registry.entries) {
		entryToId[entry.name] = entry.protocol_id;
	}
	
	// Process all tag files
	std::vector<std::pair<std::string, std::vector<int>>> tags;
	
	std::function<void(const std::string&)> processDirectory = [&](const std::string& dirPath) {
		for (const auto& file : std::filesystem::directory_iterator(dirPath)) {
			if (file.is_directory()) {
				processDirectory(file.path().string());
			} else if (file.is_regular_file() && file.path().extension() == ".json") {
				try {
					json tagJson = DataGenerator::loadJsonFile(file.path().string());
					
					if (!tagJson.contains("values") || !tagJson["values"].is_array()) {
						continue;
					}
					
					std::string tagName = "minecraft:" + file.path().stem().string();
					std::vector<int> tagEntries;
					
					for (const auto& value : tagJson["values"]) {
						if (value.is_string()) {
							std::string entryName = value.get<std::string>();
							auto it = entryToId.find(entryName);
							if (it != entryToId.end()) {
								tagEntries.push_back(it->second);
							}
						}
					}
					
					if (!tagEntries.empty()) {
						std::sort(tagEntries.begin(), tagEntries.end());
						tags.push_back({tagName, tagEntries});
					}
					
				} catch (const std::exception& e) {
					std::cerr << "Warning: Failed to process tag file " << file.path() << ": " << e.what() << "\n";
				}
			}
		}
	};
	
	processDirectory(tagPath);
	
	if (tags.empty()) {
		return;
	}
	
	// Sort tags by name
	std::sort(tags.begin(), tags.end(), 
	          [](const auto& a, const auto& b) { return a.first < b.first; });
	
	// Write to output
	out << "\t\t{\"" << registryName << "\",\n";
	out << "\t\t {";
	
	for (size_t i = 0; i < tags.size(); ++i) {
		const auto& [tagName, entries] = tags[i];
		
		out << "{\"" << tagName << "\", {";
		for (size_t j = 0; j < entries.size(); ++j) {
			out << entries[j];
			if (j < entries.size() - 1) out << ", ";
		}
		out << "}}";
		
		if (i < tags.size() - 1) {
			out << ",\n\t\t  ";
		}
	}
	
	out << "}},\n";
}

void DataGenerator::normalizeRegistryEntries(std::unordered_map<std::string, Registry>& registries) {
	for (auto& [regName, registry] : registries) {
		for (auto& entry : registry.entries) {
			if (entry.name.find(':') == std::string::npos) {
				entry.name = "minecraft:" + entry.name;
			}
		}
	}
}

void DataGenerator::generateRegistryHeader(const std::unordered_map<std::string, Registry>& registries) {
	// This function is kept for compatibility but not used in the new implementation
}

void DataGenerator::writeHelperFunctions(std::ofstream& out) {
	// This function is kept for compatibility but not used in the new implementation
}

void DataGenerator::addVariantRegistries(std::unordered_map<std::string, Registry>& registries, 
                                        const std::unordered_set<std::string>& includedRegistries) {
	// This function is kept for compatibility but not used in the new implementation
}

int main(int argc, char* argv[]) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: " << argv[0] << " <output_directory>\n";
			std::cerr << "Example: " << argv[0] << " ./out\n";
			return 1;
		}
		
		std::string outputDir = argv[1];
		
		std::cout << "=== Minecraft 1.21.1 Data Generator ===\n";
		std::cout << "Output directory: " << outputDir << "\n\n";
		
		DataGenerator generator(outputDir);
		generator.generateAll();
		
		std::cout << "\n=== Generation completed successfully! ===\n";
		return 0;
		
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	} catch (...) {
		std::cerr << "Unknown error occurred\n";
		return 1;
	}
}