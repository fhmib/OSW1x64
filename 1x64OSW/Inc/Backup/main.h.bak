/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"
#include "common.h"
#include "command.h"
#include "flash_if.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t reset_flag;
extern uint8_t flash_in_use;
extern LogFileState log_file_state;
extern const uint32_t file_flash_addr[];
extern const uint32_t file_flash_end;
extern const uint8_t file_flash_count;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define UART_PROCESS_PRIORITY     osPriorityRealtime6
#define UART_DMA_WAIT_PRIORITY    osPriorityRealtime5
#define CMD_PROCESS_PRIORITY      osPriorityHigh
#define WATCHDOG_PRIORITY         osPriorityISR
#define INTERRUPT_TASK_PRIORITY   osPriorityISR
#define LOG_MESSAGE_PRIORITY      osPriorityLow1

#define WATCH_DOG_DELAY_TIME      500 // ms
#define CMD_WAIT_TEMP_TIME        50    //ms

// Reset Flags
#define BOR_RESET_BIT             (1UL << 0)
#define PIN_RESET_BIT             (1UL << 1)
#define POR_RESET_BIT             (1UL << 2)
#define SFT_RESET_BIT             (1UL << 3)
#define IWDG_RESET_BIT            (1UL << 4)
#define WWDG_RESET_BIT            (1UL << 5)
#define LPWR_RESET_BIT            (1UL << 6)
#define IS_RESETFLAG_SET(bit)     (reset_flag & bit)
#define SET_RESETFLAG(bit)        (reset_flag |= bit)

// Log
#define LOG_QUEUE_LENGTH          6
#define LOG_FIRST_SECTOR          12
#define LOG_PACKET_LENGTH         1024

#define ISR_QUEUE_LENGTH          6


#define EEPROM_ADDR               (0x57 << 1)
#define UPGRADE_MAGIC             0xAA

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void OSW_Init(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define EEPROM_WP_Pin GPIO_PIN_5
#define EEPROM_WP_GPIO_Port GPIOB
#define Strobe_N_Pin GPIO_PIN_13
#define Strobe_N_GPIO_Port GPIOH
#define Strobe_N_EXTI_IRQn EXTI15_10_IRQn
#define HARD_RST_N_Pin GPIO_PIN_0
#define HARD_RST_N_GPIO_Port GPIOI
#define HARD_RST_N_EXTI_IRQn EXTI0_IRQn
#define ROM_WP_Pin GPIO_PIN_2
#define ROM_WP_GPIO_Port GPIOF
#define SPI5_CS_Pin GPIO_PIN_6
#define SPI5_CS_GPIO_Port GPIOF
#define SW_MODE_SEL_Pin GPIO_PIN_12
#define SW_MODE_SEL_GPIO_Port GPIOH
#define SW_D5_Pin GPIO_PIN_11
#define SW_D5_GPIO_Port GPIOH
#define SW_D4_Pin GPIO_PIN_10
#define SW_D4_GPIO_Port GPIOH
#define SW_D0_Pin GPIO_PIN_6
#define SW_D0_GPIO_Port GPIOH
#define SW_D2_Pin GPIO_PIN_8
#define SW_D2_GPIO_Port GPIOH
#define SW_D3_Pin GPIO_PIN_9
#define SW_D3_GPIO_Port GPIOH
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define SW_D1_Pin GPIO_PIN_7
#define SW_D1_GPIO_Port GPIOH
#define SPI4_CS_Pin GPIO_PIN_11
#define SPI4_CS_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
