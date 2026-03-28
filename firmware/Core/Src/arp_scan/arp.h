#ifndef SRC_ARP_SCAN_ARP_H_
#define SRC_ARP_SCAN_ARP_H_

#include "lwip/ip_addr.h"

/* 啟動掃描任務 */
void ARP_Scan_Init(void);

/* 對外的掃描任務入口，由 FreeRTOS 呼叫 */
void StartScanTask(void *argument);

#endif /* SRC_ARP_SCAN_ARP_H_ */
