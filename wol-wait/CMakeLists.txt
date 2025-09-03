pkg_check_modules(LIBWOL REQUIRED libwakeonlan --static)

add_library(proxy-wol-wait SHARED
    wol_wait.c
)

target_include_directories(proxy-wol-wait PRIVATE
    ${FREERDP_INCLUDE_DIRS}
    ${LIBWOL_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(proxy-wol-wait PRIVATE
    freerdp-server
    freerdp
    ${LIBWOL_STATIC_LIBRARIES}
    text_renderer
)

set_target_properties(proxy-wol-wait PROPERTIES PREFIX "")
