#ifndef REGISTRY_MANAGER_HPP
#define REGISTRY_MANAGER_HPP

#include "json.hpp"
#include <unordered_map>
#include <string>

using json = nlohmann::json;

class RegistryManager {
public:
    std::unordered_map<std::string, json> datapack_data;

    std::unordered_map<std::string, int> biome_ids;
    std::unordered_map<std::string, int> dimension_ids;
    std::unordered_map<std::string, int> block_ids;
    std::unordered_map<std::string, int> item_ids;
    std::unordered_map<std::string, int> packet_ids;

    json biome_registry_packet;
    json dimension_registry_packet;

    bool load_everything(const std::string& generated_folder_path);

private:
    bool load_static_registries(const std::string& path);
    bool load_packets(const std::string& path);
    bool load_datapack_recursive(const std::string& data_folder);
};

#endif
