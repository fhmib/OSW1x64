#ifndef __command_H
#define __command_H

#include "main.h"
#include "cmsis_os.h"

extern char *fw_version;

#define TAG_MAX_SPACE               0x20

#define TRANS_START_BYTE            0x55
#define TRANS_MAX_LENGTH            1200
#define TRANS_WAIT_HEAD_TIME        20    //ms
#define TRANS_WAIT_TIME             500   //ms
#define COMMAND_PROCESS_TIME        900   //ms

// Respond Code
#define  RESPOND_SUCCESS            0x00000000
#define  RESPOND_CHECKSUM_ERR       0x000027A3
#define  RESPOND_LENGTH_ERR         0x000027A4
#define  RESPOND_SEGMENT_ERR        0x000027A5
#define  RESPOND_NOT_CPLT           0x0000277D
#define  RESPOND_UNKNOWN_CMD        0x00002783
#define  RESPOND_RESET_FAIL         0x00002784 //Reset Failure and Reboot from previous version of firmware
#define  RESPOND_DL_CHK_ERR         0xFFFFFFF6 //Downloaded Firmware Checksum Does Not Match Firmware Header
#define  RESPOND_DL_INSTALL_FAIL    0xFFFFFFF7 //Firmware Failed to Install, Flash Switch Did Not Complete
#define  RESPOND_FAILURE            0xFFFFFFFF //Downloaded Firmware Checksum Does Not Match Firmware Header


#define FILL_RESP_MSG(c, s, l)        do {\
                                        resp_buf.cmd = c;\
                                        resp_buf.status = s;\
                                        resp_buf.length = l;\
                                      } while(0)

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
  CMD_FOR_DEBUG           = 0x7FFFFFFF,
} CmdId;

typedef enum {
  CMD_DEBUG_SW_DAC        = 0x01,
  CMD_DEBUG_SW_ADC        = 0x02,
  CMD_DEBUG_VOL_ADC       = 0x03,
  CMD_DEBUG_TAG           = 0x04,
} CmdDebugId;

typedef enum {
  CMD_SEQ_MSG_ID          = 0x01,
  CMD_SEQ_MSG_LENGTH      = 0x05,
  CMD_SEQ_MSG_DATA        = 0x09,
} CmdSeq;

typedef struct {
  uint8_t stage;
  uint32_t length;
  uint8_t buf[TRANS_MAX_LENGTH + 1];
} TransStu;

typedef struct {
  uint32_t cmd;
  uint32_t status;
  uint32_t length;
  uint8_t buf[TRANS_MAX_LENGTH + 1];
} RespondStu;

typedef enum {
  TRANS_WAIT_START,
  TRANS_WAIT_HEADER,
  TRANS_RECV_DATA,
} TransStage;

typedef uint32_t (*cmdFunc)(void);

typedef struct {
  uint32_t cmd_id;
  cmdFunc func;
}CmdStruct;


typedef enum {
  SWITCH_ALL_CHANNEL = 0,
  SWITCH_NUM_1 = 1,
  SWITCH_NUM_2 = 2,
  SWITCH_NUM_3 = 3,
  SWITCH_NUM_4 = 4,
  SWITCH_NUM_5 = 5,
  SWITCH_NUM_6 = 6,
  SWITCH_NUM_7 = 7,
  SWITCH_NUM_TOTAL,
} SwitchNumber;

typedef struct {
  uint32_t sw_num;
  uint8_t px_chan; // dac channel correspond to positive x
  uint8_t nx_chan; // dac channel correspond to negatve x
  uint8_t py_chan; // dac channel correspond to positive y
  uint8_t ny_chan; // dac channel correspond to negatve y
  uint8_t adc_px_chan; // adc channel correspond to positive x
  uint8_t adc_nx_chan; // adc channel correspond to negatve x
  uint8_t adc_py_chan; // adc channel correspond to positive y
  uint8_t adc_ny_chan; // adc channel correspond to negatve y
} SwitchMapStruct;


#define UPGRADE_MAX_DATA_LENGTH     1024
#define UPGRADE_MIN_DATA_LENGTH     256

typedef enum {
  FW_HEAD_MODULE_NAME    = 0x00,
  FW_HEAD_FW_LENGTH      = 0xC0,
  FW_HEAD_CRC            = 0xC4,
  FW_HEAD_END            = 0xFF,
  FW_HEAD_HEADER_LENGTH,
} FwFileHeader;

typedef struct {
  uint8_t run;
  uint8_t is_erasing;
  
  uint32_t upgrade_addr;
  uint32_t upgrade_sector;
  
  uint8_t pre_state;
  uint32_t pre_seq;
  
  uint32_t crc32;
  uint32_t fw_size;
  uint32_t block_size;
  uint32_t recvd_length;
} UpgradeStruct;

typedef enum {
  RUN_MODE_APPLICATION = 0x0,
  RUN_MODE_UPGRADE = 0x1,
} RunMode;

typedef enum {
  UPGRADE_UNUSABLE       = 0x00,
  UPGRADE_RESET          = 0x01,
  UPGRADE_FAILURE        = 0x02,
  UPGRADE_SUCCESS        = 0x03,
} UpgradeState;


uint32_t Cmd_Process(void);
int Uart_Respond(uint32_t cmd, uint32_t status, uint8_t *pdata, uint32_t len);

uint32_t Cmd_Upgrade_Init(void);
uint32_t Cmd_Upgrade_Data(void);
uint32_t Cmd_Upgrade_Install(void);
uint32_t Cmd_Get_Version(void);
uint32_t Cmd_Softreset(void);
uint32_t Cmd_Get_Temperature(void);
uint32_t Cmd_Device_Status(void);
uint32_t Cmd_Voltage(void);
uint32_t Cmd_Set_Time(void);
uint32_t Cmd_Get_Time(void);
uint32_t Cmd_LOG_Number(void);
uint32_t Cmd_LOG_Content(void);

uint32_t Cmd_For_Debug(void);
uint32_t debug_sw_dac(uint8_t sw_num, int32_t val_x, int32_t val_y);
uint32_t debug_sw_adc(uint8_t sw_num);
uint32_t debug_vol_adc(uint8_t chan);
uint32_t debug_tag(uint8_t type, uint8_t *p, uint32_t length);

#endif
