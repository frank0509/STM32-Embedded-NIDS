#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#include "main.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define WITH_RTOS 1
#define MEM_ALIGNMENT 4
#define MEM_SIZE (128*1024)
#define LWIP_RAM_HEAP_POINTER 0x30020000

/* ❗ 修正：加大 ARP 表與相關資源池，防止 MAC Unknown */
#define LWIP_ARP               1
#define ARP_TABLE_SIZE         100    // 加大以容納所有掃描到的設備
#define MEMP_NUM_ARP_QUEUE     30     // 增加 ARP 請求佇列
#define ETHARP_SUPPORT_STATIC_ENTRIES 1

/* ❗ 允許 Ping 自己 */
#define LWIP_NETIF_LOOPBACK    1
#define LWIP_HAVE_LOOPIF       1

/* ❗ 記憶體 Pool 優化 */
#define PBUF_POOL_SIZE         40
#define MEMP_NUM_PBUF          20

#define CHECKSUM_BY_HARDWARE   0
#define CHECKSUM_GEN_IP        1
#define CHECKSUM_GEN_ICMP      1
#define CHECKSUM_CHECK_IP      1
#define CHECKSUM_CHECK_ICMP    1

#define LWIP_RAW               1
#define LWIP_ICMP              1
#define LWIP_IPV4              1

#define LWIP_TCPIP_CORE_LOCKING 1
void sys_lock_tcpip_core(void);
void sys_unlock_tcpip_core(void);
#define LOCK_TCPIP_CORE() sys_lock_tcpip_core()
#define UNLOCK_TCPIP_CORE() sys_unlock_tcpip_core()

#define TCPIP_THREAD_STACKSIZE 2048
#define TCPIP_THREAD_PRIO      24
#define TCPIP_MBOX_SIZE        32

#ifdef __cplusplus
}
#endif
#endif
