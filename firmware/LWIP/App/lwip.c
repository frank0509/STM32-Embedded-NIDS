#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "ethernetif.h"
#include <string.h>

/* 全域變數 */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];

#define INTERFACE_THREAD_STACK_SIZE ( 1024 )
osThreadAttr_t attributes;

void MX_LWIP_Init(void)
{
  /* IP 地址設定 */
  IP_ADDRESS[0] = 192; IP_ADDRESS[1] = 168; IP_ADDRESS[2] = 1; IP_ADDRESS[3] = 114;
  NETMASK_ADDRESS[0] = 255; NETMASK_ADDRESS[1] = 255; NETMASK_ADDRESS[2] = 255; NETMASK_ADDRESS[3] = 0;
  GATEWAY_ADDRESS[0] = 192; GATEWAY_ADDRESS[1] = 168; GATEWAY_ADDRESS[2] = 1; GATEWAY_ADDRESS[3] = 1;

  /* 1. 初始化 Stack */
  tcpip_init( NULL, NULL );

  /* 2. 核心鎖定 */
  LOCK_TCPIP_CORE();

  /* 3. 配置網卡 */
  IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
  IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
  IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);

  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  netif_set_default(&gnetif);
  netif_set_up(&gnetif);

  /* 💡 這裡已經徹底移除報錯的 Callback 設定 */

  /* 4. 啟動乙太網執行緒 */
  memset(&attributes, 0x0, sizeof(osThreadAttr_t));
  attributes.name = "EthLink";
  attributes.stack_size = INTERFACE_THREAD_STACK_SIZE;
  attributes.priority = osPriorityBelowNormal;
  osThreadNew(ethernet_link_thread, &gnetif, &attributes);

  /* 5. 釋放鎖 */
  UNLOCK_TCPIP_CORE();
}
