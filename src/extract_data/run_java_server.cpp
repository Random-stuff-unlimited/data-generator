#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

bool run_data_generator() {
    fs::path data_dir = "data";
    fs::path server_jar = data_dir / "server.jar";

    if (!fs::exists(server_jar)) {
        std::cerr << "Error: server.jar not found in /data. Cannot generate reports." << std::endl;
        return false;
    }

    fs::current_path(data_dir);
    std::string command = "java -DbundlerMainClass=net.minecraft.data.Main -jar server.jar --reports --server";

    std::cout << "\n==============================================" << std::endl;
    std::cout << "Starting Mojang Data Generator..." << std::endl;
    std::cout << "Executing: " << command << std::endl;
    std::cout << "==============================================\n" << std::endl;

    int return_code = std::system(command.c_str());

    if (return_code == 0) {
        std::cout << "\n[SUCCESS] Data generation complete!" << std::endl;
        std::cout << "You can find your files in: data/generated/reports/ and data/generated/data/" << std::endl;
        return true;
    } else {
        std::cerr << "\n[ERROR] Data generator failed with code: " << return_code << std::endl;
        std::cerr << "Make sure Java is installed and visible in your system PATH." << std::endl;
        return false;
    }
}
