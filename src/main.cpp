#include "server_downloader.hpp"
#include "server_extration.hpp"
#include "registry_manager.hpp"
#include <iostream>
#include <string>

int main() {
	download_java_server();
	run_data_generator();

	RegistryManager registry;

    if (!registry.load_everything("generated")) {
        std::cerr << "parsing fails" << std::endl;
        return 1;
    }

    int id_pierre = registry.block_ids["minecraft:stone"];
    int id_paquet_chat = registry.packet_ids["play/clientbound/minecraft:system_chat"];
    float temp = registry.datapack_data["worldgen/biome/desert"]["temperature"].get<float>();
    auto zombie_drops = registry.datapack_data["loot_table/entities/zombie"];

    std::cout << "zombie drop : " << zombie_drops << "\n";
}
