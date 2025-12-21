#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

struct RegistryEntry {
	std::string name;
	int			protocol_id;
};

struct Registry {
	std::vector<RegistryEntry> entries;
	int						   protocol_id = 0;
};

class DataGenerator {
  private:
	std::string outputDirectory;

	void ensureOutputDirectory();

	void generateRegistryHeader(const std::unordered_map<std::string, Registry>& registries);
	void generateRegistryIdsHeader(const std::unordered_map<std::string, Registry>& registries);
	void generateMinecraftRegistriesHeader(const std::unordered_map<std::string, Registry>& registries);
	void generateRegistriesTagsHeader(const std::unordered_map<std::string, Registry>& registries);
	void processTagsForRegistry(std::ofstream& out, const std::string& tagPath, 
	                            const std::string& registryName,
	                            const std::unordered_map<std::string, Registry>& registries);

	void writeHelperFunctions(std::ofstream& out);

	void addVariantRegistries(std::unordered_map<std::string, Registry>& registries, const std::unordered_set<std::string>& includedRegistries);
	void addDataRegistries(std::unordered_map<std::string, Registry>& registries);
	void addWorldgenBiomeRegistry(std::unordered_map<std::string, Registry>& registries);
	void normalizeRegistryEntries(std::unordered_map<std::string, Registry>& registries);

  public:
	DataGenerator(const std::string& outputDir);

	static json loadJsonFile(const std::string& filename);
	void		generateAll();
};
