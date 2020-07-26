#ifndef __FUNCTION_H
#define __FUNCTION_H

#include <stdint.h>

typedef enum {
  CMD_SEQ_MSG_ID          = 0x01,
  CMD_SEQ_MSG_LENGTH      = 0x05,
  CMD_SEQ_MSG_DATA        = 0x09,
} CmdSeq;

typedef enum {
  CMD_UPGRADE_INIT        = 0x10, // Firmware Download Init
  CMD_UPGRADE_DATA        = 0x11, // Firmware Load
  CMD_UPGRADE_INSTALL     = 0x12, // Firmware install
  CMD_QUERY_VERSION       = 0x30, // Version Request
  CMD_SOFTRESET           = 0x40, // Device Reset
  CMD_QUERY_TEMP          = 0x50, // Device Temperature Request
  CMD_DEVICE_STATUS       = 0x60, // Device Status Check
  CMD_QUERY_VOLTAGE       = 0x70, // Device Voltage Request
  CMD_QUERY_VOL_THR       = 0x72, // Voltage Alarm Threshold Request Command
  CMD_SET_LOG_TIME        = 0x73, // Set Device Log Time Commands
  CMD_QUERY_LOG_TIME      = 0x74, // Get Device Log Time Commands
  CMD_QUERY_LOG_NUM       = 0x75, // Get Device Error Log Packet Number
  CMD_QUERY_LOG           = 0x76, // Get Device Error Log Commands
  CMD_QUERY_IL            = 0x77, // Optical Switch Insertion Loss Request Command
  CMD_SET_SWITCH          = 0xC0, // Set Switch
  CMD_QUERY_SWITCH        = 0xD0, // Switch Status Request

  // for test
  CMD_FOR_TEST            = 0xFF,
} CmdId;

typedef enum {
  FW_HEAD_MODULE_NAME    = 0x00,
  FW_HEAD_FW_LENGTH      = 0xC0,
  FW_HEAD_CRC            = 0xC4,
  FW_HEAD_END            = 0xFF,
  FW_FILE_HEADER_LENGTH,
} FwFileHeader;


int8_t cmd_version(uint8_t argc, char **argv);
int8_t cmd_upgrade(uint8_t argc, char **argv);
int8_t upgrade_init(void);
int8_t upgrade_file(void);
int8_t upgrade_install(void);
/*
int8_t cmd_log(uint8_t argc, char **argv);
int8_t log_packets(void);
int8_t log_obtain(char *arg1, char *arg2);
int8_t cmd_upgrade(uint8_t argc, char **argv);
int8_t upgrade_mode(char *arg);
int8_t upgrade_file(void);
int8_t upgrade_run(void);
int8_t cmd_reset(uint8_t argc, char **argv);
int8_t cmd_for_test(uint8_t argc, char **argv);
*/

int8_t process_command(uint32_t cmd, uint8_t *pdata, uint32_t len, uint8_t *rx_buf, uint32_t *rx_len);
uint8_t Cal_Check(uint8_t *pdata, uint32_t len);
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);
uint32_t Cal_CRC32(uint8_t* packet, uint32_t length);
void PRINT_HEX(char *head, uint8_t *pdata, uint32_t len);
void PRINT_CHAR(char *head, uint8_t *pdata, uint32_t len);
uint32_t switch_endian(uint32_t i);
void BE32_To_Buffer(uint32_t data, uint8_t *pbuf);
uint32_t Buffer_To_BE32(uint8_t *pbuf);

#endif
