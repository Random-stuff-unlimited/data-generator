#pragma once

#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <string>
#include <unordered_map>
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

struct Tag {
	std::string				 name;
	std::vector<std::string> values;
};

struct TagCategory {
	std::string		 categoryName;
	std::vector<Tag> tags;
};

class DataGenerator {
  private:
	std::string outputDirectory;

	void ensureOutputDirectory();
	void generateRegistryHeader(const std::unordered_map<std::string, Registry>& registries);
	void writeHelperFunctions(std::ofstream& out);
	void parseTagsDirectory();
	void parseTagsInDirectory(const std::filesystem::path& dirPath, TagCategory& category, const std::string& prefix);
	void generateTagsHeader(const std::vector<TagCategory>& tagCategories);
	void writeTagHelperFunctions(std::ofstream& out);

  public:
	DataGenerator(const std::string& outputDir);

	static json loadJsonFile(const std::string& filename);
	void		generateAll();
};
