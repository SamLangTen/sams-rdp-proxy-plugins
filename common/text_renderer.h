#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <winpr/wtypes.h>

/* 渲染一行文本到 RGBA 缓冲区 */
void render_text(BYTE* buffer, int width, int height, const char* text);

#endif // TEXT_RENDERER_H
