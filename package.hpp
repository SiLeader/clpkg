//
// Created by sileader on 18/07/09.
//

#ifndef CLPKG_PACKAGE_HPP
#define CLPKG_PACKAGE_HPP

#include <string>
#include <vector>

#include <json11.hpp>
#include <algorithm>
#include "settings.hpp"
#include "downloader.hpp"

namespace clpkg {
    class package_error : std::runtime_error {
        using runtime_error::runtime_error;
    };

    class package_info {
    private:
        std::string _name, _version;
        int _code;
        bool _is_build_required;
        std::string _build_command;
        std::vector<std::tuple<std::string /* name */, std::string /* version */>> _dependencies;

    public:
        package_info() {}
    public:
        package_info(const std::string& name, const std::string& version, int code,
                     bool is_build_required, const std::string& command, const std::vector<std::tuple<std::string, std::string>>& dep)
                : _name(name), _version(version), _code(code), _is_build_required(is_build_required), _build_command(command), _dependencies(dep){
        }

        static package_info from_json(const json11::Json& json) {
            const auto& items = json.object_items();
            if(items.count("name") == 0) {
                throw package_error("Required package info is missing. key: 'name'");
            }
            if(items.count("version") == 0) {
                throw package_error("Required package info is missing. key: 'version'");
            }
            bool is_build_required = false;
            std::string command;
            if(items.count("build") != 0) {
                if(json["build"].object_items().count("required") != 0) {
                    is_build_required = json["build"]["required"].bool_value();
                }
                if(is_build_required) {
                    if(json["build"].object_items().count("command") != 0) {
                        command = json["build"]["command"].string_value();
                    }
                }
            }

            std::vector<std::tuple<std::string, std::string>> dep;
            if(items.count("dependencies") != 0) {
                for(const auto& d : json["dependencies"].object_items()) {
                    if(!d.second.is_string()) {
                        throw package_error("Package dependency is not valid type.");
                    }
                    dep.emplace_back(d.first, d.second.string_value());
                }
            }

            return package_info(json["name"].string_value(), json["version"]["name"].string_value(), json["version"]["code"].int_value(), is_build_required, command, dep);
        }

        static package_info from_json(const std::string& json_str) {
            std::string err;
            auto json = json11::Json::parse(json_str, err);

            return from_json(json);
        }

        static std::vector<package_info> from_json_array(const std::string& json_str) {
            std::string err;
            auto json = json11::Json::parse(json_str, err);

            const auto& items = json.array_items();
            std::vector<package_info> pinfos(items.size());
            std::transform(std::begin(items), std::end(items), std::begin(pinfos), [&pinfos](const json11::Json& j) {
                return package_info::from_json(j);
            });

            return pinfos;
        }
        package_info(const package_info&)=default;
        package_info(package_info&&)=default;
        package_info& operator=(const package_info&)=default;
        package_info& operator=(package_info&&)=default;

    public:
        const std::string& name()const noexcept {
            return _name;
        }
        const std::string& version()const noexcept {
            return _version;
        }
        int version_code()const noexcept {
            return _code;
        }
        bool is_build_required()const noexcept {
            return _is_build_required;
        }
        const std::string& build_command() const noexcept {
            return _build_command;
        }

        void download(const std::string& dir)const {
            auto file_name = settings().temporary_directory() + "/" + name() + version();
            downloader()
        }
    };

    bool operator==(const package_info& lhs, const package_info& rhs)noexcept {
        return lhs.version_code() == rhs.version_code() && lhs.name() == rhs.name();
    }
    bool operator!=(const package_info& lhs, const package_info& rhs)noexcept {
        return !(lhs == rhs);
    }
    bool operator<(const package_info& lhs, const package_info& rhs)noexcept {
        return lhs.version_code() < rhs.version_code() && lhs.name() < rhs.name();
    }
    bool operator<=(const package_info& lhs, const package_info& rhs)noexcept {
        return lhs < rhs || lhs == rhs;
    }
    bool operator>(const package_info& lhs, const package_info& rhs)noexcept {
        return !(lhs <= rhs);
    }
    bool operator>=(const package_info& lhs, const package_info& rhs)noexcept {
        return !(lhs < rhs);
    }
} /* clpkg */

#endif //CLPKG_PACKAGE_HPP
