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
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t reset_flag = 0;
uint8_t flash_in_use;
UpgradeFlashState upgrade_status;
LogFileState log_file_state;
const uint32_t file_flash_addr[] =  {  0x08100000, 0x08104000, 0x08108000 };
const uint32_t file_flash_end = 0x0810BFFF;
const uint8_t file_flash_count = sizeof(file_flash_addr) / sizeof (file_flash_addr[0]);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern UpgradeStruct up_state;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_IWDG_Init();
  MX_SPI1_Init();
  MX_SPI4_Init();
  MX_SPI5_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */
  /* Init scheduler */
  osKernelInitialize();
 
  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init(); 
 
  /* Start scheduler */
  osKernelStart();
 
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 20;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
extern RespondStu resp_buf;
void OSW_Init(void)
{
  osStatus_t status;
  uint32_t i;
  uint8_t buf[TAG_MAX_SPACE];
  char *p = (char*)resp_buf.buf;

  EPT("Firmware version: %s\n", fw_version);

  if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST) != RESET){
    SET_RESETFLAG(BOR_RESET_BIT);
    EPT("BOR Reset is set\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST) != RESET){
    SET_RESETFLAG(PIN_RESET_BIT);
    EPT("PIN Reset is set\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) != RESET){
    SET_RESETFLAG(POR_RESET_BIT);
    EPT("POR/PDR Reset is set\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) != RESET){
    SET_RESETFLAG(SFT_RESET_BIT);
    EPT("Software Reset is set\n");
    memset(p, 0, 116);
    p += 4;
    p += 16;
    status = RTOS_EEPROM_Read(EEPROM_ADDR, EE_TAG_PN, buf, 8);
    memcpy(p, buf, 8);
    p += 10;
    status |= RTOS_EEPROM_Read(EEPROM_ADDR, EE_TAG_DATE, buf, 8);
    memcpy(p, buf, 8);
    p += 10;
    strcpy((char*)p, fw_version);
    p += 37;
    status |= RTOS_EEPROM_Read(EEPROM_ADDR, EE_TAG_ASN, buf, 12);
    memcpy(p, buf, 12);
    p += 20;
    status |= RTOS_EEPROM_Read(EEPROM_ADDR, EE_TAG_FSN, buf, 23);
    memcpy(p, buf, 23);
    p += 23;
    if (status != osOK) {
      EPT("EEPROM ERROR! status = %#X", status);
    }

    Uart_Respond(CMD_SOFTRESET, RESPOND_SUCCESS, resp_buf.buf, 120);
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET){
    SET_RESETFLAG(IWDG_RESET_BIT);
    EPT("Independent Watchdog Reset is set\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST) != RESET){
    SET_RESETFLAG(WWDG_RESET_BIT);
    EPT("Window Watchdog Reset is set\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST) != RESET){
    SET_RESETFLAG(LPWR_RESET_BIT);
    EPT("Low-Power Reset is set\n");
  }
/*
  if (IS_RESETFLAG_SET(BOR_RESET_BIT)) {
  }
*/
  __HAL_RCC_CLEAR_RESET_FLAGS();

  // upgrade
  up_state.run = RUN_MODE_APPLICATION;
  up_state.is_erasing = 0;
  flash_in_use = 0;
  
  FLASH_If_Init();
  status = Get_Up_Status(&upgrade_status);
  if (status) {
    EPT("Get upgrade status failed, status = %u\n", status);
  }
  EPT("upgrade_status: %#X, %u, %u, %u, %#X, %u, %#X\n", upgrade_status.magic, upgrade_status.run, upgrade_status.flash_empty, upgrade_status.length, upgrade_status.crc32,\
                upgrade_status.factory_length, upgrade_status.factory_crc32);

  if (upgrade_status.magic != UPGRADE_MAGIC) {
    EPT("Verify f_state.magic failed\n");
  }
  switch (upgrade_status.run) {
    case 0:
      EPT("Boot from factory\n");
      up_state.upgrade_addr = APPLICATION_1_ADDRESS;
      up_state.upgrade_sector = APPLICATION_1_SECTOR;
      break;
    case 1:
      EPT("Boot from Application 1\n");
      up_state.upgrade_addr = APPLICATION_2_ADDRESS;
      up_state.upgrade_sector = APPLICATION_2_SECTOR;
      break;
    case 2:
      EPT("Boot from Application 2\n");
      up_state.upgrade_addr = APPLICATION_1_ADDRESS;
      up_state.upgrade_sector = APPLICATION_1_SECTOR;
      break;
    default:
      EPT("f_state.run invalid\n");
      up_state.upgrade_addr = RESERVE_ADDRESS;
      up_state.upgrade_sector = RESERVE_SECTOR;
      break;
  }
  EPT("FLASH->OPTCR = %#X\n", FLASH->OPTCR);
  if (!upgrade_status.flash_empty) {
    flash_in_use = 1;
    up_state.is_erasing = 1;
    // erase flash
    EPT("flash is not empty\n");
    if (up_state.upgrade_addr != RESERVE_ADDRESS)
      FLASH_If_Erase_IT(up_state.upgrade_sector);
    EPT("erase sector...\n");
  } else {
    EPT("flash is empty\n");
  }
  
  // log
  Get_Log_Status(&log_file_state);
  EPT("log_file_state: %#X, %#X\n", log_file_state.offset, log_file_state.header);
  for (i = 0; i < file_flash_count - 1; ++i) {
    if (log_file_state.offset < file_flash_addr[i + 1]) {
      break;
    }
  }
  log_file_state.cur_sector = LOG_FIRST_SECTOR + i;

  Throw_Log((uint8_t*)"OSW1x64 program\n", 16);

}

extern FLASH_ProcessTypeDef pFlash;
extern osMutexId_t i2cMutex;
extern osMessageQueueId_t mid_ISR;
extern osSemaphoreId_t logEraseSemaphore;
void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue)
{
  MsgStruct isr_msg;
  if (0xFFFFFFFFU == ReturnValue) {
    EPT("Erase flash operation completely\n");
    HAL_FLASH_Lock();
    flash_in_use = 0;
    if (up_state.is_erasing == 0) {
      // send signal to logTask
      osSemaphoreRelease(logEraseSemaphore);
    } else {
      upgrade_status.flash_empty = 1;
      up_state.is_erasing = 0;
      isr_msg.type = MSG_TYPE_FLASH_ISR;
      osMessageQueuePut(mid_ISR, &isr_msg, 0U, 0U);
    }
  }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t sw_num;

  if (GPIO_Pin == GPIO_PIN_13) {
    if (HAL_GPIO_ReadPin(SW_MODE_SEL_GPIO_Port, SW_MODE_SEL_Pin) == GPIO_PIN_SET) {
      EPT("Switch strobe signal come but switch mode is set\n");
    } else {
      sw_num = Get_SW_By_IO();
      EPT("Switch number is %u\n", sw_num);
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM11 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM11) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
