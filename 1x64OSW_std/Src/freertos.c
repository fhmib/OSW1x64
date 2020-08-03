/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "command.h"
#include "iwdg.h"
#include "flash_if.h"
#include "functions.h"

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
/* USER CODE BEGIN Variables */
osSemaphoreId_t uartProcessSemaphore;
osSemaphoreId_t cmdProcessSemaphore;
osSemaphoreId_t logEraseSemaphore;                  // semaphore id

osMessageQueueId_t mid_LogMsg;
osMessageQueueId_t mid_ISR;                // message queue id

osMutexId_t i2c1Mutex;
osMutexId_t i2c2Mutex;
osMutexId_t spi1Mutex;
osMutexId_t spi4Mutex;
osMutexId_t spi5Mutex;
const osMutexAttr_t Thread_Mutex_attr = {
  "Mutex",                              // human readable mutex name
  osMutexPrioInherit,                       // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
};

osThreadId_t cmdProcessTaskHandle;
const osThreadAttr_t cmdProcessTask_attributes = {
  .name = "cmdProcessTask",
  .priority = (osPriority_t) CMD_PROCESS_PRIORITY,
  .stack_size = 2048
};

osThreadId_t uartProcessTaskHandle;
const osThreadAttr_t uartProcessTask_attributes = {
  .name = "uartProcessTask",
  .priority = (osPriority_t) UART_PROCESS_PRIORITY,
  .stack_size = 512
};

osThreadId_t watchdogTaskHandle;
const osThreadAttr_t watchdogTask_attributes = {
  .name = "watchdogTask",
  .priority = (osPriority_t) WATCHDOG_PRIORITY,
  .stack_size = 512
};

osThreadId_t isrTaskHandle;
const osThreadAttr_t isrTask_attributes = {
  .name = "isrTask",
  .priority = (osPriority_t) INTERRUPT_TASK_PRIORITY,
  .stack_size = 512
};

