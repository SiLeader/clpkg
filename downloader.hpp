//
// Created by sileader on 18/07/09.
//

#ifndef CLPKG_DOWNLOADER_HPP
#define CLPKG_DOWNLOADER_HPP

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>

#include <curl/curl.h>

namespace clpkg {
    std::vector<char> downloader(const std::string& url) {
        auto curl = curl_easy_init();
        if(!curl) {
            throw std::runtime_error("curl_easy_init failed.");
        }
        std::vector<char> buf;

        auto callback = [](void *buf, std::size_t size, std::size_t nmemb, void *userp) {
            auto b = *static_cast<std::vector<char>*>(userp);
            auto cb = static_cast<const char*>(buf);

            b.insert(std::end(b), buf, buf + (size + nmemb));
        };

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

        auto ret = curl_easy_perform(curl);

        curl_easy_cleanup(curl);

        if(ret == 0) {
            return buf;
        }
        return buf;
    }
    std::string to_string(const std::vector<char>& str) {
        return std::string(std::begin(str), std::end(str));
    }
} /* clpkg */

#endif //CLPKG_DOWNLOADER_HPP
