#pragma once

#include "json.hpp"
#include <string>

struct VersionInfo {
    std::string id;
    std::string type;
    std::string url;
    std::string time;
    std::string releaseTime;
    std::string sha1;
    int complianceLevel;
};

std::string selectVersion(const nlohmann::json& manifest);
int	download_java_server();
