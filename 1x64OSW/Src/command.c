#include "main.h"
#include "command.h"
#include "flash_if.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "iwdg.h"
#include "i2c.h"
#include "spi.h"
#include "rtc.h"

TransStu trans_buf;
RespondStu resp_buf;

UpgradeStruct up_state;

char *product_number = "01234567"; //8 bytes
char *manu_date = "20200720"; // 8 bytes
char *fw_version = "ONET00.0100.14"; // 14 bytes
char *serial_number = " Pxxxx-xxxxxx"; //16 bytes
char *filter_number = "";

SwitchMapStruct switch_map[] = {
  {SWITCH_NUM_1,  6,  2,  3,  5},
  {SWITCH_NUM_2,  4,  8,  1,  0},
  {SWITCH_NUM_3, 12, 10,  7,  9},
  {SWITCH_NUM_4, 14, 13, 15, 11},
  {SWITCH_NUM_5, 22, 17, 26, 21},
  {SWITCH_NUM_6, 18, 20, 19, 16},
  {SWITCH_NUM_7, 23, 29, 31, 25}
};

extern UpgradeFlashState upgrade_status;

CmdStruct command_list[] = {
  {CMD_UPGRADE_INIT, Cmd_Upgrade_Init},
  {CMD_UPGRADE_DATA, Cmd_Upgrade_Data},
  {CMD_UPGRADE_INSTALL, Cmd_Upgrade_Install},
  {CMD_QUERY_VERSION, Cmd_Get_Version},
  {CMD_SOFTRESET, Cmd_Softreset},
  {CMD_QUERY_TEMP, Cmd_Get_Temperature},
  {CMD_DEVICE_STATUS, Cmd_Device_Status},
  {CMD_QUERY_VOLTAGE, NULL},
  {CMD_QUERY_VOL_THR, NULL},
  {CMD_SET_LOG_TIME, Cmd_Set_Time},
  {CMD_QUERY_LOG_TIME, Cmd_Get_Time},
  {CMD_QUERY_LOG_NUM, Cmd_LOG_Number},
  {CMD_QUERY_LOG, Cmd_LOG_Content},
  {CMD_QUERY_IL, NULL},
  {CMD_SET_SWITCH, NULL},
  {CMD_QUERY_SWITCH, NULL},

  {CMD_FOR_DEBUG, Cmd_For_Debug},
};

uint32_t Cmd_Process()
{
  int i;
  uint32_t cmd_id;

  cmd_id = switch_endian(*(uint32_t*)&trans_buf.buf[CMD_SEQ_MSG_ID]);

  for (i = 0; i < sizeof(command_list) / sizeof(command_list[0]); ++i) {
    if (cmd_id == command_list[i].cmd_id) {
      //EPT("Command id = %#X\n", cmd_id);
      if (command_list[i].func == NULL) {
        break;
      }
      return command_list[i].func();
    }
  }

  EPT("Unknow command id = %#X\n", cmd_id);
  FILL_RESP_MSG(cmd_id, RESPOND_UNKNOWN_CMD, 0);
  return 0;
}

int Uart_Respond(uint32_t cmd, uint32_t status, uint8_t *pdata, uint32_t len)
{
  uint32_t cmd_len = 0;

  trans_buf.buf[cmd_len++] = TRANS_START_BYTE;
  *(uint32_t*)(&trans_buf.buf[cmd_len]) = switch_endian(cmd);
  cmd_len += 4;
  *(uint32_t*)(&trans_buf.buf[cmd_len]) = switch_endian(4 * 4 + len);
  cmd_len += 4;
  if (len) {
    memcpy(&trans_buf.buf[cmd_len], pdata, len);
    cmd_len += len;
  }
  *(uint32_t*)(&trans_buf.buf[cmd_len]) = switch_endian(status);
  cmd_len += 4;
  *(uint32_t*)(&trans_buf.buf[cmd_len]) = switch_endian(Cal_CRC32((uint8_t*)&trans_buf.buf[1], 3 * 4 + len));
  cmd_len += 4;
  
  if (HAL_UART_Transmit(&huart1, trans_buf.buf, cmd_len, 0xFF) != HAL_OK) {
    EPT("Respond command failed\n");
      return -1;
  } else {
    if (status != 0)
      EPT("Respond command = %#X, status = %#X\n", cmd, status);
    return 0;
  }
}