osThreadId_t logTaskHandle;
const osThreadAttr_t logTask_attributes = {
  .name = "logTask",
  .priority = (osPriority_t) LOG_MESSAGE_PRIORITY,
  .stack_size = 512
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void uartProcessTask(void *argument);
void cmdProcessTask(void *argument);
void watchdogTask(void *argument);
void isrTask(void *argument);
void logTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  uartProcessSemaphore = osSemaphoreNew(1U, 0U, NULL);
  if (uartProcessSemaphore == NULL) {
  }

  cmdProcessSemaphore = osSemaphoreNew(1U, 0U, NULL);
  if (cmdProcessSemaphore == NULL) {
  }

  logEraseSemaphore = osSemaphoreNew(1U, 0U, NULL);
  if (logEraseSemaphore == NULL) {
  }

  mid_LogMsg = osMessageQueueNew(LOG_QUEUE_LENGTH, sizeof(MsgStruct), NULL);
  if (mid_LogMsg == NULL) {
  }

  mid_ISR = osMessageQueueNew(ISR_QUEUE_LENGTH, sizeof(MsgStruct), NULL);
  if (mid_ISR == NULL) {
  }

  i2c1Mutex = osMutexNew(&Thread_Mutex_attr);
  if (i2c1Mutex == NULL) {
  }

  i2c2Mutex = osMutexNew(&Thread_Mutex_attr);
  if (i2c2Mutex == NULL) {
  }

  spi1Mutex = osMutexNew(&Thread_Mutex_attr);
  if (spi1Mutex == NULL) {
  }

  spi4Mutex = osMutexNew(&Thread_Mutex_attr);
  if (spi4Mutex == NULL) {
  }

  spi5Mutex = osMutexNew(&Thread_Mutex_attr);
  if (spi5Mutex == NULL) {
  }

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  uartProcessTaskHandle = osThreadNew(uartProcessTask, NULL, &uartProcessTask_attributes);
  cmdProcessTaskHandle = osThreadNew(cmdProcessTask, NULL, &cmdProcessTask_attributes);
  watchdogTaskHandle = osThreadNew(watchdogTask, NULL, &watchdogTask_attributes);
  isrTaskHandle = osThreadNew(isrTask, NULL, &isrTask_attributes);
  logTaskHandle = osThreadNew(logTask, NULL, &logTask_attributes);

  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
extern TransStu trans_buf;
struct UartTaskSync{
  uint8_t ProcessOnGoing;
  uint8_t ProcessTimeout;
} uart_sync;

void uartProcessTask(void *argument)
{
  uint32_t msg_length;
  uint32_t msg_id;
  uint32_t chk;
  uint32_t wait_time;
  uint32_t *p32;
  osStatus_t status;

  CLEAR_BIT(huart1.Instance->SR, USART_SR_RXNE);
  __HAL_UART_FLUSH_DRREGISTER(&huart1);
  HAL_UART_Receive_IT(&huart1, trans_buf.buf, 1);

  uart_sync.ProcessTimeout = 0;
  uart_sync.ProcessOnGoing = 0;
  trans_buf.stage = TRANS_WAIT_START;

  for(;;)
  {
    if (trans_buf.stage == TRANS_WAIT_START) {
      wait_time = osWaitForever;
    } else if (trans_buf.stage == TRANS_WAIT_HEADER) {
      wait_time = pdMS_TO_TICKS(TRANS_WAIT_HEAD_TIME);
    } else if (trans_buf.stage == TRANS_RECV_DATA) {
      wait_time = pdMS_TO_TICKS(TRANS_WAIT_TIME);
    }
    status = osSemaphoreAcquire(uartProcessSemaphore, wait_time);
    if (status != osOK) {
      HAL_UART_DMAStop(&huart1);
      msg_id = 0;
      EPT("DMA timeout, stage = %u\n", trans_buf.stage);
      Uart_Respond(msg_id, RESPOND_LENGTH_ERR, NULL, 0);
      trans_buf.stage = TRANS_WAIT_START;
      HAL_UART_Receive_IT(&huart1, trans_buf.buf, 1);
      continue;
    }

    if (trans_buf.stage == TRANS_WAIT_START) {
      // Received start byte
      if (trans_buf.buf[0] != TRANS_START_BYTE) {
        // Invalid data
        EPT("Invalid character: %#X\n", trans_buf.buf[0]);
        HAL_UART_Receive_IT(&huart1, trans_buf.buf, 1);
      } else {
        // Prepare to receive message header
        trans_buf.stage = TRANS_WAIT_HEADER;
        HAL_UART_Receive_DMA(&huart1, &trans_buf.buf[CMD_SEQ_MSG_ID], 8);
      }
    } else if (trans_buf.stage == TRANS_WAIT_HEADER) {
      // Received message header
      p32 = (uint32_t*)&trans_buf.buf[CMD_SEQ_MSG_ID];
      msg_id = switch_endian(*p32);
      p32 = (uint32_t*)&trans_buf.buf[CMD_SEQ_MSG_LENGTH];
      msg_length = switch_endian(*p32);
      if (msg_length > TRANS_MAX_LENGTH - 1 || msg_length < 4 * 5) {
        // Length invalid
        PRINT_HEX("buf", trans_buf.buf, 9);
        msg_id = 0;
        EPT("Invalid length %#X\n", msg_length);
        Uart_Respond(msg_id, RESPOND_LENGTH_ERR, NULL, 0);
        trans_buf.stage = TRANS_WAIT_START;
        HAL_UART_Receive_IT(&huart1, trans_buf.buf, 1);
      } else {
        trans_buf.stage = TRANS_RECV_DATA;
        HAL_UART_Receive_DMA(&huart1, &trans_buf.buf[CMD_SEQ_MSG_DATA], msg_length - 8);
        EPT("Message Length: %u, ID: %u\n", msg_length, msg_id);
        trans_buf.length = msg_length + 1;
      }
    } else if (trans_buf.stage == TRANS_RECV_DATA){
      // Received message data

      // TODO: Check CRC
      chk = Cal_CRC32((uint8_t*)&trans_buf.buf[CMD_SEQ_MSG_ID], trans_buf.length - 4 - 1);
      p32 = (uint32_t*)&trans_buf.buf[trans_buf.length - 4];
      if (chk ^ switch_endian(*p32)) {
        // Incorrect
        EPT("CRC32 verification failed\n");
        EPT("msg_length = %#X, trans_buf.length = %#X, chk = %#X, rcv_chk = %#X\n", msg_length, trans_buf.length, chk, switch_endian((uint32_t)trans_buf.buf[trans_buf.length - 4]));
        PRINT_HEX("buf", trans_buf.buf, trans_buf.length);
        Uart_Respond(msg_id, RESPOND_CHECKSUM_ERR, NULL, 0);
      } else {
        if (!uart_sync.ProcessOnGoing) {
          // Process Command and Respond
          xTaskNotifyGive(cmdProcessTaskHandle);
          status = osSemaphoreAcquire(cmdProcessSemaphore, pdMS_TO_TICKS(COMMAND_PROCESS_TIME));
          if (status == osErrorTimeout) {
            // TODO: Process command timeout
            EPT("Process command timeout\n");
            uart_sync.ProcessTimeout = 1;
            Uart_Respond(msg_id, RESPOND_FAILURE, NULL, 0);
          } else if (status != osOK) {
            EPT("Error, status=%#X\n", status);
          }
        } else {
            EPT("Detected process ongoing\n");
            Uart_Respond(msg_id, RESPOND_FAILURE, NULL, 0);
        }
      }

      msg_id = 0;
      trans_buf.stage = TRANS_WAIT_START;
      HAL_UART_Receive_IT(&huart1, trans_buf.buf, 1);
    }
  }
}

extern RespondStu resp_buf;

void cmdProcessTask(void *argument)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (trans_buf.stage != TRANS_RECV_DATA) {
      EPT("Stage incorrect\n");
      continue;
    }

    uart_sync.ProcessOnGoing = 1;
    Cmd_Process();

    if (uart_sync.ProcessTimeout == 0) {
      osSemaphoreRelease(cmdProcessSemaphore);
      Uart_Respond(resp_buf.cmd, resp_buf.status, resp_buf.buf, resp_buf.length);
    } else {
      EPT("Detected process timeout\n");
      uart_sync.ProcessTimeout = 0;
    }

    uart_sync.ProcessOnGoing = 0;
  }
}

