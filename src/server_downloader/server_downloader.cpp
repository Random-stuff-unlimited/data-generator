#include "downloader.hpp"
#include "json.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <curl/curl.h>
#include "server_downloader.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

int download_java_server() {
 std::string url = "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";

    std::cout << "Downloading version manifest..." << std::endl;
    std::string manifest_content = download_manifest_to_ram(url);

    if (manifest_content.empty()) {
        std::cerr << "Failed to download manifest or manifest is empty." << std::endl;
        return 1;
    }

    std::cout << "Manifest downloaded, parsing..." << std::endl;
    json manifest = json::parse(manifest_content);

    std::string version_url = selectVersion(manifest);
    if (version_url.empty()) {
        std::cerr << "No version was selected. Aborting." << std::endl;
        return 1;
    }

    std::cout << "Downloading specific version manifest..." << std::endl;
    std::string version_content = download_manifest_to_ram(version_url);
    if (version_content.empty()) {
        std::cerr << "Failed to download specific version data." << std::endl;
        return 1;
    }

    json version_json = json::parse(version_content);
    if (!version_json.contains("downloads") || !version_json["downloads"].contains("server")) {
        std::cerr << "Selected version does not provide a server jar download." << std::endl;
        return 1;
    }

    std::string server_url = version_json["downloads"]["server"]["url"].get<std::string>();

    fs::path data_dir = "data";
    if (!fs::exists(data_dir)) {
        fs::create_directory(data_dir);
    }

    fs::path server_jar_path = data_dir / "server.jar";
    if (fs::exists(server_jar_path)) {
        std::cout << "Existing server.jar found. Removing old file..." << std::endl;
        fs::remove(server_jar_path);
    }

    std::cout << "Downloading server.jar..." << std::endl;
    if (download_file(server_url, server_jar_path.string())) {
        std::cout << "Server jar successfully downloaded to " << server_jar_path.string() << std::endl;
    } else {
        std::cerr << "Failed to download server.jar." << std::endl;
        return 1;
    }

    return 0;
}
