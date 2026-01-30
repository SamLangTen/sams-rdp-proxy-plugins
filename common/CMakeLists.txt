add_library(text_renderer STATIC text_renderer.c)

target_include_directories(text_renderer PUBLIC
    ${FREETYPE_INCLUDE_DIRS}
    ${WINPR_INCLUDE_DIRS}
)

target_link_libraries(text_renderer PUBLIC
    ${FREETYPE_LIBRARIES}
    ${WINPR_LIBRARIES}
)

target_compile_definitions(text_renderer PUBLIC
    FONT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/fonts/DejaVuSans.ttf"
)
