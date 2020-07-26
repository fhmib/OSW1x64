/**
  ******************************************************************************
  * @file    IAP_Main/Src/menu.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-May-2015
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "usart.h"
#include "ymodem.h"
#include "upgrade.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];
extern FLASH_ProcessTypeDef pFlash;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(uint32_t addr)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString((uint8_t *)"Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = Ymodem_Receive(addr, &size );
  if (result == COM_OK)
  {
    Serial_PutString((uint8_t *)"\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString((uint8_t *)"\n\r Size: ");
    Serial_PutString(number);
    Serial_PutString((uint8_t *)" Bytes\r\n");
    Serial_PutString((uint8_t *)"-------------------\n");
#if defined(RUN_WITH_SRAM)
    update_config_data(addr, size);
#endif
  }
  else if (result == COM_LIMIT)
  {
    Serial_PutString((uint8_t *)"\n\n\rThe image size is higher than the allowed space memory!\n\r");
  }
  else if (result == COM_DATA)
  {
    Serial_PutString((uint8_t *)"\n\n\rVerification failed!\n\r");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString((uint8_t *)"\r\n\nAborted by user.\n\r");
  }
  else
  {
    Serial_PutString((uint8_t *)"\n\rFailed to receive the file!\n\r");
  }
}

/**
  * @brief  Upload a file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(uint32_t addr)
{
  uint8_t status = 0;

  Serial_PutString((uint8_t *)"\n\n\rSelect Receive File\n\r");

  HAL_UART_Receive(&huart3, &status, 1, RX_TIMEOUT);
  if ( status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)addr, (const uint8_t*)"UploadedFlashImage.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
      Serial_PutString((uint8_t *)"\n\rError Occurred while Transmitting File\n\r");
    }
    else
    {
      Serial_PutString((uint8_t *)"\n\rFile uploaded successfully \n\r");
    }
  }
}

void disableSystick()
{
  SysTick->VAL   = 0UL;                                             /* Load the SysTick Counter Value */
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk;                   /* Disable SysTick IRQ and SysTick Timer */
}


void JumpToAddr(uint32_t addr)
{
  // Disable interrupt
  disableSystick();
  HAL_TIM_Base_Stop_IT(&htim3);

  /* Execute the new program */
  JumpAddress = *(__IO uint32_t*) (addr + 4);
  /* Jump to user application */
  JumpToApplication = (pFunction) JumpAddress;
  /* Initialize user application's Stack Pointer */
  __set_MSP(*(__IO uint32_t*) addr);
  JumpToApplication();
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{

  uint8_t key = 0;
  char buf[32];
  FLASH_OBProgramInitTypeDef OptionsBytesStruct;
  
  Serial_PutString((uint8_t *)"\r\n======================================================================");
  Serial_PutString((uint8_t *)"\r\n=              (C) COPYRIGHT 2020 O-Net Technologies                 =");
  Serial_PutString((uint8_t *)"\r\n=                                                                    =");
  Serial_PutString((uint8_t *)"\r\n=       In-Application Programming Application  (Version 1.0.0)      =");
  Serial_PutString((uint8_t *)"\r\n=                                                                    =");
  Serial_PutString((uint8_t *)"\r\n=                                                By Beijing R&D      =");
  Serial_PutString((uint8_t *)"\r\n======================================================================");
  Serial_PutString((uint8_t *)"\r\n\r\n");

  while (1) {
    Serial_PutString((uint8_t *)"\r\n=================== Main Menu ============================\r\n\n");
    Serial_PutString((uint8_t *)"  Download image to the Factory ------------------------ 1\r\n\n");
    Serial_PutString((uint8_t *)"  Download image to the Application 1 ------------------ 2\r\n\n");
    Serial_PutString((uint8_t *)"  Download image to the Application 2 ------------------ 3\r\n\n");
    Serial_PutString((uint8_t *)"  Execute the process ---------------------------------- 4\r\n\n");
    Serial_PutString((uint8_t *)"  Erase upgrade status --------------------------------- 5\r\n\n");
    Serial_PutString((uint8_t *)"  Print upgrade status --------------------------------- 6\r\n\n");
    Serial_PutString((uint8_t *)"  Get Option Byte -------------------------------------- 9\r\n\n");
    Serial_PutString((uint8_t *)"==========================================================\r\n\n");

    /* Clean the input path */
    __HAL_UART_FLUSH_DRREGISTER(&huart3);
  
    /* Receive key */
    HAL_UART_Receive(&huart3, &key, 1, RX_TIMEOUT);
  
    switch (key) {
      case '1' :
        /* Download user application in the Flash */
        SerialDownload(FACTORY_ADDRESS);
        break;
      case '2' :
        /* Download user application in the Flash */
        SerialDownload(APPLICATION_1_ADDRESS);
        break;
      case '3' :
        /* Download user application in the Flash */
        SerialDownload(APPLICATION_2_ADDRESS);
        break;
      case '4' :
        startup_process();
        break;
      case '5' :
        erase_up_status();
        break;
      case '6' :
        print_up_status();
        break;
      case '9' :
        sprintf(buf, "OPTCR = %#X\r\n\n", (uint32_t)FLASH->OPTCR);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "OPTCR = %#X\r\n\n", (uint32_t)FLASH->OPTCR1);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "MEMRMP = %#X\r\n\n", (uint32_t)SYSCFG->MEMRMP);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "ErrorCode = %#X\r\n\n", pFlash.ErrorCode);
        Serial_PutString((uint8_t *)buf);
        FLASH_If_GetOptionByte(&OptionsBytesStruct);
        sprintf(buf, "WRPSector = %#X\r\n", OptionsBytesStruct.WRPSector);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "RDPLevel = %#X\r\n", OptionsBytesStruct.RDPLevel);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "USERConfig = %#X\r\n", OptionsBytesStruct.USERConfig);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "BORLevel = %#X\r\n\n", OptionsBytesStruct.BORLevel);
        Serial_PutString((uint8_t *)buf);
        sprintf(buf, "ErrorCode = %#X\r\n\n", pFlash.ErrorCode);
        Serial_PutString((uint8_t *)buf);
        if (OptionsBytesStruct.WRPSector & (1 << FACTORY_SECTOR)) {
          Serial_PutString((uint8_t *)"Factory memory is not protected\r\n");
        } else {
          Serial_PutString((uint8_t *)"Factory memory is protected\r\n");
        }
        if (OptionsBytesStruct.WRPSector & (1 << APPLICATION_1_SECTOR)) {
          Serial_PutString((uint8_t *)"Application 1 memory is not protected\r\n");
        } else {
          Serial_PutString((uint8_t *)"Application 1 memory is protected\r\n");
        }
        if (OptionsBytesStruct.WRPSector & (1 << APPLICATION_2_SECTOR)) {
          Serial_PutString((uint8_t *)"Application 2 memory is not protected\r\n");
        } else {
          Serial_PutString((uint8_t *)"Application 2 memory is protected\r\n");
        }
        if (OptionsBytesStruct.RDPLevel == 0xAA) {
          Serial_PutString((uint8_t *)"RDP level 0\r\n\n");
        } else if (OptionsBytesStruct.RDPLevel == 0xCC) {
          Serial_PutString((uint8_t *)"RDP level 2\r\n\n");
        } else {
          Serial_PutString((uint8_t *)"RDP level 1\r\n\n");
        }
        break;
      default:
        Serial_PutString((uint8_t *)"Invalid character!\r\n");
        break;
    }
  }
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

