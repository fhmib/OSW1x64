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
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "flash_if.h"
#include "common.h"
#include "menu.h"
#include "upgrade.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define WAIT_TIME 5

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
const char start_msg[] = "\nWaiting to enter application, Press any key to interrput";
const char VERSION[] = "\n******* BOOTLOADER VERSION 0.0.1 *******\n";
uint8_t reset_flag = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  //int i = 0;
  //char ch;
#if defined(USE_SRAM_FOR_FW_IMG) || defined(USE_FLASH_FOR_FW_IMG)
  uint8_t start;
#endif
  //HAL_StatusTypeDef status;
//  GPIO_PinState boot_state;

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  FLASH_If_Init();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  Boot_Init();
  HAL_UART_Transmit(&huart3, (uint8_t*)VERSION, strlen(VERSION), TX_TIMEOUT);
  //HAL_UART_Transmit(&huart3, (uint8_t*)start_msg, strlen(start_msg), TX_TIMEOUT);
  startup_process();
#if 0
  boot_state = HAL_GPIO_ReadPin(BOOT_GPIO_Port, BOOT_Pin);
  if (boot_state == GPIO_PIN_RESET) {
    startup_process();
  }
#endif
  CLEAR_BIT(huart3.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&huart3);
  HAL_TIM_Base_Start_IT(&htim3);
  Main_Menu();
  
  /* USER CODE END 2 */
 
 

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
#if 0
  while (++i <= WAIT_TIME) {
    status = HAL_UART_Receive(&huart3, (uint8_t*)&ch, 1, 1000);
    if (status == HAL_OK) {
      HAL_UART_Transmit(&huart3, (uint8_t*)"\n", 1, TX_TIMEOUT);
      Main_Menu();
      break;
    }
    HAL_UART_Transmit(&huart3, (uint8_t*)".", 1, TX_TIMEOUT);
  }

  HAL_UART_Transmit(&huart3, (uint8_t*)"\n", 1, TX_TIMEOUT);
#endif

  while (1)
  {
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

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void Boot_Init(void)
{
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET){
    SET_RESETFLAG(BOR_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET){
    SET_RESETFLAG(PIN_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET){
    SET_RESETFLAG(POR_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET){
    SET_RESETFLAG(SFT_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET){
    SET_RESETFLAG(IWDG_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET){
    SET_RESETFLAG(WWDG_RESET_BIT);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET){
    SET_RESETFLAG(LPWR_RESET_BIT);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == htim3.Instance) {
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
  }
}

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
