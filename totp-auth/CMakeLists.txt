pkg_check_modules(LIBOATH REQUIRED liboath)

add_library(proxy-totp-auth SHARED totp_auth.c)

target_include_directories(proxy-totp-auth PRIVATE
    ${FREERDP_INCLUDE_DIRS}
    ${LIBOATH_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${WINPR_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(proxy-totp-auth PRIVATE
    ${FREERDP_SERVER_LIBRARY_DIRS}
    ${FREERDP_SERVER_PROXY_LIBRARY_DIRS}
    ${LIBOATH_STATIC_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)

target_link_libraries(proxy-totp-auth PRIVATE
    ${FREERDP_SERVER_LIBRARIES}
    ${FREERDP_SERVER_PROXY_LIBRARIES}
    ${LIBOATH_STATIC_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    text_renderer
)

set_target_properties(proxy-totp-auth PROPERTIES PREFIX "")
