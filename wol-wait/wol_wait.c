#include "wol_wait.h"
#include "text_renderer.h"

#include <freerdp/server/proxy/proxy_config.h>
#include <wakeonlan/wol.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include <freerdp/update.h>

static char g_mac[32] = "00:11:22:33:44:55";
static char g_host[128] = "127.0.0.1";
static int g_port = 3389;
static int g_timeout  = 60;
static int g_interval = 2;

#define WAIT_WIDTH  800
#define WAIT_HEIGHT 600

/* 从 config.ini 读取配置 */
static void load_wol_config(const proxyServerConnection* conn)
{
    const proxyConfig* config = conn->context->config;

    const char* mac = proxy_config_get_string(config, "wol", "Mac");
    g_timeout  = proxy_config_get_int(config, "wol", "Timeout", 60);
    g_interval = proxy_config_get_int(config, "wol", "Interval", 2);
    if (mac) strncpy(g_mac, mac, sizeof(g_mac) - 1);

    const char* host = proxy_config_get_string(config, "Target", "Host");
    int port = proxy_config_get_int(config, "Target", "Port", 3389);
    if (host) strncpy(g_host, host, sizeof(g_host) - 1);
    g_port = port;

    printf("[WOL] 配置已加载: MAC=%s Target=%s:%d Timeout=%d Interval=%d\n",
           g_mac, g_host, g_port, g_timeout, g_interval);
}

/* 用 libwakeonlan 发送魔术包 */
static void send_wol(const char* mac)
{
    wol_packet_t* packet = wol_create_packet_from_string(mac);
    if (!packet) {
        fprintf(stderr, "[WOL] 无法生成魔术包，MAC=%s\n", mac);
        return;
    }

    if (wol_send_packet(packet, "255.255.255.255", 9) != 0) {
        fprintf(stderr, "[WOL] 发送失败\n");
    } else {
        printf("[WOL] 魔术包已发送到 %s\n", mac);
    }

    wol_free_packet(packet);
}

/* socket 探测目标端口 */
static int is_host_up(const char* host, int port)
{
    struct addrinfo hints, *res = NULL;
    int sock = -1;
    int ret = 0;
    char port_str[16];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(port_str, sizeof(port_str), "%d", port);
    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return 0;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return 0;
    }

    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, res->ai_addr, res->ai_addrlen) == 0) {
        ret = 1;
    }

    close(sock);
    freeaddrinfo(res);
    return ret;
}

/* 绘制等待页面 */
static void draw_waiting_screen(rdpContext* context, int waited, int total)
{
    rdpUpdate* update = context->update;
    if (!update) return;

    int width = WAIT_WIDTH;
    int height = WAIT_HEIGHT;
    int bpp = 32;
    size_t size = width * height * (bpp / 8);

    BYTE* buffer = (BYTE*)calloc(1, size);
    if (!buffer) return;

    memset(buffer, 0x00, size); // 黑色背景

    char msg[256];
    snprintf(msg, sizeof(msg), "目标正在启动，请稍候... (已等待 %d / %d 秒)", waited, total);

    render_text(buffer, width, height, msg);

    SURFACE_BITS_COMMAND cmd = { 0 };
    cmd.destLeft = 0;
    cmd.destTop = 0;
    cmd.destRight = width;
    cmd.destBottom = height;
    cmd.bpp = bpp;
    cmd.codecID = 0;
    cmd.width = width;
    cmd.height = height;
    cmd.bitmapDataLength = size;
    cmd.bitmapData = buffer;

    update->SurfaceBits(update->context, &cmd);
    free(buffer);

    printf("[WOL] 等待页面已绘制: %s\n", msg);
}

static void wol_wait_on_connect(proxyPlugin* plugin, const proxyServerConnection* conn)
{
    load_wol_config(conn);

    if (!is_host_up(g_host, g_port)) {
        send_wol(g_mac);
        printf("[WOL] 目标未启动，显示等待页面...\n");

        int waited = 0;
        while (waited < g_timeout) {
            draw_waiting_screen(conn->context, waited, g_timeout);
            sleep(g_interval);
            waited += g_interval;
            if (is_host_up(g_host, g_port)) {
                printf("[WOL] 目标已上线\n");
                return;
            }
        }
        printf("[WOL] 超时，目标未启动\n");
        exit(1);
    } else {
        printf("[WOL] 目标已在线\n");
    }
}

PROXY_API int proxy_plugin_init(proxyPlugin* plugin, const proxyServerConnection* connection)
{
    plugin->OnServerSessionStart = wol_wait_on_connect;
    return 0;
}
