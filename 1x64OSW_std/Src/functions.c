#include "main.h"
#include "common.h"
#include "command.h"
#include "flash_if.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "iwdg.h"
#include "i2c.h"
#include "spi.h"

extern osMessageQueueId_t mid_LogMsg;
extern UpgradeFlashState upgrade_status;

void Throw_Log(uint8_t *buf, uint32_t length)
{
  MsgStruct log_msg;
  
  log_msg.pbuf = pvPortMalloc(length);
  memcpy(log_msg.pbuf, buf, length);
  log_msg.length = length;
  log_msg.type = MSG_TYPE_LOG;

  osMessageQueuePut(mid_LogMsg, &log_msg, 0U, 0U);
}

uint32_t Log_Write(uint32_t addr, uint8_t *pbuf, uint32_t length)
{
  uint32_t w_len = 0, data, i;
  uint8_t remainder = length % 4;

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  while (w_len < length) {
    if (w_len + 4 > length) {
      for (i = 0, data = 0; i < 4; ++i) {
        data |= (i < remainder ? pbuf[w_len + i] : ' ') << i * 8;
      }
    } else {
      memcpy(&data, pbuf + w_len, 4);
    }
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)data);
    addr += 4;
    w_len += 4;
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return w_len;
}

uint32_t Log_Write_byte(uint32_t addr, uint8_t ch, uint32_t length)
{
  uint32_t w_len = 0, data;

  data = ch << 24 | ch << 16 | ch << 8 | ch;
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  while (w_len < length) {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)data);
    addr += 4;
    w_len += 4;
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return w_len;
}

uint32_t Log_Read(uint32_t addr, uint8_t *pbuf, uint32_t length)
{
  memcpy(pbuf, (void*)addr, length);

  return length;
}

osStatus_t Get_Up_Status(UpgradeFlashState *up_status)
{
  osStatus_t status;
  uint8_t buf[EE_UP_FACTORY_CRC32 + 4 - EE_UP_MAIGC];

  status = RTOS_EEPROM_Read(EEPROM_ADDR, EE_UP_MAIGC, buf, sizeof(buf));
  if (status != osOK) {
    return status;
  }
  
  up_status->magic = Buffer_To_BE32(&buf[EE_UP_MAIGC - EE_UP_MAIGC]);
  up_status->run = buf[EE_UP_RUN - EE_UP_MAIGC];
  up_status->flash_empty = buf[EE_UP_FLASH_EMPTY - EE_UP_MAIGC];
  up_status->length = Buffer_To_BE32(&buf[EE_UP_LENGTH - EE_UP_MAIGC]);
  up_status->crc32 = Buffer_To_BE32(&buf[EE_UP_CRC32 - EE_UP_MAIGC]);
  up_status->factory_length = Buffer_To_BE32(&buf[EE_UP_FACTORY_LENGTH - EE_UP_MAIGC]);
  up_status->factory_crc32 = Buffer_To_BE32(&buf[EE_UP_FACTORY_CRC32 - EE_UP_MAIGC]);
  return status;
}

osStatus_t Update_Up_Status(UpgradeFlashState *up_status)
{
  uint8_t buf[EE_UP_FACTORY_CRC32 + 4 - EE_UP_MAIGC];
  
  BE32_To_Buffer(up_status->magic, &buf[EE_UP_MAIGC - EE_UP_MAIGC]);
  buf[EE_UP_RUN - EE_UP_MAIGC] = up_status->run;
  buf[EE_UP_FLASH_EMPTY - EE_UP_MAIGC] = up_status->flash_empty;
  BE32_To_Buffer(up_status->length, &buf[EE_UP_LENGTH - EE_UP_MAIGC]);
  BE32_To_Buffer(up_status->crc32, &buf[EE_UP_CRC32 - EE_UP_MAIGC]);
  BE32_To_Buffer(up_status->factory_length, &buf[EE_UP_FACTORY_LENGTH - EE_UP_MAIGC]);
  BE32_To_Buffer(up_status->factory_crc32, &buf[EE_UP_FACTORY_CRC32 - EE_UP_MAIGC]);

  return RTOS_EEPROM_Write(EEPROM_ADDR, EE_UP_MAIGC, buf, sizeof(buf));
}

osStatus_t Get_Log_Status(LogFileState *log_status)
{
  osStatus_t status;
  uint8_t buf[8];

  status = RTOS_EEPROM_Read(EEPROM_ADDR, EE_LOG_OFFSET, buf, sizeof(buf));
  if (status != osOK) {
    return status;
  }

  log_status->offset = Buffer_To_BE32(&buf[EE_LOG_OFFSET - EE_LOG_OFFSET]);
  log_status->header = Buffer_To_BE32(&buf[EE_LOG_HEADER - EE_LOG_OFFSET]);
  return status;
}

osStatus_t Update_Log_Status(LogFileState *log_status)
{
  uint8_t buf[8];
  
  BE32_To_Buffer(log_status->offset, &buf[EE_LOG_OFFSET - EE_LOG_OFFSET]);
  BE32_To_Buffer(log_status->header, &buf[EE_LOG_HEADER - EE_LOG_OFFSET]);

  return RTOS_EEPROM_Write(EEPROM_ADDR, EE_LOG_OFFSET, buf, sizeof(buf));
}

uint32_t Get_SW_By_IO()
{
  uint32_t val = 0;

  if (HAL_GPIO_ReadPin(SW_D5_GPIO_Port, SW_D5_Pin) == GPIO_PIN_SET)
    val |= 1 << 5;
  if (HAL_GPIO_ReadPin(SW_D4_GPIO_Port, SW_D4_Pin) == GPIO_PIN_SET)
    val |= 1 << 4;
  if (HAL_GPIO_ReadPin(SW_D3_GPIO_Port, SW_D3_Pin) == GPIO_PIN_SET)
    val |= 1 << 3;
  if (HAL_GPIO_ReadPin(SW_D2_GPIO_Port, SW_D2_Pin) == GPIO_PIN_SET)
    val |= 1 << 2;
  if (HAL_GPIO_ReadPin(SW_D1_GPIO_Port, SW_D1_Pin) == GPIO_PIN_SET)
    val |= 1 << 1;
  if (HAL_GPIO_ReadPin(SW_D0_GPIO_Port, SW_D0_Pin) == GPIO_PIN_SET)
    val |= 1 << 0;
  
  return val;
}
