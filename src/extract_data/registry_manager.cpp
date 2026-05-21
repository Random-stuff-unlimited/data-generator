#include "registry_manager.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

bool RegistryManager::load_everything(const std::string& generated_folder) {
    std::string reports_dir = generated_folder + "/reports";
    std::string minecraft_data_dir = generated_folder + "/data/minecraft";

    std::cout << "\nLoading all server data into RAM..." << std::endl;

    bool success = true;

    success &= load_static_registries(reports_dir + "/registries.json");
    std::cout << "1 : " << success << "\n";
    success &= load_packets(reports_dir + "/packets.json");
    std::cout << "2 : " << success << "\n";
    success &= load_datapack_recursive(minecraft_data_dir);
    std::cout << "3 : " << success << "\n";
    int b_id = 0, d_id = 0;
    biome_registry_packet = json::object();
    dimension_registry_packet = json::object();

    for (const auto& [path, data] : datapack_data) {
        if (path.find("worldgen/biome/") == 0) {
            std::string name = "minecraft:" + path.substr(15);
            biome_ids[name] = b_id;
            biome_registry_packet[name] = {{"protocol_id", b_id}, {"element", data}};
            b_id++;
        }
        else if (path.find("worldgen/dimension_type/") == 0) {
            std::string name = "minecraft:" + path.substr(24);
            dimension_ids[name] = d_id;
            dimension_registry_packet[name] = {{"protocol_id", d_id}, {"element", data}};
            d_id++;
        }
    }

    if (success) {
        std::cout << "\n==============================================" << std::endl;
        std::cout << "[SUCCESS] Game fully loaded in RAM:" << std::endl;
        std::cout << " - " << block_ids.size() << " Blocks (Fast Access)" << std::endl;
        std::cout << " - " << packet_ids.size() << " Network Packets (Fast Access)" << std::endl;
        std::cout << " - " << biome_ids.size() << " Biomes (Fast Access)" << std::endl;
        std::cout << " - " << datapack_data.size() << " JSON files loaded in the Big Map!" << std::endl;
        std::cout << "==============================================\n" << std::endl;
    }

    return success;
}

bool RegistryManager::load_datapack_recursive(const std::string& data_folder) {
    fs::path dir(data_folder);
    if (!fs::exists(dir) || !fs::is_directory(dir)) return false;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string rel_path = fs::relative(entry.path(), dir).string();
            std::replace(rel_path.begin(), rel_path.end(), '\\', '/');
            rel_path = rel_path.substr(0, rel_path.find_last_of('.'));
            std::ifstream file(entry.path());
            if (file.is_open()) {
                try {
                    json data;
                    file >> data;
                    datapack_data[rel_path] = data;
                } catch (...) {
                    std::cerr << "Warning: Failed to parse JSON file: " << rel_path << std::endl;
                }
            }
        }
    }
    return true;
}

bool RegistryManager::load_static_registries(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    json root; file >> root;

    auto parse_registry = [&](const std::string& key, std::unordered_map<std::string, int>& map_to_fill) {
        if (root.contains(key) && root[key].contains("entries")) {
            for (auto& [name, value] : root[key]["entries"].items()) {
                if (value.contains("protocol_id")) map_to_fill[name] = value["protocol_id"].get<int>();
            }
        }
    };
    parse_registry("minecraft:block", block_ids);
    parse_registry("minecraft:item", item_ids);
    return true;
}

bool RegistryManager::load_packets(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    json root; file >> root;
    for (auto& [state, state_data] : root.items()) {
        for (auto& [bound, bound_data] : state_data.items()) {
            for (auto& [packet_name, packet_data] : bound_data.items()) {
                if (packet_data.contains("protocol_id")) {
                    packet_ids[state + "/" + bound + "/" + packet_name] = packet_data["protocol_id"].get<int>();
                }
            }
        }
    }
    return true;
}
