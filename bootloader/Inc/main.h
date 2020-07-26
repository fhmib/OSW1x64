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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#undef USE_FLASH_FOR_FW_IMG
#undef USE_SRAM_FOR_FW_IMG
#define RUN_WITH_SRAM

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t reset_flag;
#ifdef USE_SRAM_FOR_FW_IMG
extern uint8_t fw_buffer[];
#endif

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define FW_MAX_LENGTH             0x10000

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

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void Boot_Init(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_6
#define LED1_GPIO_Port GPIOF
#define LED2_Pin GPIO_PIN_7
#define LED2_GPIO_Port GPIOF
#define LED3_Pin GPIO_PIN_8
#define LED3_GPIO_Port GPIOF
#define LED4_Pin GPIO_PIN_9
#define LED4_GPIO_Port GPIOF
#define BOOT_Pin GPIO_PIN_12
#define BOOT_GPIO_Port GPIOH
#define EEPROM_WP_Pin GPIO_PIN_5
#define EEPROM_WP_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
