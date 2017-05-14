cmake_minimum_required(VERSION 3.0)
project(vktest CXX)

if(NOT CMAKE_BUILD_TYPE)
    message("Defaulting to RELEASE build.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "'Debug' or 'Release'" FORCE)
endif()

find_package(PkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(XCB REQUIRED xcb)
pkg_search_module(X11XCB REQUIRED x11-xcb)
pkg_search_module(Vulkan QUIET vulkan)
if(NOT Vulkan_FOUND)
    # If pkg-config wasn't able to find your build of vulkan, you need to
    # point it in the right direction.  Here, I'm telling pkg-config that
    # I keep my vulkan.pc file in the ~/.local/lib/pkgconfig/ directory.
    set(ENV{PKG_CONFIG_PATH} "$ENV{HOME}/.local/lib/pkgconfig/")
    pkg_search_module(Vulkan REQUIRED vulkan)
endif()

add_subdirectory(src)
