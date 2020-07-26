#include "upgrade.h"
#include "main.h"
#include "common.h"
#include "menu.h"
#include "flash_if.h"
#include <stdio.h>
#include <string.h>

void startup_process()
{
  UpgradeFlashState f_state;
  uint8_t buf[256];

  if (Get_Up_Status(&f_state) != HAL_OK) {
    sprintf((char*)buf, "Get upgrade status failed 1\n");
    Serial_PutString(buf);
  }
  if (f_state.magic != 0xAA) {
    sprintf((char*)buf, "state is uninitialized, boot from factory\n");
    Serial_PutString(buf);
    update_config_data(FACTORY_ADDRESS, 0x10000);
    if (Get_Up_Status(&f_state) != HAL_OK) {
      sprintf((char*)buf, "Get upgrade status failed 2\n");
      Serial_PutString(buf);
    }
  }

  sprintf((char*)buf, "f_state: %#X, %u, %u, %u, %#X, %u, %#X\n", f_state.magic, f_state.run, f_state.flash_empty, f_state.length, f_state.crc32,\
                f_state.factory_length, f_state.factory_crc32);
  Serial_PutString(buf);

  if (f_state.run == 0) {
    sprintf((char*)buf, "Boot from factory...\n");
    Serial_PutString(buf);
    if (boot_process(FACTORY_ADDRESS, f_state.factory_length, f_state.factory_crc32)) {
      sprintf((char*)buf, "Fatal error!!!\n");
      Serial_PutString(buf);
    }
  } else if (f_state.run == 1) {
    sprintf((char*)buf, "Boot from application 1...\n");
    Serial_PutString(buf);
    if (boot_process(APPLICATION_1_ADDRESS, f_state.length, f_state.crc32)) {
      sprintf((char*)buf, "Boot application failed\n");
      Serial_PutString(buf);
      if (boot_process(FACTORY_ADDRESS, f_state.factory_length, f_state.factory_crc32)) {
        sprintf((char*)buf, "Fatal error!!!\n");
        Serial_PutString(buf);
      }
    }
  } else if (f_state.run == 2) {
    sprintf((char*)buf, "Boot from application 2...\n");
    Serial_PutString(buf);
    if (boot_process(APPLICATION_2_ADDRESS, f_state.length, f_state.crc32)) {
      sprintf((char*)buf, "Boot application failed\n");
      Serial_PutString(buf);
      if (boot_process(FACTORY_ADDRESS, f_state.factory_length, f_state.factory_crc32)) {
        sprintf((char*)buf, "Fatal error!!!\n");
        Serial_PutString(buf);
      }
    }
  }
  
  return;
}

uint8_t boot_process(uint32_t addr, uint32_t length, uint32_t crc32)
{
  memcpy((void*)SRAM_TARGET_ADDRESS, (void*)addr, length);
  
  if (crc32 != Cal_CRC32((uint8_t*)addr, length)) {
    return 1;
  } else {
    JumpToAddr(SRAM_TARGET_ADDRESS);
  }

  return 0;
}

void update_config_data(uint32_t addr, uint32_t size)
{
  UpgradeFlashState f_state;
  LogFileState log_state;

  if (addr == FACTORY_ADDRESS) {
    f_state.magic = 0xAA;
    f_state.run = 0;
    f_state.flash_empty = 0;
    f_state.length = 0;
    f_state.crc32 = 0;
    f_state.factory_crc32 = Cal_CRC32((uint8_t*)addr, size);
    f_state.factory_length = size;
    log_state.header = 0x08080000;
    log_state.offset = 0x08080000;
    if (Update_Log_Status(&log_state) != HAL_OK) {
      Serial_PutString((uint8_t *)"Update log status failed in update_config_data\n");
    }
    Serial_PutString((uint8_t *)"Erasing bank 2...\n");
    FLASH_If_Erase_Bank(FLASH_BANK_2);
  } else {
    //memcpy(&f_state, (void*)CONFIG_ADDRESS, sizeof(UpgradeFlashState));
    if (Get_Up_Status(&f_state) != HAL_OK) {
      Serial_PutString((uint8_t *)"Get upgrade status failed in update_config_data\n");
    }
    f_state.run = addr == APPLICATION_1_ADDRESS ? 1 : 2;
    f_state.flash_empty = 0;
    f_state.length = size;
    f_state.crc32 = Cal_CRC32((uint8_t*)addr, size);
  }

  if (Update_Up_Status(&f_state) != HAL_OK) {
    Serial_PutString((uint8_t *)"Update upgrade status failed in update_config_data\n");
  }
  //FLASH_If_Erase(CONFIG_SECTOR);
  //FLASH_If_Write(CONFIG_ADDRESS, (uint32_t*)&f_state, sizeof(UpgradeFlashState) / 4);
}

