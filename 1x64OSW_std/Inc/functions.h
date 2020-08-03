#ifndef __functions_H
#define __functions_H

#include "main.h"

typedef struct {
  uint8_t type;
  void *pbuf;
  uint32_t length;
} MsgStruct;

typedef enum {
  MSG_TYPE_LOG,
  MSG_TYPE_FLASH_ISR,
} MsgType;

// For Log File
typedef struct {
  uint32_t header;
  uint32_t offset;
  uint8_t cur_sector;
} LogFileState;

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

void Throw_Log(uint8_t *buf, uint32_t length);
uint32_t Log_Write(uint32_t addr, uint8_t *pbuf, uint32_t length);
uint32_t Log_Write_byte(uint32_t addr, uint8_t ch, uint32_t length);
uint32_t Log_Read(uint32_t addr, uint8_t *pbuf, uint32_t length);


typedef struct {
  uint32_t magic;
  uint8_t run;
  uint8_t flash_empty;
  uint32_t length;
  uint32_t crc32;
  uint32_t factory_length;
  uint32_t factory_crc32;
} UpgradeFlashState;

osStatus_t Get_Up_Status(UpgradeFlashState *status);
osStatus_t Update_Up_Status(UpgradeFlashState *status);
osStatus_t Get_Log_Status(LogFileState *log_status);
osStatus_t Update_Log_Status(LogFileState *log_status);

uint32_t Get_SW_By_IO(void);

#endif
