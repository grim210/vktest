cmake_minimum_required(VERSION 3.0)
project(vktest CXX)

find_package(Vulkan)
find_package(PkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

add_subdirectory(src)
