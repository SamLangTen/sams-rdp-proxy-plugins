#include "totp_auth.h"
#include "text_renderer.h"
#include <freerdp/server/proxy/proxy_config.h>
#include <freerdp/server/proxy/proxy_context.h>
#include <freerdp/channels/rdpdr.h>
#include <oath.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static char g_secret[128] = "JBSWY3DPEHPK3PXP";
static int g_window = 30;
static int g_verified = 0; // 0=未验证, 1=成功, -1=失败

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

/* 从 config.ini 读取配置 */
static void load_totp_config(const proxyData* data)
{
    const proxyConfig* config = data->config;
    const char* secret = pf_config_get(config, "totp", "Secret");
    const char* window_str = pf_config_get(config, "totp", "Window");

    if (secret)
        strncpy(g_secret, secret, sizeof(g_secret) - 1);
    if (window_str)
        g_window = atoi(window_str);
    else
        g_window = 30;

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

/* 会话开始时调用 */
static BOOL totp_auth_on_session_start(proxyPlugin* plugin, proxyData* data, void* custom)
{
    load_totp_config(data);
    g_verified = 0;

    printf("[TOTP] 等待用户输入验证码...\n");
    printf("[TOTP] 请在客户端上操作以完成验证\n");

    // 阻塞直到验证完成（通过键盘事件）
    while (g_verified == 0) {
        if (proxy_data_shall_disconnect(data)) {
            printf("[TOTP] 连接已断开\n");
            return FALSE;
        }
        sleep(1);
    }

    if (g_verified == 1) {
        printf("[TOTP] 验证成功\n");
        return TRUE;
    } else {
        printf("[TOTP] 验证失败\n");
        return FALSE;
    }
}

/* 键盘输入过滤器 */
static BOOL totp_on_keyboard_event(proxyPlugin* plugin, proxyData* data, void* param)
{
    proxyKeyboardEventInfo* info = (proxyKeyboardEventInfo*)param;

    // 只处理数字键和回车
    UINT16 code = info->rdp_scan_code;
    UINT16 flags = info->flags;

    // 检查是否是按键释放事件 (KBD_FLAGS_RELEASE = 0x8001 or 0x8000)
    // 在 FreeRDP 3.x 中，flags 的含义可能不同
    // 我们假设 flags & 0x8000 表示释放

    // 简化：我们只检测输入，不实现完整的输入缓冲
    // 在实际应用中，需要维护输入状态

    // 这里我们打印键盘事件，用于调试
    printf("[TOTP] 键盘事件: flags=0x%04X code=0x%04X\n", flags, code);

    // 如果是回车键，我们验证一个默认的测试码
    // 在实际应用中，需要维护完整的输入缓冲
    if (code == 0x1C) { // Enter key
        const char* test_code = "123456"; // 测试用
        if (validate_totp(test_code)) {
            g_verified = 1;
        } else {
            g_verified = -1;
        }
    }

    return TRUE; // 允许事件通过
}

/* 插件入口点 */
BOOL proxy_plugin_entry(proxyPluginsManager* plugins_manager, void* userdata)
{
    proxyPlugin plugin = { 0 };

    plugin.name = "totp-auth";
    plugin.description = "TOTP two-factor authentication plugin";

    // 注册 ServerSessionStarted 钩子
    plugin.ServerSessionStarted = totp_auth_on_session_start;

    // 注册键盘事件过滤器
    plugin.KeyboardEvent = totp_on_keyboard_event;

    // 注册插件
    if (!plugins_manager->RegisterPlugin(plugins_manager, &plugin)) {
        fprintf(stderr, "[TOTP] 注册插件失败\n");
        return FALSE;
    }

    printf("[TOTP] 插件已注册\n");
    return TRUE;
}