void watchdogTask(void *argument)
{
  uint32_t delay = pdMS_TO_TICKS(WATCH_DOG_DELAY_TIME);

  HAL_IWDG_Refresh(&hiwdg);
  for(;;)
  {
    osDelay(delay);
    HAL_IWDG_Refresh(&hiwdg);
  }
}

extern UpgradeFlashState upgrade_status;
void isrTask(void *argument)
{
  osStatus_t status;
  MsgStruct msg;

  OSW_Init();

  for (;;)
  {
    status = osMessageQueueGet(mid_ISR, &msg, 0U, osWaitForever);
    if (status != osOK)
      continue;

    if (msg.type == MSG_TYPE_FLASH_ISR) {
      EPT("Received MSG_TYPE_FLASH_ISR, flash_empty = %u\n", upgrade_status.flash_empty);
      status = Update_Up_Status(&upgrade_status);
      if (status != osOK) {
        EPT("Update upgrade status to eeprom failed\n");
      }
    }
  }
}

extern UpgradeStruct up_state;
void logTask(void *argument)
{
  MsgStruct log_msg;
  osStatus_t status;
  uint32_t write_length;

  uint32_t overflow_addr;
  uint32_t next_sector_addr;
  uint8_t next_sector;
  uint8_t remainder;

  if (log_file_state.cur_sector == LOG_FIRST_SECTOR + file_flash_count - 1) {
    overflow_addr = file_flash_end + 1;
    next_sector_addr = file_flash_addr[0];
    next_sector = LOG_FIRST_SECTOR;
  } else {
    next_sector_addr = overflow_addr = file_flash_addr[log_file_state.cur_sector - LOG_FIRST_SECTOR + 1];
    next_sector = log_file_state.cur_sector + 1;
  }

  for (;;)
  {
    status = osMessageQueueGet(mid_LogMsg, &log_msg, 0U, osWaitForever);
    if (status != osOK)
      continue;
    
    if (log_file_state.offset >= file_flash_end + 1 || log_file_state.offset < file_flash_addr[0])
      continue;

    // The final amount of data written to flash must be an integer multiple of 4
    remainder = log_msg.length % 4;
    if (remainder) remainder = 4 - remainder;
    if (log_file_state.offset + log_msg.length + remainder >= overflow_addr) {
      // 1. fill the rest space with ' '
      Log_Write_byte(log_file_state.offset, ' ', overflow_addr - log_file_state.offset);
      
      // 2. erase flash
      while (flash_in_use) {
        osDelay(pdMS_TO_TICKS(200));
      }
      flash_in_use = 1;
      osSemaphoreAcquire(logEraseSemaphore, 0);
      FLASH_If_Erase_IT(next_sector);

      // 3. waiting for completion
      osSemaphoreAcquire(logEraseSemaphore, osWaitForever);

      // 4. update log_file_state, overflow_addr and next_***s
      log_file_state.offset = next_sector_addr;
      log_file_state.cur_sector = next_sector;
      if (log_file_state.cur_sector == LOG_FIRST_SECTOR + file_flash_count - 1) {
        overflow_addr = file_flash_end + 1;
        next_sector_addr = file_flash_addr[0];
        next_sector = LOG_FIRST_SECTOR;
      } else {
        next_sector_addr = overflow_addr = file_flash_addr[log_file_state.cur_sector - LOG_FIRST_SECTOR + 1];
        next_sector = log_file_state.cur_sector + 1;
      }
      if (log_file_state.offset == log_file_state.header) {
        log_file_state.header = next_sector_addr;
      }
    }

    EPT("Writing log to flash address %#X, header = %#X\n", log_file_state.offset, log_file_state.header);
    while (flash_in_use) {
      EPT("Flash in using...\n");
      osDelay(pdMS_TO_TICKS(200));
    }
    write_length = Log_Write(log_file_state.offset, log_msg.pbuf, log_msg.length);
    log_file_state.offset += write_length;
    
    vPortFree(log_msg.pbuf);
    
    if (osMessageQueueGetCount(mid_LogMsg) == 0) {
      // update log_file_state to eeprom/flash
      Update_Log_Status(&log_file_state);
    }
  }
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
