add_library(proxy-wol-wait SHARED
    wol_wait.c
)

target_include_directories(proxy-wol-wait PRIVATE
    ${FREERDP_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${WINPR_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_directories(proxy-wol-wait PRIVATE
    ${FREERDP_SERVER_LIBRARY_DIRS}
    ${FREERDP_SERVER_PROXY_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)

target_link_libraries(proxy-wol-wait PRIVATE
    ${FREERDP_SERVER_LIBRARIES}
    ${FREERDP_SERVER_PROXY_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    text_renderer
)

set_target_properties(proxy-wol-wait PROPERTIES PREFIX "")
