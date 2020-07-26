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
#include "stm32f2xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usart.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uint8_t usart1_tx_buf[];
extern uint8_t usart2_tx_buf[];
extern uint8_t uart1_irq_sel;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define TOGGLE_LED1() HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin)
#define TOGGLE_LED2() HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin)

#define TRANS_MAX_LENGTH            1200

#if 0
#define EPT(format, ...)	do{\
                      sprintf((char*)usart1_tx_buf, "%s,%d: " format, __func__, __LINE__, ##__VA_ARGS__);\
                      HAL_UART_Transmit(&huart1, usart1_tx_buf, strlen((char*)usart1_tx_buf), 0xFF);\
                    } while(0)
#define PRINT(format, ...)	do{\
                      sprintf((char*)usart1_tx_buf, format, ##__VA_ARGS__);\
                      HAL_UART_Transmit(&huart1, usart1_tx_buf, strlen((char*)usart1_tx_buf), 0xFF);\
                    } while(0)
#else
#define EPT(format, ...)	do{\
                      sprintf((char*)usart1_tx_buf, "%s,%d: " format, __func__, __LINE__, ##__VA_ARGS__);\
                      if (uart1_irq_sel) {\
                        serial_tx(usart1_tx_buf, strlen((char*)usart1_tx_buf));\
                      } else {\
                        HAL_UART_Transmit(&huart1, usart1_tx_buf, strlen((char*)usart1_tx_buf), 0xFF);\
                      }\
                    } while(0)
#define PRINT(format, ...)	do{\
                      sprintf((char*)usart1_tx_buf, format, ##__VA_ARGS__);\
                      if (uart1_irq_sel) {\
                        serial_tx(usart1_tx_buf, strlen((char*)usart1_tx_buf));\
                      } else {\
                        HAL_UART_Transmit(&huart1, usart1_tx_buf, strlen((char*)usart1_tx_buf), 0xFF);\
                      }\
                    } while(0)
#define PRINT2(format, ...)	do{\
                      sprintf((char*)usart1_tx_buf, format, ##__VA_ARGS__);\
                      HAL_UART_Transmit(&huart1, usart1_tx_buf, strlen((char*)usart1_tx_buf), 0xFF);\
                    } while(0)
#endif

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint32_t send_cmd(uint8_t ch, char *arg);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PINOUT2_Pin GPIO_PIN_13
#define PINOUT2_GPIO_Port GPIOC
#define PINOUT_Pin GPIO_PIN_14
#define PINOUT_GPIO_Port GPIOC
#define PINOUT3_Pin GPIO_PIN_5
#define PINOUT3_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_1
#define LED2_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_2
#define LED3_GPIO_Port GPIOB
#define LED4_Pin GPIO_PIN_3
#define LED4_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
