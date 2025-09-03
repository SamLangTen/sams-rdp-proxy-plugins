#include "text_renderer.h"
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <string.h>

#ifndef FONT_PATH
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#endif

void render_text(BYTE* buffer, int width, int height, const char* text)
{
    FT_Library ft;
    FT_Face face;
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "[Renderer] FreeType 初始化失败\n");
        return;
    }

    if (FT_New_Face(ft, FONT_PATH, 0, &face)) {
        fprintf(stderr, "[Renderer] 加载字体失败: %s\n", FONT_PATH);
        FT_Done_FreeType(ft);
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 32);

    int pen_x = 100, pen_y = height / 2;

    for (const char* p = text; *p; p++) {
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = face->glyph;

        for (int row = 0; row < g->bitmap.rows; row++) {
            for (int col = 0; col < g->bitmap.width; col++) {
                int x = pen_x + g->bitmap_left + col;
                int y = pen_y - g->bitmap_top + row;
                if (x < 0 || x >= width || y < 0 || y >= height) continue;

                BYTE gray = g->bitmap.buffer[row * g->bitmap.pitch + col];
                int offset = (y * width + x) * 4;
                buffer[offset + 0] = gray; // B
                buffer[offset + 1] = gray; // G
                buffer[offset + 2] = gray; // R
                buffer[offset + 3] = 0x00; // A
            }
        }

        pen_x += (g->advance.x >> 6);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}
