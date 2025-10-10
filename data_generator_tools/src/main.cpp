#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include "json.hpp"
#include "utils.hpp"
#include "file_writer.hpp"

class DataGenerator {
public:
    DataGenerator(const std::string& outputDir = "generated");
    void doItems();
    void doBlocks();
    void doBlockTypes();
    void doProperties();
    void doGetBlockStateIdFromRaw();
    void doGetRawPropertiesFromBlockStateId();
    void doBlockEntityTypes();
    void generateAll();

private:
    std::string outputDirectory;
    json loadJsonFile(const std::string& filename);
    void ensureOutputDirectory();
};

DataGenerator::DataGenerator(const std::string& outputDir) : outputDirectory(outputDir) {
    ensureOutputDirectory();
}

void DataGenerator::ensureOutputDirectory() {
    std::filesystem::create_directories(outputDirectory);
}

json DataGenerator::loadJsonFile(const std::string& filename) {
    try {
        std::string content = Utils::readFile(filename);
        return json::parse(content);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load JSON file " + filename + ": " + e.what());
    }
}

void DataGenerator::doItems() {
    json itemsJson = loadJsonFile("../generated/reports/items.json");
    json registriesJson = loadJsonFile("../generated/reports/registries.json");

    auto itemsRegistry = registriesJson["minecraft:item"]["entries"];

    FileWriter writer(outputDirectory + "/items.hpp");

    writer << "#ifndef ITEMS_HPP\n";
    writer << "#define ITEMS_HPP\n\n";
    writer << "#include <map>\n";
    writer << "#include <string>\n\n";

    writer << "// Generated Item data structure\n";
    writer << "enum class ItemRarity {\n";
    writer << "    Common,\n";
    writer << "    Uncommon,\n";
    writer << "    Rare,\n";
    writer << "    Epic\n";
    writer << "};\n\n";

    writer << "struct Item {\n";
    writer << "    int max_stack_size;\n";
    writer << "    ItemRarity rarity;\n";
    writer << "    int repair_cost;\n";
    writer << "    int id;\n";
    writer << "};\n\n";

    writer << "std::map<std::string, Item> getItems() {\n";
    writer << "    std::map<std::string, Item> items = {\n";

    for (auto& [key, value] : itemsJson.items()) {
        auto components = value["components"];

        int maxStackSize = components["minecraft:max_stack_size"].get<int>();
        std::string rarityStr = components["minecraft:rarity"].get<std::string>();
        std::string rarity = Utils::convertToUpperCamelCase(rarityStr);
        int repairCost = components["minecraft:repair_cost"].get<int>();
        int id = itemsRegistry[key]["protocol_id"].get<int>();

        writer << "        {\"" << key << "\", {" << maxStackSize
               << ", ItemRarity::" << rarity << ", " << repairCost
               << ", " << id << "}},\n";
    }

    writer << "    };\n";
    writer << "    return items;\n";
    writer << "}\n\n";
    writer << "#endif // ITEMS_HPP\n";
}

void DataGenerator::doBlockEntityTypes() {
    json registriesJson = loadJsonFile("../generated/reports/registries.json");
    auto registry = registriesJson["minecraft:block_entity_type"]["entries"];

    FileWriter writer(outputDirectory + "/block_entity_types.hpp");

    writer << "#ifndef BLOCK_ENTITY_TYPES_HPP\n";
    writer << "#define BLOCK_ENTITY_TYPES_HPP\n\n";
    writer << "#include <map>\n";
    writer << "#include <string>\n";
    writer << "#include <cstdint>\n\n";

    writer << "std::map<std::string, uint8_t> getBlockEntityTypes() {\n";
    writer << "    std::map<std::string, uint8_t> output;\n\n";

    for (auto& [key, value] : registry.items()) {
        int protocolId = value["protocol_id"].get<int>();
        writer << "    output[\"" << key << "\"] = " << protocolId << ";\n";
    }

    writer << "\n    return output;\n";
    writer << "}\n\n";
    writer << "#endif // BLOCK_ENTITY_TYPES_HPP\n";
}

