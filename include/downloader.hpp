#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>

bool		download_file(const std::string& url, const std::string& destination);
std::string	download_manifest_to_ram(const std::string& url);

#endif
