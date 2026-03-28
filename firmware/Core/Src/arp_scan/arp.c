#include "arp.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/etharp.h"
#include "lwip/tcp.h"       // 新增：用於 TCP 連線
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* 私有定義 */
#define FLASK_IP      "192.168.1.113"
#define FLASK_PORT    5001

/* 私有變數 */
extern struct netif gnetif;
__attribute__((aligned(32))) static volatile uint8_t onlineIPs[256];
static struct raw_pcb* ping_pcb = NULL;

static uint8_t whitelist[256]; // 0: 未授權, 1: 白名單
static int scan_count = 0;    // 紀錄目前是第幾次掃描

/* 擴展後的廠商比對結構 */
typedef struct {
    uint8_t oui[3];
    const char* name;
} MAC_Vendor;

const MAC_Vendor vendor_list[] = {
    // --- 原有設備 ---
    {{0x00, 0x80, 0xE1}, "STMicroelectronics"},
    {{0xF0, 0x79, 0x59}, "TP-Link"},
    {{0xBC, 0x07, 0x1D}, "Apple"},
    {{0x7C, 0x10, 0xC9}, "HP / PC"},
    {{0x98, 0x10, 0xE8}, "Mobile Device"},
    {{0xD8, 0x5E, 0xD3}, "ASUS Device"},
    {{0xF0, 0x2F, 0x74}, "TP-Link Device"},

    // --- 擴充：台灣常見網通與電腦品牌 ---
    {{0x0C, 0x9D, 0x92}, "ASUSTek PC/RT"}, // 你剛才詢問的華碩新段位
    {{0x14, 0xDD, 0xA9}, "D-Link Systems"},
    {{0x00, 0x22, 0x15}, "ASUSTek"},
    {{0x00, 0x0C, 0x43}, "Ralink/Tenda"},
    {{0xB0, 0x6E, 0xBF}, "Foxconn (Apple/PC)"}, // 鴻海 (常代工 Apple/Dell)
    {{0x28, 0xD2, 0x44}, "LCFC (Lenovo)"},     // 聯想
    {{0x4C, 0xED, 0xFB}, "AzureWave (Sony/Asus)"}, // 常見 Wi-Fi 模組

    // --- 擴充：手機與行動裝置 ---
    {{0x2C, 0xF0, 0xEE}, "Samsung Electronics"},
    {{0xDC, 0x2B, 0x61}, "Sony Mobile"},
    {{0x40, 0x4E, 0x36}, "HTC Corporation"},
    {{0x50, 0x8A, 0x06}, "Apple (iPhone/Mac)"},

    // --- 擴充：智慧家居與物聯網 (IoT) ---
    {{0x24, 0x62, 0xAB}, "Espressif (ESP32)"}, // 很多 DIY 或平價智慧插座
    {{0xAC, 0x67, 0xB2}, "Espressif (ESP8266)"},
    {{0x28, 0x6C, 0x07}, "Xiaomi (Mi)"},        // 小米
    {{0x50, 0xEC, 0x50}, "Xiaomi (Mi Home)"},   // 小米生態鏈
    {{0x68, 0xFF, 0x7B}, "Google / Nest"},

    // --- 擴充：國際網通大廠 ---
    {{0x00, 0x14, 0xBF}, "Cisco Linksys"},
    {{0xE0, 0x60, 0x66}, "Realtek"},           // 瑞昱 (常見內建網卡)
    {{0x20, 0x4E, 0x7F}, "Netgear"},
    {{0x00, 0x11, 0x32}, "Synology"},          // 群暉 NAS
};
/* 內部函數宣告 */
static struct pbuf* ping_create_packet(void);
static u8_t ping_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr);

/* --- Flask 連動邏輯 --- */

static err_t flask_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t flask_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err == ERR_OK) {
        char *msg = (char *)arg;
        tcp_write(tpcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_sent(tpcb, flask_sent_cb);
    } else {
        tcp_abort(tpcb);
    }
    return ERR_OK;
}

void Telegram_Notify_To_Flask(int ip_last, const char* mac, const char* vendor) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;

    static char httpRequest[256];
    // 配合你的 Flask: /get?ip=xxx&mac=xxx&vendor=xxx
    snprintf(httpRequest, sizeof(httpRequest),
             "GET /get?ip=%d&mac=%s&vendor=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             ip_last, mac, vendor, FLASK_IP);

    ip_addr_t server_ip;
    ipaddr_aton(FLASK_IP, &server_ip);

    LOCK_TCPIP_CORE();
    err_t err = tcp_connect(pcb, &server_ip, FLASK_PORT, flask_connected_cb);
    if (err == ERR_OK) {
        tcp_arg(pcb, httpRequest);
    } else {
        tcp_abort(pcb);
    }
    UNLOCK_TCPIP_CORE();
}

/* --- 原有邏輯 --- */

const char* Get_Vendor_Name(uint8_t *mac) {
    for (int i = 0; i < sizeof(vendor_list)/sizeof(MAC_Vendor); i++) {
        if (memcmp(mac, vendor_list[i].oui, 3) == 0) return vendor_list[i].name;
    }
    static char unknown_vendor[20];
    sprintf(unknown_vendor, "Unknown(%02X%02X%02X)", mac[0], mac[1], mac[2]);
    return unknown_vendor;
}

void ARP_Scan_Init(void) {
    LOCK_TCPIP_CORE();
    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (ping_pcb) {
        raw_bind(ping_pcb, IP_ADDR_ANY);
        raw_recv(ping_pcb, (raw_recv_fn)ping_recv_callback, NULL);
    }
    UNLOCK_TCPIP_CORE();
}