void DataGenerator::doBlockTypes() {
    std::string blocksFile = Utils::readFile("../generated/reports/blocks.json");

    std::vector<std::string> blockTypes;
    std::stringstream ss(blocksFile);
    std::string line;

    while (std::getline(ss, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

        if (trimmed.find("\"type\":") == 0) {
            std::string type = trimmed;
            type = Utils::replace(type, "\"type\": \"", "");
            type = Utils::replace(type, "\",", "");
            blockTypes.push_back(Utils::convertToUpperCamelCase(type));
        }
    }

    std::sort(blockTypes.begin(), blockTypes.end());
    blockTypes.erase(std::unique(blockTypes.begin(), blockTypes.end()), blockTypes.end());

    FileWriter writer(outputDirectory + "/block_types.hpp");

    writer << "#ifndef BLOCK_TYPES_HPP\n";
    writer << "#define BLOCK_TYPES_HPP\n\n";

    writer << "enum class BlockType {\n";
    for (const auto& blockType : blockTypes) {
        if (blockType != "Type: [") {
            writer << "    " << blockType << ",\n";
        }
    }
    writer << "};\n\n";
    writer << "#endif // BLOCK_TYPES_HPP\n";
}

void DataGenerator::doProperties() {
    json blocksJson = loadJsonFile("../generated/reports/blocks.json");

    std::map<std::string, std::vector<std::string>> properties;

    for (auto& [blockKey, blockData] : blocksJson.items()) {
        if (!blockData.contains("properties") || !blockData["properties"].is_object()) {
            continue;
        }

        std::string blockType = Utils::convertToUpperCamelCase(blockData["definition"]["type"].get<std::string>());

        for (auto& [propKey, propValues] : blockData["properties"].items()) {
            std::string propertyEntry = blockType + Utils::convertToUpperCamelCase(propKey);

            if (properties.find(propertyEntry) == properties.end()) {
                std::vector<std::string> values;
                for (auto& value : propValues) {
                    values.push_back(value.get<std::string>());
                }
                properties[propertyEntry] = values;
            }
        }
    }

    FileWriter writer(outputDirectory + "/properties.hpp");

    writer << "#ifndef PROPERTIES_HPP\n";
    writer << "#define PROPERTIES_HPP\n\n";
    writer << "#include <variant>\n\n";

    // Generate enums for each property
    for (const auto& [propName, propValues] : properties) {
        writer << "enum class " << propName << " {\n";

        for (const auto& variant : propValues) {
            std::string enumVariant = Utils::isNumeric(variant) ?
                "Num" + Utils::convertToUpperCamelCase(variant) :
                Utils::convertToUpperCamelCase(variant);
            writer << "    " << enumVariant << ",\n";
        }

        writer << "};\n\n";
    }

    // Generate main Property variant using std::variant
    writer << "using Property = std::variant<\n";

    auto it = properties.begin();
    for (size_t i = 0; i < properties.size(); ++i) {
        writer << "    " << it->first;
        if (i < properties.size() - 1) {
            writer << ",";
        }
        writer << "\n";
        ++it;
    }

    writer << ">;\n\n";
    writer << "#endif // PROPERTIES_HPP\n";
}

void DataGenerator::doBlocks() {
    json blocksJson = loadJsonFile("../generated/reports/blocks.json");

    FileWriter writer(outputDirectory + "/blocks.hpp");

    writer << "#ifndef BLOCKS_HPP\n";
    writer << "#define BLOCKS_HPP\n\n";
    writer << "#include <map>\n";
    writer << "#include <string>\n";
    writer << "#include <vector>\n";
    writer << "#include \"block_types.hpp\"\n";
    writer << "#include \"properties.hpp\"\n\n";

    writer << "// Generated Block data structures\n";
    writer << "struct State {\n";
    writer << "    int id;\n";
    writer << "    std::vector<Property> properties;\n";
    writer << "    bool is_default;\n";
    writer << "};\n\n";

    writer << "struct Block {\n";
    writer << "    BlockType block_type;\n";
    writer << "    std::vector<Property> properties;\n";
    writer << "    std::vector<State> states;\n";
    writer << "};\n\n";

    writer << "std::map<std::string, Block> getBlocks() {\n";
    writer << "    std::map<std::string, Block> output;\n\n";

    for (auto& [key, blockData] : blocksJson.items()) {
        std::string blockType = Utils::convertToUpperCamelCase(blockData["definition"]["type"].get<std::string>());

        writer << "    // Adding block: " << key << "\n";
        writer << "    {\n";
        writer << "        Block block;\n";
        writer << "        block.block_type = BlockType::" << blockType << ";\n";

        // Add block properties
        if (blockData.contains("properties") && blockData["properties"].is_object()) {
            writer << "        block.properties = {\n";
            for (auto& [propKey, propValues] : blockData["properties"].items()) {
                for (auto& propValue : propValues) {
                    std::string propValStr = propValue.get<std::string>();
                    std::string enumValue = Utils::isNumeric(propValStr) ?
                        "Num" + Utils::convertToUpperCamelCase(propValStr) :
                        Utils::convertToUpperCamelCase(propValStr);

                    writer << "            " << blockType << Utils::convertToUpperCamelCase(propKey)
                           << "::" << enumValue << ",\n";
                }
            }
            writer << "        };\n";
        }

        // Add block states
        if (blockData.contains("states") && blockData["states"].is_array()) {
            writer << "        block.states = {\n";
            for (auto& state : blockData["states"]) {
                int id = state["id"].get<int>();
                bool isDefault = state.contains("default") && state["default"].get<bool>();

                writer << "            {" << id << ", {";

                if (state.contains("properties") && state["properties"].is_object()) {
                    for (auto& [propKey, propValue] : state["properties"].items()) {
                        std::string propValStr = propValue.get<std::string>();
                        std::string enumValue = Utils::isNumeric(propValStr) ?
                            "Num" + Utils::convertToUpperCamelCase(propValStr) :
                            Utils::convertToUpperCamelCase(propValStr);

                        writer << blockType << Utils::convertToUpperCamelCase(propKey)
                               << "::" << enumValue << ", ";
                    }
                }

                writer << "}, " << (isDefault ? "true" : "false") << "},\n";
            }
            writer << "        };\n";
        }

        writer << "        output[\"" << key << "\"] = block;\n";
        writer << "    }\n\n";
    }

    writer << "    return output;\n";
    writer << "}\n\n";
    writer << "#endif // BLOCKS_HPP\n";
}

void DataGenerator::doGetBlockStateIdFromRaw() {
    json blocksJson = loadJsonFile("../generated/reports/blocks.json");
    std::string blocksFile = Utils::readFile("../generated/reports/blocks.json");

    // Extract block types
    std::vector<std::string> blockTypes;
    std::stringstream ss(blocksFile);
    std::string line;

    while (std::getline(ss, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

        if (trimmed.find("\"type\":") == 0) {
            std::string type = trimmed;
            type = Utils::replace(type, "\"type\": \"", "");
            type = Utils::replace(type, "\",", "");
            blockTypes.push_back(Utils::convertToUpperCamelCase(type));
        }
    }

    std::sort(blockTypes.begin(), blockTypes.end());
    blockTypes.erase(std::unique(blockTypes.begin(), blockTypes.end()), blockTypes.end());

    // Extract properties
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> properties;

    for (auto& [blockKey, blockData] : blocksJson.items()) {
        if (!blockData.contains("properties") || !blockData["properties"].is_object()) {
            continue;
        }

        std::string blockType = Utils::convertToUpperCamelCase(blockData["definition"]["type"].get<std::string>());

        for (auto& [propKey, propValues] : blockData["properties"].items()) {
            std::pair<std::string, std::string> key = {blockType, propKey};

            if (properties.find(key) == properties.end()) {
                std::vector<std::string> values;
                for (auto& value : propValues) {
                    values.push_back(value.get<std::string>());
                }
                properties[key] = values;
            }
        }
    }

    FileWriter writer(outputDirectory + "/block_state_functions.hpp");

    writer << "#ifndef BLOCK_STATE_FUNCTIONS_HPP\n";
    writer << "#define BLOCK_STATE_FUNCTIONS_HPP\n\n";
    writer << "#include \"blocks.hpp\"\n";
    writer << "#include <stdexcept>\n";
    writer << "#include <algorithm>\n\n";

    writer << "int getBlockStateIdFromRaw(const std::map<std::string, Block>& blockStates,\n";
    writer << "                           const std::string& blockName,\n";
    writer << "                           const std::vector<std::pair<std::string, std::string>>& properties) {\n";
    writer << "    auto blockIt = blockStates.find(blockName);\n";
    writer << "    if (blockIt == blockStates.end()) {\n";
    writer << "        throw std::runtime_error(\"Block not found: \" + blockName);\n";
    writer << "    }\n";
    writer << "    const Block& block = blockIt->second;\n\n";
    writer << "    switch (block.block_type) {\n";

    for (const auto& blockType : blockTypes) {
        if (blockType == "Type: [") continue;

        writer << "        case BlockType::" << blockType << ": {\n";
        writer << "            std::vector<State> possibleStates = block.states;\n";

        // Check if this block type has properties
        bool hasProperties = false;
        for (const auto& [propKey, _] : properties) {
            if (propKey.first == blockType) {
                hasProperties = true;
                break;
            }
        }

        if (!hasProperties) {
            writer << "            return possibleStates.empty() ? -1 : possibleStates[0].id;\n";
            writer << "        }\n";
        } else {
            writer << "            for (const auto& prop : properties) {\n";
            writer << "                const std::string& propName = prop.first;\n";
            writer << "                const std::string& propValue = prop.second;\n\n";

            for (const auto& [propKey, propValues] : properties) {
                if (propKey.first == blockType) {
                    writer << "                if (propName == \"" << propKey.second << "\") {\n";

                    for (const auto& propVal : propValues) {
                        std::string propertyName = blockType + Utils::convertToUpperCamelCase(propKey.second);
                        std::string propertyValue = Utils::isNumeric(propVal) ?
                            "Num" + Utils::convertToUpperCamelCase(propVal) :
                            Utils::convertToUpperCamelCase(propVal);

                        writer << "                    if (propValue == \"" << propVal << "\") {\n";
                        writer << "                        // Filter states that have this property value\n";
                        writer << "                        possibleStates.erase(\n";
                        writer << "                            std::remove_if(possibleStates.begin(), possibleStates.end(),\n";
                        writer << "                                [](const State& state) {\n";
                        writer << "                                    // Check if state has the required property\n";
                        writer << "                                    return !hasProperty(state, " << propertyName << "::" << propertyValue << ");\n";
                        writer << "                                }),\n";
                        writer << "                            possibleStates.end());\n";
                        writer << "                    }\n";
                    }

                    writer << "                }\n";
                }
            }

            writer << "            }\n";
            writer << "            if (possibleStates.size() != 1) {\n";
            writer << "                throw std::runtime_error(\"Expected exactly one matching state, got \" + std::to_string(possibleStates.size()));\n";
            writer << "            }\n";
            writer << "            return possibleStates[0].id;\n";
            writer << "        }\n";
        }
    }

    writer << "        default:\n";
    writer << "            throw std::runtime_error(\"Unknown block type\");\n";
    writer << "    }\n";
    writer << "}\n\n";
    writer << "#endif // BLOCK_STATE_FUNCTIONS_HPP\n";
}

void DataGenerator::doGetRawPropertiesFromBlockStateId() {
    json blocksJson = loadJsonFile("../generated/reports/blocks.json");

    // Extract properties map
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> properties;

    for (auto& [blockKey, blockData] : blocksJson.items()) {
        if (!blockData.contains("properties") || !blockData["properties"].is_object()) {
            continue;
        }

        std::string blockType = Utils::convertToUpperCamelCase(blockData["definition"]["type"].get<std::string>());

        for (auto& [propKey, propValues] : blockData["properties"].items()) {
            std::pair<std::string, std::string> key = {blockType, propKey};

            if (properties.find(key) == properties.end()) {
                std::vector<std::string> values;
                for (auto& value : propValues) {
                    values.push_back(value.get<std::string>());
                }
                properties[key] = values;
            }
        }
    }

    FileWriter writer(outputDirectory + "/property_functions.hpp");

    writer << "#ifndef PROPERTY_FUNCTIONS_HPP\n";
    writer << "#define PROPERTY_FUNCTIONS_HPP\n\n";
    writer << "#include \"blocks.hpp\"\n";
    writer << "#include <vector>\n";
    writer << "#include <utility>\n";
    writer << "#include <cstdint>\n\n";

    writer << "std::vector<std::pair<std::string, std::string>> getRawPropertiesFromBlockStateId(\n";
    writer << "    const std::map<std::string, Block>& blockStates, uint16_t blockStateId) {\n\n";
    writer << "    // Find the state with the given ID\n";
    writer << "    const State* foundState = nullptr;\n";
    writer << "    for (const auto& [blockName, block] : blockStates) {\n";
    writer << "        for (const auto& state : block.states) {\n";
    writer << "            if (state.id == blockStateId) {\n";
    writer << "                foundState = &state;\n";
    writer << "                break;\n";
    writer << "            }\n";
    writer << "        }\n";
    writer << "        if (foundState) break;\n";
    writer << "    }\n\n";
    writer << "    if (!foundState) {\n";
    writer << "        throw std::runtime_error(\"Block state ID not found: \" + std::to_string(blockStateId));\n";
    writer << "    }\n\n";
    writer << "    std::vector<std::pair<std::string, std::string>> output;\n\n";
    writer << "    // Extract properties from the state\n";
    writer << "    for (const auto& property : foundState->properties) {\n";
    writer << "        std::visit([&output](const auto& prop) {\n";
    writer << "            using T = std::decay_t<decltype(prop)>;\n";

    for (const auto& [propKey, propValues] : properties) {
        std::string enumVariant = propKey.first + Utils::convertToUpperCamelCase(propKey.second);

        writer << "            if constexpr (std::is_same_v<T, " << enumVariant << ">) {\n";
        writer << "                switch (prop) {\n";

        for (const auto& option : propValues) {
            std::string variant = Utils::isNumeric(option) ?
                "Num" + Utils::convertToUpperCamelCase(option) :
                Utils::convertToUpperCamelCase(option);

            writer << "                    case " << enumVariant << "::" << variant << ":\n";
            writer << "                        output.emplace_back(\"" << propKey.second << "\", \"" << option << "\");\n";
            writer << "                        break;\n";
        }

        writer << "                }\n";
        writer << "            }\n";
    }

    writer << "        }, property);\n";
    writer << "    }\n\n";
    writer << "    return output;\n";
    writer << "}\n\n";
    writer << "#endif // PROPERTY_FUNCTIONS_HPP\n";
}

void DataGenerator::generateAll() {
    std::cout << "Generating all files..." << std::endl;

    doBlockEntityTypes();
    doItems();
    doBlockTypes();
    doProperties();
    doBlocks();
    doGetBlockStateIdFromRaw();
    doGetRawPropertiesFromBlockStateId();

    std::cout << "All files generated in directory: " << outputDirectory << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        std::string outputDir = "generated";

        // Allow specifying output directory as command line argument
        if (argc > 1) {
            outputDir = argv[1];
        }

        DataGenerator generator(outputDir);

        if (argc > 2) {
            std::string command = argv[2];
            if (command == "items") {
                generator.doItems();
            } else if (command == "blocks") {
                generator.doBlocks();
            } else if (command == "block-types") {
                generator.doBlockTypes();
            } else if (command == "properties") {
                generator.doProperties();
            } else if (command == "block-entities") {
                generator.doBlockEntityTypes();
            } else if (command == "all") {
                generator.generateAll();
            } else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Available commands: items, blocks, block-types, properties, block-entities, all" << std::endl;
                return 1;
            }
        } else {
            // Default behavior - generate block entity types
            generator.doBlockEntityTypes();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
