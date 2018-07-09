cmake_minimum_required(VERSION 3.10)
project(clpkg)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(json11)
add_subdirectory(curl)

include_directories(curl/include)

add_executable(clpkg main.cpp args.hpp package.hpp site.hpp downloader.hpp settings.hpp)
target_link_libraries(clpkg json11 libcurl)