static u8_t ping_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    if (p != NULL) {
        uint8_t last_byte = ip4_addr_get_byte(addr, 3);
        onlineIPs[last_byte] = 1;
        SCB_CleanDCache_by_Addr((uint32_t*)&onlineIPs[last_byte], 1);
        pbuf_free(p);
        return 1;
    }
    return 0;
}

static struct pbuf* ping_create_packet(void) {
    struct pbuf* p = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr) + 32, PBUF_RAM);
    if (!p) return NULL;
    struct icmp_echo_hdr* iecho = (struct icmp_echo_hdr*)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = 0xAFAF;
    iecho->seqno = 0x0001;
    uint8_t *data = (uint8_t *)iecho + sizeof(struct icmp_echo_hdr);
    for (int i = 0; i < 32; i++) data[i] = (uint8_t)i;
    iecho->chksum = inet_chksum(iecho, p->len);
    return p;
}

/* 掃描主任務邏輯 - 自動白名單警報版 */
void StartScanTask(void *argument) {
    extern void MX_LWIP_Init(void);
    MX_LWIP_Init();
    printf("\r\n>>> 系統啟動，初始化白名單中...\r\n");

    memset(whitelist, 0, 256);
    scan_count = 0;

    osDelay(5000);
    ARP_Scan_Init();

    while (1) {
        if (!netif_is_up(&gnetif)) { osDelay(1000); continue; }

        scan_count++;
        printf("\r\n[狀態] 第 %d 次深度掃描中... (網段: 192.168.1.x)\r\n", scan_count);
        memset((void*)onlineIPs, 0, 256);
        SCB_CleanDCache_by_Addr((uint32_t*)onlineIPs, 256);

        ip4_addr_t target_ip;
        uint8_t local_ip_last_byte = ip4_addr_get_byte(&(gnetif.ip_addr), 3);

        // 建立 Ping 預熱 (完全保留原本的延遲邏輯)
        for (int i = 1; i <= 254; i++) {
            if (i == local_ip_last_byte) continue;
            IP4_ADDR(&target_ip, 192, 168, 1, i);
            LOCK_TCPIP_CORE();
            struct pbuf* p = ping_create_packet();
            if (p) { raw_sendto(ping_pcb, p, &target_ip); pbuf_free(p); }
            UNLOCK_TCPIP_CORE();
            osDelay(10);
        }

        osDelay(2500);
        SCB_InvalidateDCache_by_Addr((uint32_t*)onlineIPs, 256);

        printf("========================================================================\r\n");
        printf("  TYPE        IP ADDRESS         MAC ADDRESS          VENDOR HINT       \r\n");
        printf("------------------------------------------------------------------------\r\n");

        int device_count = 0;
        whitelist[local_ip_last_byte] = 1; // 本機必為白名單

        // 1. 本機顯示
        printf("  [LOCAL]     192.168.1.%-3d      %02X:%02X:%02X:%02X:%02X:%02X    %-15s\r\n",
                local_ip_last_byte, gnetif.hwaddr[0], gnetif.hwaddr[1], gnetif.hwaddr[2],
                gnetif.hwaddr[3], gnetif.hwaddr[4], gnetif.hwaddr[5], Get_Vendor_Name(gnetif.hwaddr));
        device_count++;

        // 2. 掃描與白名單邏輯
        for (int i = 1; i <= 254; i++) {
            if (i == local_ip_last_byte) continue;
            IP4_ADDR(&target_ip, 192, 168, 1, i);
            struct eth_addr *mac;
            const ip4_addr_t *ip_tmp;

            LOCK_TCPIP_CORE();
            s8_t res = etharp_find_addr(&gnetif, &target_ip, &mac, &ip_tmp);
            if (res < 0) {
                etharp_request(&gnetif, &target_ip);
                UNLOCK_TCPIP_CORE();
                osDelay(30);
                LOCK_TCPIP_CORE();
                res = etharp_find_addr(&gnetif, &target_ip, &mac, &ip_tmp);
            }
            UNLOCK_TCPIP_CORE();

            if (res >= 0) {
                device_count++;
                const char* vendor_name = Get_Vendor_Name(mac->addr);
                char mac_str[20];
                sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                        mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5]);

                if (scan_count <= 3) {
                    // 初始化階段：建立白名單
                    whitelist[i] = 1;
                    printf("  [ON]        192.168.1.%-3d      %02X:%02X:%02X:%02X:%02X:%02X    %-15s (W)\r\n",
                            i, mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5],
                            vendor_name);
                } else {
                    // 警報階段：檢查是否在白名單
                    if (whitelist[i] == 1) {
                        printf("  [ON]        192.168.1.%-3d      %02X:%02X:%02X:%02X:%02X:%02X    %-15s\r\n",
                                i, mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5],
                                vendor_name);
                    } else {
                        // 發現入侵者！
                        printf("  [INTRUDER!] 192.168.1.%-3d      %02X:%02X:%02X:%02X:%02X:%02X    %-15s\r\n",
                                i, mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5],
                                vendor_name);

                        // 發送到 Flask 觸發 Telegram
                        Telegram_Notify_To_Flask(i, mac_str, vendor_name);
                    }
                }
            }
        }

        if (scan_count == 3) printf("\r\n>>> [系統] 白名單初始化完成。後續掃描將進入監控模式。\r\n");

        printf("------------------------------------------------------------------------\r\n");
        printf("  統計: 發現 %d 個設備 | 狀態: %s\r\n", device_count, (scan_count <= 3 ? "建立中" : "監控中"));
        printf("========================================================================\r\n");

        osDelay(30000);
    }
}
