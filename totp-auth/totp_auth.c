#include "totp_auth.h"
#include "text_renderer.h"
#include <freerdp/server/proxy/proxy_config.h>
#include <freerdp/update.h>
#include <oath.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static char g_secret[128] = "JBSWY3DPEHPK3PXP";
static int g_window = 30;
static char g_input[16] = {0};
static int g_input_len = 0;
static int g_verified = 0; // 0=未验证, 1=成功, -1=失败

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

/* 从 config.ini 读取配置 */
static void load_totp_config(const proxyServerConnection* conn)
{
    const proxyConfig* config = conn->context->config;
    const char* secret = proxy_config_get_string(config, "totp", "Secret");
    int window = proxy_config_get_int(config, "totp", "Window", 30);

    if (secret)
        strncpy(g_secret, secret, sizeof(g_secret) - 1);
    g_window = window;

    printf("[TOTP] 配置已加载: secret=%s window=%d\n", g_secret, g_window);
}

/* 验证 TOTP */
static int validate_totp(const char* code)
{
    int rc = oath_init();
    if (rc != OATH_OK) return 0;

    time_t now = time(NULL);
    rc = oath_totp_validate(g_secret, strlen(g_secret),
                            now, g_window, 0, 6, code);

    oath_done();
    return rc >= 0; // 成功返回匹配的时间偏移
}

/* 绘制页面 */
static void draw_totp_screen(rdpContext* context, const char* prompt, const char* input)
{
    rdpUpdate* update = context->update;
    if (!update) return;

    int bpp = 32;
    size_t size = SCREEN_WIDTH * SCREEN_HEIGHT * (bpp / 8);
    BYTE* buffer = (BYTE*)calloc(1, size);
    if (!buffer) return;

    memset(buffer, 0x00, size); // 黑色背景

    char msg[256];
    snprintf(msg, sizeof(msg), "%s %s", prompt, input ? input : "");
    render_text(buffer, SCREEN_WIDTH, SCREEN_HEIGHT, msg);

    SURFACE_BITS_COMMAND cmd = { 0 };
    cmd.destLeft = 0;
    cmd.destTop = 0;
    cmd.destRight = SCREEN_WIDTH;
    cmd.destBottom = SCREEN_HEIGHT;
    cmd.bpp = bpp;
    cmd.codecID = 0;
    cmd.width = SCREEN_WIDTH;
    cmd.height = SCREEN_HEIGHT;
    cmd.bitmapDataLength = size;
    cmd.bitmapData = buffer;

    update->SurfaceBits(update->context, &cmd);
    free(buffer);
}

/* 捕获键盘输入 */
static BOOL totp_on_keyboard_event(proxyPlugin* plugin,
                                   const proxyServerConnection* conn,
                                   UINT16 flags, UINT16 code)
{
    // 只关心按下事件
    if (!(flags & KBD_FLAGS_DOWN)) return TRUE;

    // 数字键 0-9
    if (code >= 0x0B && code <= 0x12) { // RDP scancode 对应数字键
        int digit = (code - 0x0B) % 10;
        if (g_input_len < 6) {
            g_input[g_input_len++] = '0' + digit;
            g_input[g_input_len] = '\0';
        }
    }
    // 回退键
    else if (code == 0x0E && g_input_len > 0) {
        g_input[--g_input_len] = '\0';
    }
    // 回车键
    else if (code == 0x1C && g_input_len == 6) {
        if (validate_totp(g_input)) {
            g_verified = 1;
        } else {
            g_verified = -1;
        }
    }

    // 刷新界面
    draw_totp_screen(conn->context,
                     (g_verified == -1 ? "验证失败，请重新输入:" : "请输入动态验证码:"),
                     g_input);

    return TRUE;
}

/* 会话开始时调用 */
static void totp_auth_on_connect(proxyPlugin* plugin, const proxyServerConnection* conn)
{
    load_totp_config(conn);

    g_input_len = 0;
    g_verified = 0;
    memset(g_input, 0, sizeof(g_input));

    draw_totp_screen(conn->context, "请输入动态验证码:", "");

    // 阻塞直到用户输入成功
    while (g_verified == 0) {
        sleep(1);
    }

    if (g_verified == 1) {
        draw_totp_screen(conn->context, "验证成功!", "");
        printf("[TOTP] 验证成功\n");
    } else {
        draw_totp_screen(conn->context, "验证失败!", "");
        printf("[TOTP] 验证失败\n");
        exit(1);
    }
}

PROXY_API int proxy_plugin_init(proxyPlugin* plugin, const proxyServerConnection* connection)
{
    plugin->OnServerSessionStart = totp_auth_on_connect;
    plugin->KeyboardEvent = totp_on_keyboard_event;
    return 0;
}
