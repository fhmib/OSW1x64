#ifndef __UPGRADE_H
#define __UPGRADE_H

#include "main.h"

#define EEPROM_ADDR                   (0x57 << 1)

typedef enum {
  UPGRADE_UNUSABLE       = 0x00,
  UPGRADE_RESET          = 0x01,
  UPGRADE_FAILURE        = 0x02,
  UPGRADE_SUCCESS        = 0x03,
} UpgradeState;


typedef struct {
  uint8_t magic;
  uint8_t run;
  uint8_t flash_empty;
  uint32_t length;
  uint32_t crc32;
  uint32_t factory_length;
  uint32_t factory_crc32;
} UpgradeFlashState;

typedef enum {
  START_ERROR,
  START_FROM_FACTORY,
  START_FROM_APPLICATION,
} UpgradeReturn;

typedef enum {
  EE_UP_MAIGC             = 0x00, // 0x0 ~ 0x0
  EE_UP_RUN               = 0x01, // 0x1 ~ 0x1 Which fw is running
  EE_UP_FLASH_EMPTY       = 0x02, // 0x2 ~ 0x2 If flash is empty
  EE_UP_LENGTH            = 0x03, // 0x3 ~ 0x6
  EE_UP_CRC32             = 0x07, // 0x7 ~ 0xA
  EE_UP_FACTORY_LENGTH    = 0x0B, // 0xB ~ 0xE
  EE_UP_FACTORY_CRC32     = 0x0F, // 0xF ~ 0x12

  EE_LOG_OFFSET           = 0x13, // 0x13 ~ 0x16
  EE_LOG_HEADER           = 0x17, // 0x17 ~ 0x1A
} EepromAddrMap;

// For Log File
typedef struct {
  uint32_t header;
  uint32_t offset;
  uint8_t cur_sector;
} LogFileState;


void startup_process(void);
uint8_t boot_process(uint32_t addr, uint32_t length, uint32_t crc32);
void update_config_data(uint32_t, uint32_t);
HAL_StatusTypeDef Get_Up_Status(UpgradeFlashState *up_status);
HAL_StatusTypeDef Update_Up_Status(UpgradeFlashState *up_status);
void erase_up_status(void);
void print_up_status(void);
HAL_StatusTypeDef Get_Log_Status(LogFileState *log_status);
HAL_StatusTypeDef Update_Log_Status(LogFileState *log_status);

#endif
