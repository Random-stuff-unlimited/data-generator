#include "json.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <curl/curl.h>
#include "server_downloader.hpp"

using json = nlohmann::json;

std::string selectVersion(const json& manifest) {
    std::vector<VersionInfo> versions;
    for (const auto& version_entry : manifest["versions"]) {
        versions.push_back({
            version_entry["id"].get<std::string>(),
            version_entry["type"].get<std::string>(),
            version_entry["url"].get<std::string>(),
            version_entry["time"].get<std::string>(),
            version_entry["releaseTime"].get<std::string>(),
            version_entry["sha1"].get<std::string>(),
            version_entry["complianceLevel"].get<int>()
        });
    }

    int current_page = 0;
    int versions_per_page = 10;
    bool show_snapshots = false;
    std::string selected_version_url;

    while (true) {
        std::cout << "\033[2J\033[H";
        std::cout << "\n--- Version Selection Menu ---" << std::endl;

        std::vector<VersionInfo> filtered_versions;
        if (show_snapshots) {
            filtered_versions = versions;
        } else {
            for (const auto& v : versions) {
                if (v.type != "snapshot") {
                    filtered_versions.push_back(v);
                }
            }
        }

        if (filtered_versions.empty()) {
            std::cout << "No versions available with the current filters." << std::endl;
            break;
        }

        int start_index = current_page * versions_per_page;
        int end_index = std::min(start_index + versions_per_page, (int)filtered_versions.size());

        for (int i = start_index; i < end_index; ++i) {
            std::cout << (i - start_index + 1) << ". " << filtered_versions[i].id << " (" << filtered_versions[i].type << ")" << std::endl;
        }

        std::cout << "\nOptions:" << std::endl;
        if (start_index > 0) {
            std::cout << "P. Previous page" << std::endl;
        }
        if ((size_t)end_index < filtered_versions.size()) {
            std::cout << "N. Next page" << std::endl;
        }
        std::cout << "S. " << (show_snapshots ? "Disable" : "Enable") << " snapshots" << std::endl;
        std::cout << "X. Exit" << std::endl;
        std::cout << "Enter version number or an option (P/N/S/X): ";

        std::string input;
        std::cin >> input;

        if (std::cin.eof()) {
            std::cout << "EOF detected. Exiting version selection." << std::endl;
            return "";
        }

        if (input == "P" || input == "p") {
            if (current_page > 0) {
                current_page--;
            }
        } else if (input == "N" || input == "n") {
            if ((size_t)end_index < filtered_versions.size()) {
                current_page++;
            }
        } else if (input == "S" || input == "s") {
            show_snapshots = !show_snapshots;
            current_page = 0;
        } else if (input == "X" || input == "x") {
            std::cout << "Exiting version selection." << std::endl;
            return "";
        } else {
            try {
                int selection = std::stoi(input);
                if (selection > 0 && selection <= (end_index - start_index)) {
                    int final_index = start_index + selection - 1;
                    std::cout << "\nSelected version: " << filtered_versions[final_index].id << std::endl;
                    return filtered_versions[final_index].url;
                } else {
                    std::cout << "Invalid selection. Please try again." << std::endl;
                }
            } catch (const std::invalid_argument& e) {
                std::cout << "Invalid input. Please enter a number or a valid option." << std::endl;
            } catch (const std::out_of_range& e) {
                std::cout << "Number out of range. Please enter a valid number." << std::endl;
            }
        }
    }
    return "";
}