HAL_StatusTypeDef Get_Up_Status(UpgradeFlashState *up_status)
{
  HAL_StatusTypeDef status;
  uint8_t buf[EE_UP_FACTORY_CRC32 + 4];

  status = I2cEepromRead(EEPROM_ADDR, EE_UP_MAIGC, buf, sizeof(buf));
  if (status != HAL_OK) {
    return status;
  }
  
  up_status->magic = buf[EE_UP_MAIGC];
  up_status->run = buf[EE_UP_RUN];
  up_status->flash_empty = buf[EE_UP_FLASH_EMPTY];
  up_status->length = Buffer_To_BE32(&buf[EE_UP_LENGTH]);
  up_status->crc32 = Buffer_To_BE32(&buf[EE_UP_CRC32]);
  up_status->factory_length = Buffer_To_BE32(&buf[EE_UP_FACTORY_LENGTH]);
  up_status->factory_crc32 = Buffer_To_BE32(&buf[EE_UP_FACTORY_CRC32]);
  return status;
}

HAL_StatusTypeDef Update_Up_Status(UpgradeFlashState *up_status)
{
  uint8_t buf[EE_UP_FACTORY_CRC32 + 4];
  
  buf[EE_UP_MAIGC] = up_status->magic;
  buf[EE_UP_RUN] = up_status->run;
  buf[EE_UP_FLASH_EMPTY] = up_status->flash_empty;
  BE32_To_Buffer(up_status->length, &buf[EE_UP_LENGTH]);
  BE32_To_Buffer(up_status->crc32, &buf[EE_UP_CRC32]);
  BE32_To_Buffer(up_status->factory_length, &buf[EE_UP_FACTORY_LENGTH]);
  BE32_To_Buffer(up_status->factory_crc32, &buf[EE_UP_FACTORY_CRC32]);

  return I2cEepromWrite(EEPROM_ADDR, EE_UP_MAIGC, buf, sizeof(buf));
}

void erase_up_status(void)
{
  uint8_t ebuf[EE_UP_FACTORY_CRC32 + 4];
  uint8_t buf[256];
  HAL_StatusTypeDef status;
  
  memset(ebuf, 0xFF, EE_UP_FACTORY_CRC32 + 4);
  if ((status = I2cEepromWrite(EEPROM_ADDR, EE_UP_MAIGC, ebuf, sizeof(ebuf))) != HAL_OK) {
    sprintf((char*)buf, "erase failed, status = %u\n", status);
    Serial_PutString(buf);
  }
}

void print_up_status(void)
{
  UpgradeFlashState up_status;
  uint8_t buf[256];
  
  Get_Up_Status(&up_status);
  sprintf((char*)buf, "up_status: %#X, %u, %u, %u, %#X, %u, %#X\n", up_status.magic, up_status.run, up_status.flash_empty, up_status.length, up_status.crc32,\
                up_status.factory_length, up_status.factory_crc32);
  Serial_PutString(buf);
}

HAL_StatusTypeDef Get_Log_Status(LogFileState *log_status)
{
  HAL_StatusTypeDef status;
  uint8_t buf[8];

  status = I2cEepromRead(EEPROM_ADDR, EE_LOG_OFFSET, buf, sizeof(buf));
  if (status != HAL_OK) {
    return status;
  }

  log_status->offset = Buffer_To_BE32(&buf[EE_LOG_OFFSET - EE_LOG_OFFSET]);
  log_status->header = Buffer_To_BE32(&buf[EE_LOG_HEADER - EE_LOG_OFFSET]);
  return status;
}

HAL_StatusTypeDef Update_Log_Status(LogFileState *log_status)
{
  uint8_t buf[8];
  
  BE32_To_Buffer(log_status->offset, &buf[EE_LOG_OFFSET - EE_LOG_OFFSET]);
  BE32_To_Buffer(log_status->header, &buf[EE_LOG_HEADER - EE_LOG_OFFSET]);

  return I2cEepromWrite(EEPROM_ADDR, EE_LOG_OFFSET, buf, sizeof(buf));
}
