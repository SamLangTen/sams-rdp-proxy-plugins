#include "wol_wait.h"
#include "text_renderer.h"

#include <freerdp/server/proxy/proxy_config.h>
#include <freerdp/server/proxy/proxy_context.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

static char g_mac[32] = "00:11:22:33:44:55";
static char g_host[128] = "127.0.0.1";
static int g_port = 3389;
static int g_timeout  = 60;
static int g_interval = 2;

#define WAIT_WIDTH  800
#define WAIT_HEIGHT 600

/* 从 config.ini 读取配置 */
static void load_wol_config(const proxyData* data)
{
    const proxyConfig* config = data->config;

    const char* mac = pf_config_get(config, "wol", "Mac");
    const char* timeout_str = pf_config_get(config, "wol", "Timeout");
    const char* interval_str = pf_config_get(config, "wol", "Interval");

    if (mac) strncpy(g_mac, mac, sizeof(g_mac) - 1);
    if (timeout_str) g_timeout = atoi(timeout_str);
    else g_timeout = 60;
    if (interval_str) g_interval = atoi(interval_str);
    else g_interval = 2;

    const char* host = pf_config_get(config, "Target", "Host");
    const char* port_str = pf_config_get(config, "Target", "Port");

    if (host) strncpy(g_host, host, sizeof(g_host) - 1);
    if (port_str) g_port = atoi(port_str);
    else g_port = 3389;

    printf("[WOL] 配置已加载: MAC=%s Target=%s:%d Timeout=%d Interval=%d\n",
           g_mac, g_host, g_port, g_timeout, g_interval);
}

/* 将 MAC 地址字符串转换为 6 字节数组 */
static int parse_mac(const char* mac_str, unsigned char* mac_bytes)
{
    int mac_hex[6];
    if (sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
               &mac_hex[0], &mac_hex[1], &mac_hex[2],
               &mac_hex[3], &mac_hex[4], &mac_hex[5]) != 6) {
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        mac_bytes[i] = (unsigned char)mac_hex[i];
    }
    return 0;
}

/* 发送 WOL 魔术包 */
static void send_wol(const char* mac)
{
    int sock = -1;
    struct sockaddr_in addr;
    unsigned char mac_bytes[6];
    unsigned char packet[102];
    int i;

    /* 解析 MAC 地址 */
    if (parse_mac(mac, mac_bytes) != 0) {
        fprintf(stderr, "[WOL] MAC 地址格式错误: %s\n", mac);
        return;
    }

    /* 创建魔术包 */
    memset(packet, 0xFF, 6);
    for (i = 1; i <= 16; i++) {
        memcpy(packet + i * 6, mac_bytes, 6);
    }

    /* 创建 UDP socket */
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        fprintf(stderr, "[WOL] 创建 socket 失败: %s\n", strerror(errno));
        return;
    }

    /* 设置广播选项 */
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        fprintf(stderr, "[WOL] 设置广播选项失败: %s\n", strerror(errno));
        close(sock);
        return;
    }

    /* 发送到广播地址 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);
    addr.sin_addr.s_addr = INADDR_BROADCAST;

    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[WOL] 发送失败: %s\n", strerror(errno));
    } else {
        printf("[WOL] 魔术包已发送到 %s\n", mac);
    }

    close(sock);
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

/* 会话开始时调用 */
static BOOL wol_wait_on_session_start(proxyPlugin* plugin, proxyData* data, void* custom)
{
    load_wol_config(data);

    if (!is_host_up(g_host, g_port)) {
        send_wol(g_mac);
        printf("[WOL] 目标未启动，等待目标上线...\n");

        int waited = 0;
        while (waited < g_timeout) {
            if (proxy_data_shall_disconnect(data)) {
                printf("[WOL] 连接已断开\n");
                return FALSE;
            }

            sleep(g_interval);
            waited += g_interval;

            if (is_host_up(g_host, g_port)) {
                printf("[WOL] 目标已上线 (等待了 %d 秒)\n", waited);
                return TRUE;
            }

            printf("[WOL] 等待中... (%d/%d 秒)\n", waited, g_timeout);
        }

        printf("[WOL] 超时，目标未启动\n");
        return FALSE;
    } else {
        printf("[WOL] 目标已在线\n");
        return TRUE;
    }
}

/* 插件入口点 */
BOOL proxy_plugin_entry(proxyPluginsManager* plugins_manager, void* userdata)
{
    proxyPlugin plugin = { 0 };

    plugin.name = "wol-wait";
    plugin.description = "Wake-on-LAN wait plugin";

    // 注册 ServerSessionStarted 钩子
    plugin.ServerSessionStarted = wol_wait_on_session_start;

    // 注册插件
    if (!plugins_manager->RegisterPlugin(plugins_manager, &plugin)) {
        fprintf(stderr, "[WOL] 注册插件失败\n");
        return FALSE;
    }

    printf("[WOL] 插件已注册\n");
    return TRUE;
}