uint32_t Cmd_Upgrade_Init()
{
  uint32_t block_size = switch_endian(*(uint32_t*)&trans_buf.buf[CMD_SEQ_MSG_DATA + 4]);
  uint8_t *p = resp_buf.buf;

  memset(p, 4, 0);

  if (block_size > UPGRADE_MAX_DATA_LENGTH || block_size < UPGRADE_MIN_DATA_LENGTH) {
    EPT("FW received invalid block size %u\n", block_size);
    FILL_RESP_MSG(CMD_UPGRADE_INIT, RESPOND_NOT_CPLT, 4);
    return RESPOND_NOT_CPLT;
  }

  up_state.run = RUN_MODE_UPGRADE;
  up_state.block_size = block_size;
  up_state.pre_state = UPGRADE_UNUSABLE;
  up_state.pre_seq = 0;
  up_state.recvd_length = 0;
  FILL_RESP_MSG(CMD_UPGRADE_INIT, RESPOND_SUCCESS, 4);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Upgrade_Data()
{
  uint8_t *p_fw_data = &trans_buf.buf[CMD_SEQ_MSG_DATA];
  uint8_t *p_resp = resp_buf.buf;
  uint32_t seq = switch_endian(*(uint32_t*)p_fw_data);
  uint32_t length = switch_endian(*(uint32_t*)(p_fw_data + 4));
  uint32_t to = 0;
  //EPT("seq = %u, length = %u\n", seq, length);

  memset(p_resp, 8, 0);
  *(uint32_t*)p_resp = switch_endian(seq);
  p_fw_data += 8;

  if (up_state.run != RUN_MODE_UPGRADE) {
    EPT("Cannot excute command\n");
    FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_UNKNOWN_CMD, 8);
    return RESPOND_UNKNOWN_CMD;
  }

  if (length > up_state.block_size) {
    EPT("FW received invalid data length %u\n", length);
    FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_SEGMENT_ERR, 8);
    return RESPOND_SEGMENT_ERR;
  }

  if (seq == 0x1) {
    if (length != up_state.block_size) {
      EPT("FW received invalid data length : %u\n", length);
      FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_SEGMENT_ERR, 8);
      return RESPOND_SEGMENT_ERR;
    }
    if (strcmp((char*)&p_fw_data[FW_HEAD_MODULE_NAME], "ONET-OSW1x64")) {
      EPT("The file is not the firmware corresponding to this module\n");
      PRINT_HEX("module name", p_fw_data, 0x20);
      FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_UNKNOWN_CMD, 8);
      return RESPOND_UNKNOWN_CMD;
    }
    up_state.pre_seq = seq;
    up_state.recvd_length = 0;
    up_state.fw_size = (p_fw_data[FW_HEAD_FW_LENGTH + 0] << 24) | \
                      (p_fw_data[FW_HEAD_FW_LENGTH + 1] << 16) | \
                      (p_fw_data[FW_HEAD_FW_LENGTH + 2] <<  8 )| \
                      (p_fw_data[FW_HEAD_FW_LENGTH + 3] <<  0 );
      
    up_state.crc32 =  (p_fw_data[FW_HEAD_CRC + 0] << 24) | \
                      (p_fw_data[FW_HEAD_CRC + 1] << 16) | \
                      (p_fw_data[FW_HEAD_CRC + 2] <<  8 )| \
                      (p_fw_data[FW_HEAD_CRC + 3] <<  0 );
    EPT("Fw size = %u, crc = %#X\n", up_state.fw_size, up_state.crc32);

    if (!upgrade_status.flash_empty) {
      EPT("flash is not empty\n");
      // erase flash
      if (flash_in_use) {
        EPT("Flash in using\n");
      } else {
        flash_in_use = 1;
        up_state.is_erasing = 1;
        // erase flash
        if (up_state.upgrade_addr != RESERVE_ADDRESS)
          FLASH_If_Erase_IT(up_state.upgrade_sector);
        EPT("erase sector...\n");
      }
    }

    p_fw_data += FW_HEAD_HEADER_LENGTH;
    length -= FW_HEAD_HEADER_LENGTH;
  } else if ((seq == up_state.pre_seq + 1 && up_state.pre_state == UPGRADE_SUCCESS) || \
      (seq == up_state.pre_seq && up_state.pre_state == UPGRADE_FAILURE)) {
    up_state.pre_seq = seq;
  } else {
    EPT("Seq invalid : %u\tpre_seq : %u\n", seq, up_state.pre_seq);
    FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_UNKNOWN_CMD, 8);
    return RESPOND_UNKNOWN_CMD;
  }

  while (flash_in_use && to++ < 6) {
    osDelay(pdMS_TO_TICKS(100));
  }
  if (to >= 6) {
    EPT("Waiting flash timeout\n");
    up_state.pre_state = UPGRADE_FAILURE;
    FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_NOT_CPLT, 8);
    return RESPOND_NOT_CPLT;
  }

  if (upgrade_status.flash_empty) {
    upgrade_status.flash_empty = 0;
    if (Update_Up_Status(&upgrade_status) != osOK) {
      EPT("Update upgrade status to eeprom failed\n");
      up_state.pre_state = UPGRADE_FAILURE;
      FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_NOT_CPLT, 8);
      return RESPOND_NOT_CPLT;
    }
  }
  FLASH_If_Write(up_state.upgrade_addr + up_state.recvd_length, (uint32_t*)p_fw_data, length / 4);
  up_state.recvd_length += length;
  up_state.pre_state = UPGRADE_SUCCESS;
  FILL_RESP_MSG(CMD_UPGRADE_DATA, RESPOND_SUCCESS, 8);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Upgrade_Install()
{
  uint8_t *p = resp_buf.buf;

  memset(p, 4, 0);

  if (up_state.run != RUN_MODE_UPGRADE) {
    EPT("Cannot excute cmd beacuse of wrong mode\n");
    FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_UNKNOWN_CMD, 4);
    return RESPOND_UNKNOWN_CMD;
  }

  if (up_state.recvd_length != up_state.fw_size) {
    EPT("The received length %u is not equal to length in head %u.\n", up_state.recvd_length, up_state.fw_size);
    FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_DL_CHK_ERR, 4);
    return RESPOND_DL_CHK_ERR;
  }

  if (up_state.pre_seq < 3) {
    EPT("No valid data.\n");
    FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_DL_CHK_ERR, 4);
    return RESPOND_DL_CHK_ERR;
  }

  uint8_t *pdata = (uint8_t*)up_state.upgrade_addr;
  uint32_t crc = Cal_CRC32(pdata, up_state.fw_size);
  if (crc ^ up_state.crc32) {
    EPT("CRC verified failed. %#X != %#X\n", crc, up_state.crc32);
    FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_DL_CHK_ERR, 4);
    return RESPOND_DL_CHK_ERR;
  }

  upgrade_status.flash_empty = 0;
  upgrade_status.run = up_state.upgrade_addr == APPLICATION_1_ADDRESS ? 1 : 2;
  upgrade_status.length = up_state.fw_size;
  upgrade_status.crc32 = up_state.crc32;
  EPT("upgrade_status: %#X, %u, %u, %u, %#X, %u, %#X\n", upgrade_status.magic, upgrade_status.run, upgrade_status.flash_empty, upgrade_status.length, upgrade_status.crc32,\
                upgrade_status.factory_length, upgrade_status.factory_crc32);
  if (Update_Up_Status(&upgrade_status) != osOK) {
    EPT("Update upgrade status to eeprom failed\n");
    FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_DL_INSTALL_FAIL, 4);
    return RESPOND_DL_INSTALL_FAIL;
  }

  FILL_RESP_MSG(CMD_UPGRADE_INSTALL, RESPOND_SUCCESS, 4);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Get_Version()
{
  char *p = (char*)resp_buf.buf;
  
  memset(p, 116, 0);
  p += 4;
  p += 16;
  strcpy(p, product_number);
  p += 10;
  strcpy(p, manu_date);
  p += 10;
  strcpy(p, fw_version);
  p += 37;
  strcpy(p, serial_number);
  p += 20;
  strcpy(p, filter_number);
  p += 23;
  FILL_RESP_MSG(CMD_QUERY_VERSION, RESPOND_SUCCESS, 120);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Softreset()
{
  // TODO: Save system configuration data

  __NVIC_SystemReset();

  return RESPOND_SUCCESS;
}

uint32_t Cmd_Get_Temperature()
{
  uint32_t vol;
  int16_t res;
  double voltage, temp;
  int32_t *p = (int32_t*)resp_buf.buf;

  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, CMD_WAIT_TEMP_TIME) != HAL_OK) {
    EPT("Poll for temperature sensor failed\n");
    *p = switch_endian(1000);
    HAL_ADC_Stop(&hadc1);
    FILL_RESP_MSG(CMD_QUERY_TEMP, RESPOND_SUCCESS, 4);
    return RESPOND_FAILURE;
  }
  vol = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  voltage = ((double)vol / (double)4096) * 3.3;
  temp = (voltage - 0.76) / 0.0025 + 25;
  res = (int16_t)(temp * 10);
  if (res > 1250 || res < -45) {
    EPT("res invalid\n");
    *p = switch_endian(1000);
    FILL_RESP_MSG(CMD_QUERY_TEMP, RESPOND_SUCCESS, 4);
    return RESPOND_FAILURE;
  }
  *p = switch_endian((int32_t)res);

  FILL_RESP_MSG(CMD_QUERY_TEMP, RESPOND_SUCCESS, 4);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Device_Status()
{
  *(uint32_t*)resp_buf.buf = 0;
  FILL_RESP_MSG(CMD_DEVICE_STATUS, RESPOND_SUCCESS, 4);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Set_Time(void)
{
  char buf[5] = {0};
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  uint32_t temp;

  memset(resp_buf.buf, 0, 4);
  memset(&time, 0, sizeof(time));
  date.WeekDay = 1;
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA, 4);
  temp = strtoul(buf, NULL, 10) - 2000;
  date.Year = (uint8_t)temp;
  buf[2] = 0;
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA + 4, 2);
  date.Month = (uint8_t)strtoul(buf, NULL, 10);
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA + 6, 2);
  date.Date = (uint8_t)strtoul(buf, NULL, 10);
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA + 8, 2);
  time.Hours = (uint8_t)strtoul(buf, NULL, 10);
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA + 10, 2);
  time.Minutes = (uint8_t)strtoul(buf, NULL, 10);
  memcpy(buf, trans_buf.buf + CMD_SEQ_MSG_DATA + 12, 2);
  time.Seconds = (uint8_t)strtoul(buf, NULL, 10);
  EPT("Set time to %u-%u-%u %u:%u:%u\n", 2000 + date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

  FILL_RESP_MSG(CMD_SET_LOG_TIME, RESPOND_SUCCESS, 4);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_Get_Time(void)
{
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  EPT("Get time %04u-%02u-%02u %02u:%02u:%02u\n", 2000 + date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
  sprintf((char*)resp_buf.buf, "%04u%02u%02u%02u%02u%02u", 2000 + date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
  resp_buf.buf[14] = 0;
  resp_buf.buf[15] = 0;

  FILL_RESP_MSG(CMD_QUERY_LOG_TIME, RESPOND_SUCCESS, 16);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_LOG_Number()
{
  int32_t size = log_file_state.offset - log_file_state.header;
  uint32_t packet_count;

  if (log_file_state.offset < log_file_state.header) {
    size += (file_flash_end + 1) - file_flash_addr[0];
  }
  packet_count = size / LOG_PACKET_LENGTH;
  if (size)
    packet_count += 1;
  EPT("size = %d, packet_count = %u\n", size, packet_count);

  BE32_To_Buffer(0, resp_buf.buf);
  BE32_To_Buffer((uint32_t)size, resp_buf.buf + 4);
  BE32_To_Buffer((uint32_t)packet_count, resp_buf.buf + 8);

  FILL_RESP_MSG(CMD_QUERY_LOG_NUM, RESPOND_SUCCESS, 12);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_LOG_Content()
{
  uint8_t *prdata = trans_buf.buf + CMD_SEQ_MSG_DATA;
  uint32_t packet_count, packet_num;
  uint32_t addr;
  uint32_t size;

  size = log_file_state.offset - log_file_state.header;
  if (log_file_state.offset < log_file_state.header) {
    size += (file_flash_end + 1) - file_flash_addr[0];
  }
  packet_count = size / LOG_PACKET_LENGTH;
  if (size)
    packet_count += 1;

  //packet_count = Buffer_To_BE32(prdata);
  if (packet_count != Buffer_To_BE32(prdata)) {
    EPT("packet_count != Buffer_To_BE32(prdata)\n");
    BE32_To_Buffer((uint32_t)packet_count, resp_buf.buf);
    BE32_To_Buffer((uint32_t)0, resp_buf.buf + 4);
    BE32_To_Buffer((uint32_t)0, resp_buf.buf + 8);
    FILL_RESP_MSG(CMD_QUERY_LOG, RESPOND_UNKNOWN_CMD, 12);
    return RESPOND_UNKNOWN_CMD;
  }
  packet_num = Buffer_To_BE32(prdata + 4);
  if (packet_num > packet_count || packet_num == 0) {
    EPT("packet_num = %u invalid\n", packet_num);
    BE32_To_Buffer((uint32_t)packet_count, resp_buf.buf);
    BE32_To_Buffer((uint32_t)0, resp_buf.buf + 4);
    BE32_To_Buffer((uint32_t)0, resp_buf.buf + 8);
    FILL_RESP_MSG(CMD_QUERY_LOG, RESPOND_UNKNOWN_CMD, 12);
    return RESPOND_UNKNOWN_CMD;
  }
  addr = log_file_state.header + (packet_num - 1) * LOG_PACKET_LENGTH;
  if (addr >= file_flash_end + 1) {
    addr = addr - (file_flash_end + 1) + file_flash_addr[0];
  }
  if (packet_count == packet_num) {
    size = log_file_state.offset - addr;
    if (size > LOG_PACKET_LENGTH)
      size = LOG_PACKET_LENGTH;
  } else {
    size = LOG_PACKET_LENGTH;
  }
  EPT("packet_num = %u, dest addr = %#X, size = %u\n", packet_num, addr, size);
  BE32_To_Buffer((uint32_t)packet_count, resp_buf.buf);
  BE32_To_Buffer((uint32_t)packet_num, resp_buf.buf + 4);
  BE32_To_Buffer((uint32_t)size, resp_buf.buf + 8);
  Log_Read(addr, resp_buf.buf + 12, size);
  FILL_RESP_MSG(CMD_QUERY_LOG, RESPOND_SUCCESS, 12 + size);
  return RESPOND_SUCCESS;
}

uint32_t Cmd_For_Debug()
{
  uint8_t *prdata = trans_buf.buf + CMD_SEQ_MSG_DATA;
  uint16_t dac_px_val, dac_nx_val, dac_py_val, dac_ny_val;
  uint32_t i, sw_num;
  int32_t val_x, val_y;
  HAL_StatusTypeDef status;

  uint32_t temp = Buffer_To_BE32(prdata);
  memset(resp_buf.buf, 0, 4);
  if (temp != 0x5A5AA5A5) {
    FILL_RESP_MSG(CMD_FOR_DEBUG, RESPOND_UNKNOWN_CMD, 4);
    return RESPOND_UNKNOWN_CMD;
  }
  
  temp = Buffer_To_BE32(prdata + 4);
  if (temp == CMD_DEBUG_DAC_SW) {
    sw_num = Buffer_To_BE32(prdata + 8);
    val_x = (int32_t)Buffer_To_BE32(prdata + 12);
    val_y = (int32_t)Buffer_To_BE32(prdata + 16);
    if (sw_num == 0 || sw_num >= SWITCH_NUM_TOTAL) {
      FILL_RESP_MSG(CMD_FOR_DEBUG, RESPOND_FAILURE, 4);
      return RESPOND_FAILURE;
    }
    for (i = 0; i < sizeof(switch_map)/sizeof(switch_map[0]); ++i) {
      if (switch_map[i].sw_num == sw_num) {
        // x
        if (val_x >= 0) {
          dac_px_val = (uint16_t)val_x;
          dac_nx_val = 0;
        } else {
          dac_px_val = 0;
          dac_nx_val = (uint16_t)my_abs(val_x);
        }
        // y
        if (val_y >= 0) {
          dac_py_val = (uint16_t)val_y;
          dac_ny_val = 0;
        } else {
          dac_py_val = 0;
          dac_ny_val = (uint16_t)my_abs(val_y);
        }
        break;
      }
    }
    EPT("Debug: sw_num = %u, px = %u, nx = %u, py = %u, ny = %u\n", sw_num, dac_px_val, dac_nx_val, dac_py_val, dac_ny_val);
    // write dac
    status = SW_Dac_Write(switch_map[i].px_chan, dac_px_val);
    status = SW_Dac_Write(switch_map[i].nx_chan, dac_nx_val);
    status = SW_Dac_Write(switch_map[i].py_chan, dac_py_val);
    status = SW_Dac_Write(switch_map[i].ny_chan, dac_ny_val);
    if (status != HAL_OK) {
      EPT("Write dac failed\n");
      FILL_RESP_MSG(CMD_FOR_DEBUG, RESPOND_FAILURE, 4);
      return RESPOND_FAILURE;
    }
    val_x = switch_map[i].px_chan << 24;
    val_x |= switch_map[i].nx_chan << 16;
    val_x |= switch_map[i].py_chan << 8;
    val_x |= switch_map[i].ny_chan << 0;
    BE32_To_Buffer(val_x, resp_buf.buf);
    val_x = dac_px_val << 16;
    val_x |= dac_nx_val << 0;
    BE32_To_Buffer(val_x, resp_buf.buf + 4);
    val_x = dac_py_val << 16;
    val_x |= dac_ny_val << 0;
    BE32_To_Buffer(val_x, resp_buf.buf + 8);
    FILL_RESP_MSG(CMD_FOR_DEBUG, RESPOND_SUCCESS, 12);
    return RESPOND_SUCCESS;
  }

  FILL_RESP_MSG(CMD_FOR_DEBUG, RESPOND_UNKNOWN_CMD, 4);
  return RESPOND_UNKNOWN_CMD;
}

HAL_StatusTypeDef SW_Dac_Write(uint8_t chan, uint16_t val)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t buf[3];

  buf[0] = ((chan & 0x1F) << 3) | ((val >> 11) & 0x7);
  buf[1] = val >> 3;
  buf[2] = (val & 0x7) << 5;
  EPT("Dac value: %#X %#X %#X\n", buf[0], buf[1], buf[2]);
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
  status = HAL_SPI_Transmit(&hspi1, buf, 3, 50);
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

  return status;
}

uint32_t my_abs(int32_t val)
{
  char buf[32];
  uint32_t p_val;
  
  sprintf(buf, "%d", val);
  if (buf[0] == '-') {
    p_val = strtoul(buf + 1, NULL, 10);
  } else {
    p_val = val;
  }
  
  return p_val;
}
