//
// Created by sileader on 18/07/09.
//

#ifndef CLPKG_SETTINGS_HPP
#define CLPKG_SETTINGS_HPP

#include <string>
#include <vector>
#include <fstream>

#if __has_include(<filesystem>)
#   include <filesystem>
namespace sstd {
    namespace fs = std::filesystem;
} /* sstd */
#else
#   include <experimental/filesystem>
namespace sstd {
    namespace fs = std::experimental::filesystem;
} /* sstd */
#endif

#include <json11.hpp>
#include <algorithm>

#include <unistd.h>

namespace clpkg {
    class settings {
    private:
        std::string _config;

    public:
        settings() : _config(std::string(getenv("HOME")) + "/.clpkg") {}
        settings(const settings&)=default;
        settings(settings&&)=default;
        settings& operator=(const settings&)=default;
        settings& operator=(settings&&)=default;

    public:
        const std::string& config()const noexcept {
            return _config;
        }
        std::string cache()const {
            return _config + "/.cache";
        }
        std::string sites_directory()const {
            return _config + "/sites";
        }
        std::string temporary_directory()const {
            return sstd::fs::temp_directory_path().string() + "/" + std::to_string(getpid());
        }

        std::vector<std::string> package_sites()const {
            sstd::fs::directory_iterator ditr(sstd::fs::path(sites_directory()));
            std::vector<sstd::fs::path> paths(sstd::fs::begin(ditr), sstd::fs::end(ditr));

            std::vector<std::string> dirs(paths.size());
            std::transform(std::begin(paths), std::end(paths), std::begin(dirs), [](const sstd::fs::path& p) {
                return p.string();
            });
            return dirs;
        }
    };
} /* clpkg */

#endif //CLPKG_SETTINGS_HPP
