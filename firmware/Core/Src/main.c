#include "main.h"
#include "cmsis_os.h"
#include "arp_scan/arp.h" // 引入模組化標頭檔
#include <stdio.h>

UART_HandleTypeDef huart3;

/* Task Attributes */
const osThreadAttr_t scanTask_attributes = {
  .name = "scanTask",
  .stack_size = 8192,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MPU_Config(void);

#ifdef __GNUC__
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 10);
    return ch;
}
#endif

int main(void) {
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();

    osKernelInitialize();

    /* 這裡呼叫定義在 arp.c 中的 StartScanTask */
    osThreadNew(StartScanTask, NULL, &scanTask_attributes);

    osKernelStart();
    while (1);
}

/* --- 硬體配置實作 (同前版) --- */
static void MPU_Config(void) {
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    HAL_MPU_Disable();
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = 0x30000000; // D2 SRAM
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void SystemClock_Config(void) {
    /* 建議保留 STM32CubeMX 生成的原始代碼 */
}

static void MX_USART3_UART_Init(void) {
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&huart3);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}

void Error_Handler(void) {
    __disable_irq();
    while (1);
}
