#include "downloader.hpp"
#include <cstddef>
#include <cstdio>
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <string>

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

bool download_file(const std::string& url, const std::string& destination) {
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(destination.c_str(), "wb");
        if (fp == nullptr) {
            std::cerr << "Error opening file: " << destination << std::endl;
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        fclose(fp);

        return (res == CURLE_OK);
    }
    return false;
}


size_t write_to_string(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t total_size = size * nmemb;

    std::string *response_string = static_cast<std::string*>(stream);
    response_string->append(static_cast<char*>(ptr), total_size);

    return total_size;
}

std::string download_manifest_to_ram(const std::string& url) {
    CURL *curl;
    CURLcode res;
    std::string response_data;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            response_data = "";
        }

        curl_easy_cleanup(curl);
    }

    return response_data;
}
