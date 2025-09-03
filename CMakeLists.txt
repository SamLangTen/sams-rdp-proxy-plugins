cmake_minimum_required(VERSION 3.13)
project(FreeRDPProxyPlugins C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)

add_subdirectory(common)
add_subdirectory(totp-auth)
add_subdirectory(wol-wait)
