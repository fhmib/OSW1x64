#ifndef __UPGRADE_H
#define __UPGRADE_H

#include "main.h"

#define EEPROM_ADDR                   (0x50 << 1)
#define UPGRADE_MAGIC             0xA5A55A5A

typedef enum {
  UPGRADE_UNUSABLE       = 0x00,
  UPGRADE_RESET          = 0x01,
  UPGRADE_FAILURE        = 0x02,
  UPGRADE_SUCCESS        = 0x03,
} UpgradeState;


typedef struct {
  uint32_t magic;
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
  // tag
  EE_TAG_PN               = 0x0000, // Product Number
  EE_TAG_DATE             = 0x0020, // Manufacture Date
  EE_TAG_ASN              = 0x0040, // Assembly Serial Number
  EE_TAG_FSN              = 0x0060, // Filter Serial Number
  // upgrade
  EE_UP_MAIGC             = 0x3000, // 0x3000 ~ 0x3003
  EE_UP_RUN               = 0x3004, // 0x3004 ~ 0x3004 Which fw is running
  EE_UP_FLASH_EMPTY       = 0x3005, // 0x3005 ~ 0x3005 If flash is empty
  EE_UP_LENGTH            = 0x3006, // 0x3006 ~ 0x3009
  EE_UP_CRC32             = 0x300A, // 0x300A ~ 0x300D
  EE_UP_FACTORY_LENGTH    = 0x300E, // 0x300E ~ 0x3011
  EE_UP_FACTORY_CRC32     = 0x3012, // 0x3012 ~ 0x3015
  // log
  EE_LOG_OFFSET           = 0x3020, // 0x3020 ~ 0x3023
  EE_LOG_HEADER           = 0x3024, // 0x3024 ~ 0x3027
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
HAL_StatusTypeDef SW_Dac_Write(uint8_t chan, uint16_t val);

#endif
