/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "cmd.h"
#include "function.h"
#include "ring_buffer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VER "0.1.5"
#define CMD_LENGTH 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t usart1_tx_buf[256];
uint8_t usart2_tx_buf[TRANS_MAX_LENGTH];
uint32_t counter = 0;

extern usart_tr_stu *usart1_tr;
uint8_t uart1_data;
uint8_t uart1_irq_sel;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint8_t ch;
  uint8_t cmd[CMD_LENGTH];
  uint32_t cmd_len = 0;
  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  rb_init(usart1_tr->rb, USART_RX_BUF_SIZE, usart1_tr->rx_buf);
  rb_init(usart1_tr->wb, USART_TX_BUF_SIZE, usart1_tr->tx_buf);
  uart1_irq_sel = 1;
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
  
  //HAL_UART_Receive_IT(&huart1, &uart1_data, 1);

  PRINT("COM is ready\n");
  PRINT("Firmware Version: " VER "\n");
  PRINT(">");
  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (serial_available() > 0) {
      ch = serial_read();
      
      if (ch == '\r') continue;

      if (ch == '\n') {
        PRINT("\n");
        cmd[cmd_len] = 0;
        process_cmd((char*)cmd);
        PRINT(">");
        cmd_len = 0;
      } else if (ch == '\b') {
        if (cmd_len > 0) {
          HAL_UART_Transmit(&huart1, &ch, 1, 0xFFFFFFFF);
          PRINT(" ");
          HAL_UART_Transmit(&huart1, &ch, 1, 0xFFFFFFFF);
          --cmd_len;
        } 
      } else {
        if (cmd_len < CMD_LENGTH - 1)
          cmd[cmd_len++] = ch;
        HAL_UART_Transmit(&huart1, &ch, 1, 0xFFFFFFFF);
      }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 240;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

console_cmd cmdlist[] = {
  //{"temp", cmd_temp, "Get temperature", "temp", "temp"},
  {"version", cmd_version, "Get version", "version", "version"},
  //{"reset", cmd_reset, "Reset the device", "reset", "reset"},
  //{"log", cmd_log, "Command about log", "log packets | get <packets> <number>", "log packets | get 123 2"},
  {"upgrade", cmd_upgrade, "Command about upgrade", "upgrade init | file | install", "upgrade init | file | install"},
  //{"test", cmd_for_test, "Command for test", "test wd | eeprom write <addr> <value> <count> / read <addr> <count> | spi chan <chan_num>",\
                          "test wd | eeprom write 0x10 0xAA 15 | eeprom read 0x10 64 | eeprom spi chan 2"},
  {"help", cmd_help, "Help information", "help | <cmd> help", "help | upgrade help"},
};
const uint32_t cmd_count = sizeof(cmdlist) / sizeof(cmdlist[0]);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    if (rb_is_full(usart1_tr->rb)) {
        rb_remove(usart1_tr->rb);
    }
    rb_insert(usart1_tr->rb, uart1_data);

    HAL_UART_Receive_IT(&huart1, &uart1_data, 1);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3) {
    TOGGLE_LED1();
  } else if (htim->Instance == TIM2) {
    switch (counter++) {
      case 0:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_RESET);
        break;
      case 1:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
        break;
      case 2:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_RESET);
        break;
      case 3:
        break;
      case 4:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
        break;
      case 5:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_RESET);
        break;
      case 6:
        break;
      case 7:
        break;
      case 8:
        HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
        counter = 0;
        break;
      default:
        break;
    }
    
  }
}
#if 0
uint32_t send_cmd(uint8_t ch, char *arg)
{
  uint32_t length, rcv_len;


  CLEAR_BIT(huart2.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&huart2);

  switch (ch) {
    case 'C':
      HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
      counter = 0;
      HAL_TIM_Base_Start_IT(&htim2);
      return 0;
    case 'D':
      HAL_GPIO_WritePin(PINOUT_GPIO_Port, PINOUT_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PINOUT2_GPIO_Port, PINOUT2_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(PINOUT3_GPIO_Port, PINOUT3_Pin, GPIO_PIN_SET);
      HAL_TIM_Base_Stop_IT(&htim2);
      return 0;
    default:
      EPT("Unknown command\n");
      return 0;
  }

  return 0;
}
#endif
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
