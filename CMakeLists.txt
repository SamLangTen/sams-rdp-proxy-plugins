cmake_minimum_required(VERSION 3.13)
project(FreeRDPProxyPlugins C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(FREERDP REQUIRED freerdp3)
pkg_check_modules(WINPR REQUIRED winpr3)
pkg_check_modules(FREERDP_SERVER REQUIRED freerdp-server3)
pkg_check_modules(FREERDP_SERVER_PROXY REQUIRED freerdp-server-proxy3)

add_subdirectory(common)
add_subdirectory(totp-auth)
add_subdirectory(wol-wait)
