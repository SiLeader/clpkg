#include <iostream>

#include "package.hpp"
#include "args.hpp"
#include "settings.hpp"

namespace {
    constexpr char VERSION[] = "0.0.1";

    void before_exit() {
        sstd::fs::remove(sstd::fs::path(clpkg::settings().temporary_directory()));
    }
} /* anonymous */

namespace {
    int installer(const args::argument_parser& ins) {return 0;}
    int uninstaller(const args::argument_parser& uin) {return 0;}
} /* anonymous */

int main(int argc, char **argv) {
    args::argument_parser parser("clpkg: C/C++ Libraries Package manager", "PROGRAM [flags]... [positional]...", "Released under the Apache License 2.0");
    parser.add_flag({"--version"}, "show version");
    auto install = parser.add_subcommand("install", "install library");
    auto uninstall = parser.add_subcommand("uninstall", "uninstall library");

    parser.parse_args(argc, argv);

    if(parser.exists("--version")) {
        std::cout<<"clpkg version "<<VERSION<<'\n';
        std::exit(0);
    }

    sstd::fs::create_directory(sstd::fs::path(clpkg::settings().temporary_directory()));

    if(install.is_selected()) {
        return installer(install);
    }
    if(uninstall.is_selected()) {
        return uninstaller(uninstall);
    }
    return 0;
}
