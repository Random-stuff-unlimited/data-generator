cmake_minimum_required(VERSION 3.10)
project(serveur_downloader)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajouter les fichiers source
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*/*.cpp"
)

# Créer l'exécutable
add_executable(serveur_downloader ${SOURCES})

# Include directories
target_include_directories(serveur_downloader PRIVATE include)

# Trouver et lier curl
find_package(CURL REQUIRED)
target_link_libraries(serveur_downloader PRIVATE CURL::libcurl)
