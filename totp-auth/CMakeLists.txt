pkg_check_modules(LIBOATH REQUIRED liboath --static)

add_library(proxy-totp-auth SHARED totp_auth.c)

target_include_directories(proxy-totp-auth PRIVATE
    ${FREERDP_INCLUDE_DIRS}
    ${LIBOATH_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(proxy-totp-auth PRIVATE
    freerdp-server
    freerdp
    ${LIBOATH_STATIC_LIBRARIES}
    text_renderer
)

set_target_properties(proxy-totp-auth PROPERTIES PREFIX "")
