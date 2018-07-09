//
// Created by sileader on 18/07/09.
//

#ifndef CLPKG_SITE_HPP
#define CLPKG_SITE_HPP

#include <fstream>
#include <unordered_map>
#include <algorithm>

#if __has_include(<optional>)
#   include <optional>
namespace sstd {
    using std::optional;
    inline constexpr auto nullopt = std::nullopt;
} /* sstd */
#else
#   include <experimental/optional>
namespace sstd {
    using std::experimental::optional;
    inline constexpr auto nullopt = std::experimental::nullopt;
} /* sstd */
#endif

#include "package.hpp"
#include "downloader.hpp"
#include "settings.hpp"

namespace clpkg {
    class site {
    private:
        std::string _url;
        std::unordered_map<std::string, std::vector<package_info>> _packages;

    public:
        explicit site(const std::string& url) : _url(url) {
            load_package_list_from_cache();
        }
        site()=delete;

        site(const site&)=default;
        site(site&&)=default;
        site& operator=(const site&)=default;
        site& operator=(site&&)=default;

    private:
        std::string _cache_path() {
            std::string url = _url;
            std::replace(std::begin(url), std::end(url), '/', '@');
            return settings().cache() + "/sites/" + url + ".json";
        }

        void _load_impl(const std::vector<package_info>& pi, bool clear_cache=true) {
            if(clear_cache)_packages.clear();
            for(const auto& p : pi) {
                _packages[p.name()].emplace_back(p);
            }
        }

    public:
        void download_package_list() {
            auto data = to_string(downloader(_url + "/packages"));
            auto packages = package_info::from_json_array(data);
            _load_impl(packages);

            std::ofstream fout(_cache_path());
            fout<<data<<std::endl;
        }

        bool load_package_list_from_cache() {
            std::ifstream fin(_cache_path());
            if(!fin)return false;

            auto packages = package_info::from_json_array(
                    std::string(
                            std::istreambuf_iterator<char>(fin),
                            std::istreambuf_iterator<char>()
                    )
            );
            _load_impl(packages);
            return true;
        }

    public:
        const std::vector<package_info>& operator[](const std::string& name)const {
            if(_packages.count(name) == 0)return {};
            return _packages.at(name);
        }

        std::size_t size()const {
            return std::accumulate(std::begin(_packages), std::end(_packages), 0ul, [](const std::vector<package_info>& lhs, const std::vector<package_info>& rhs) {
                return lhs.size() + rhs.size();
            });
        }
    };

    namespace detail {
        inline std::vector<site> to_sites(const std::vector<std::string>& s) {
            std::vector<site> ss(s.size());

            std::transform(std::begin(s), std::end(s), std::begin(ss), [](const std::string& s) {return site(s);});
            return ss;
        }
    } /* detail */

    class sites {
    private:
        std::vector<site> _sites;

    public:
        sites(): _sites(detail::to_sites(settings().package_sites())) {
        }
        sites(const sites&)=default;
        sites(sites&&)=default;
        sites& operator=(const sites&)=default;
        sites& operator=(sites&&)=default;

    public:
        std::vector<package_info> operator[](const std::string& name)const {
            auto size = std::accumulate(std::begin(_sites), std::end(_sites), 0ul, [](const site& lhs, const site& rhs) {
                return lhs.size() + rhs.size();
            });
            std::vector<package_info> pinfos;
            pinfos.reserve(size);

            for(const auto& site : _sites) {
                const auto& pkg = site[name];
                pinfos.insert(std::end(pinfos), std::begin(pkg), std::end(pkg));
            }
            std::sort(std::begin(pinfos), std::end(pinfos));
            pinfos.erase(std::unique(std::begin(pinfos), std::end(pinfos)), std::end(pinfos));
            return pinfos;
        }
    };
} /* clpkg */

#endif //CLPKG_SITE_HPP
