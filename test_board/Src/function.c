#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"
#include "function.h"
#include "main.h"
#include "usart.h"

const char *INVALID_ARG = "Invalid arguments";
const char *HELP = "help";
#define CMD_FAILED "Command execution failed, Returned code is %#X\n"
#define CMD_SUCCESS "Command executed successfully\n"
#define UP_SUCCESS "Package %u send success\n"

#if 0
#define GET_RESULT()  do {\
                        if (ret >= 0) {\
                          if (rBuf[rLen - 2] != 0) {\
                            PRINT(CMD_FAILED, rBuf[rLen - 2]);\
                          } else {\
                            PRINT(CMD_SUCCESS);\
                          }\
                        } else {\
                          PRINT(CMD_FAILED, ret);\
                        }\
                      } while(0)
#endif

uint8_t fw_buf[40 * 1024];
uint8_t rBuf[TRANS_MAX_LENGTH];
uint32_t rLen;


int8_t cmd_version(uint8_t argc, char **argv)
{
  int8_t ret;
  uint8_t buf[4] = {0};
  uint8_t *p;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_QUERY_VERSION, buf, 4, rBuf, &rLen);
  
  if (ret) {
    return ret;
  }

  //PRINT_HEX("Received", rBuf, rLen);

  p = rBuf + CMD_SEQ_MSG_DATA;
  p += 4;
  p += 16;
  PRINT("Product Number: %s\n", (char*)p);
  p += 10;
  PRINT("Manufacture Date: %s\n", (char*)p);
  p += 10;
  PRINT("Firmware Version: %s\n", (char*)p);
  p += 37;
  PRINT("Assembly Serial Number: %s\n", (char*)p);
  p += 20;
  PRINT("Filter Serial Number: %s\n", (char*)p);

  return ret;
}

int8_t cmd_upgrade(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcmp(argv[1], "init")) {
    return upgrade_init();
  } else if (argc == 2 && !strcmp(argv[1], "file")) {
    return upgrade_file();
  } else if (argc == 2 && !strcmp(argv[1], "install")) {
    return upgrade_install();
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

#define FW_BLOCK_SIZE 1024
int8_t upgrade_init()
{
  uint8_t buf[12];

  PRINT("Initialize Upgrade\n");
  *(uint32_t*)(&buf[0]) = switch_endian(1);
  *(uint32_t*)(&buf[4]) = switch_endian(FW_BLOCK_SIZE);
  *(uint32_t*)(&buf[8]) = 0;
  return process_command(CMD_UPGRADE_INIT, buf, 12, rBuf, &rLen);
}

int8_t upgrade_file()
{
  int8_t ret;
  uint8_t buf[FW_BLOCK_SIZE + 4 + 4];
  uint32_t fw_crc, seq = 1;
  uint32_t fw_length, every_size, send_size = 0;
  uint8_t retry = 0;

  __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
  uart1_irq_sel = 0;
  PRINT2("Download image...\n");

  if (HAL_UART_Receive(&huart1, fw_buf, 256, 1000 * 60) != HAL_OK) {
    PRINT2("Timeout\n");
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    uart1_irq_sel = 1;
    return 0;
  }
  fw_length = (fw_buf[FW_HEAD_FW_LENGTH] << 24) | (fw_buf[FW_HEAD_FW_LENGTH + 1] << 16) |\
           (fw_buf[FW_HEAD_FW_LENGTH + 2] << 8) | (fw_buf[FW_HEAD_FW_LENGTH + 3] << 0);
  fw_crc = (fw_buf[FW_HEAD_CRC] << 24) | (fw_buf[FW_HEAD_CRC + 1] << 16) |\
           (fw_buf[FW_HEAD_CRC + 2] << 8) | (fw_buf[FW_HEAD_CRC + 3] << 0);
  HAL_UART_Receive(&huart1, fw_buf + 256, fw_length, 1000 * 10);
  PRINT2("Download success, Length = %u, crc = %#X\n", fw_length, fw_crc);
  if (Cal_CRC32(&fw_buf[256], fw_length) == fw_crc) {
    PRINT2("CRC32 success\n");
  } else {
    PRINT2("CRC32 failed\n");
  }

  PRINT2("Sending image...\n");
  while (send_size < fw_length + FW_FILE_HEADER_LENGTH) {
    *(uint32_t*)(&buf[0]) = switch_endian(seq);
    every_size = send_size + FW_BLOCK_SIZE > fw_length + FW_FILE_HEADER_LENGTH ?\
                 fw_length + FW_FILE_HEADER_LENGTH - send_size : FW_BLOCK_SIZE ;
    *(uint32_t*)(&buf[4]) = switch_endian(every_size);
    //PRINT2("send_size = %u\n", send_size);
    memcpy(&buf[8], &fw_buf[send_size], every_size);
    ret = process_command(CMD_UPGRADE_DATA, buf, 8 + every_size, rBuf, &rLen);
    if (ret) {
      if (seq == 1 && retry < 3) {
        PRINT2("Retry\n");
        retry++;
        continue;
      }
      break;
    }
    seq++;
    send_size += every_size;
  }

  if (send_size == fw_length + FW_FILE_HEADER_LENGTH)
    PRINT2("Send fw success\n");
  else
    PRINT2("Send fw failed\n");

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  uart1_irq_sel = 1;

  return ret;
}

int8_t upgrade_install()
{
  uint8_t buf[4] = {0};
  return process_command(CMD_UPGRADE_INSTALL, buf, 4, rBuf, &rLen);
}

/*
int8_t cmd_log(uint8_t argc, char **argv)
{
  if (argc == 2 && !strcmp(argv[1], "packets")) {
    return log_packets();
  } else if (argc == 4 && !strcmp(argv[1], "get")) {
    return log_obtain(argv[2], argv[3]);
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t log_packets()
{
  int8_t ret;
  uint32_t size, count;

  ret = process_command(CMD_LOG_NUMBER, NULL, 0, rBuf, &rLen);

  GET_RESULT(ret, rBuf, rLen);

  if (!ret) {
    size = Buffer_To_BE32(&rBuf[CMD_SEQ_DATA]);
    count = Buffer_To_BE32(&rBuf[CMD_SEQ_DATA + 4]);
    PRINT("size = %u, packets = %u\n", size, count);
  }

  return ret;
}

int8_t log_obtain(char *arg1, char *arg2)
{
  int8_t ret;
  uint32_t packets, number, size;
  uint8_t buf[8];
  
  packets = strtoul(arg1, NULL, 0);
  number = strtoul(arg2, NULL, 0);
  
  BE32_To_Buffer(packets, buf);
  BE32_To_Buffer(number, buf + 4);
  ret = process_command(CMD_LOG_CONTENT, buf, 8, rBuf, &rLen);

  GET_RESULT(ret, rBuf, rLen);
  if (!ret) {
    packets = Buffer_To_BE32(&rBuf[CMD_SEQ_DATA]);
    number = Buffer_To_BE32(&rBuf[CMD_SEQ_DATA + 4]);
    size = Buffer_To_BE32(&rBuf[CMD_SEQ_DATA + 8]);
    PRINT("size = %u, packets = %u, number = %u\n", size, packets, number);
    PRINT_CHAR("LOG", &rBuf[CMD_SEQ_DATA + 12], rLen - 2 - 4 - 12);
  }

  return ret;
}

int8_t cmd_upgrade(uint8_t argc, char **argv)
{
  if (argc == 3 && !strcmp(argv[1], "mode")) {
    return upgrade_mode(argv[2]);
  } else if (argc == 2 && !strcmp(argv[1], "file")) {
    return upgrade_file();
  } else if (argc == 2 && !strcmp(argv[1], "run")) {
    return upgrade_run();
  } else {
    cmd_help2(argv[0]);
  }
  return 0;
}

int8_t upgrade_mode(char *arg)
{
  int8_t ret;
  uint8_t buf;

  if (!strcmp(arg, "0")) {
    PRINT("Change mode to Application\n");
    buf = 0;
    ret = process_command(CMD_UPGRADE_MODE, &buf, 1, rBuf, &rLen);
  } else if (!strcmp(arg, "1")) {
    PRINT("Change mode to Upgrade\n");
    buf = 1;
    ret = process_command(CMD_UPGRADE_MODE, &buf, 1, rBuf, &rLen);
  } else {
    PRINT("%s\n", INVALID_ARG);
    return -1;
  }

  GET_RESULT(ret, rBuf, rLen);

  return ret;
}

int8_t upgrade_file()
{
  int8_t ret;
  uint8_t buf[130];
  uint16_t fw_crc, seq = 1;
  uint32_t fw_length, every_size = 128, send_size = 0;

  __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
  uart1_irq_sel = 0;
  PRINT2("Download image...\n");

  if (HAL_UART_Receive(&huart1, fw_buf, 256, 1000 * 60) != HAL_OK) {
    PRINT2("Timeout\n");
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    uart1_irq_sel = 1;
    return 0;
  }
  fw_length = (fw_buf[FW_FILE_FW_LENGTH] << 24) | (fw_buf[FW_FILE_FW_LENGTH + 1] << 16) |\
           (fw_buf[FW_FILE_FW_LENGTH + 2] << 8) | (fw_buf[FW_FILE_FW_LENGTH + 3] << 0);
  fw_crc = (fw_buf[FW_FILE_CRC] << 8) | fw_buf[FW_FILE_CRC + 1];
  HAL_UART_Receive(&huart1, fw_buf + 256, fw_length, 0xFFFFFFFF);
  PRINT2("Download success, Length = %u, crc = %#X\n", fw_length, fw_crc);
  if (Cal_CRC16(&fw_buf[256], fw_length) == fw_crc) {
    PRINT2("CRC16 success\n");
  } else {
    PRINT2("CRC16 failed\n");
  }

  PRINT2("Sending image...\n");
  while (send_size < fw_length + 256) {
    buf[0] = seq >> 8;
    buf[1] = seq++;
    if (every_size + send_size <= fw_length + 256) {
      memcpy(&buf[2], &fw_buf[send_size], every_size);
      send_size += every_size;
    } else {
      memset(&buf[2], 0, every_size);
      memcpy(&buf[2], &fw_buf[send_size], fw_length + 256 - send_size);
      send_size = fw_length + 256;
    }
    ret = process_command(CMD_UPGRADE_DATA, buf, 130, rBuf, &rLen);
    if (ret >= 0) {
      if (rBuf[rLen - 2] != 0) {
        PRINT2(CMD_FAILED, rBuf[rLen - 2]);
        break;
      } else {
        //PRINT2(UP_SUCCESS, seq - 1);
      }
    } else {
      PRINT2(CMD_FAILED, ret);
      break;
    }
  }

  if (send_size >= fw_length + 256)
    PRINT2("Send fw success\n");
  else
    PRINT2("Send fw failed\n");

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  uart1_irq_sel = 1;

  return ret;
}

int8_t upgrade_run()
{
  int8_t ret;

  ret = process_command(CMD_UPGRADE_RUN, NULL, 0, rBuf, &rLen);

  GET_RESULT(ret, rBuf, rLen);

  return ret;
}

int8_t cmd_reset(uint8_t argc, char **argv)
{
  int8_t ret;

  if (argc > 1) {
    cmd_help2(argv[0]);
    return 0;
  }

  ret = process_command(CMD_SOFTRESET, NULL, 0, rBuf, &rLen);

  GET_RESULT(ret, rBuf, rLen);

  return ret;
}

int8_t cmd_for_test(uint8_t argc, char **argv)
{
  int8_t ret = 0;
  uint8_t buf[8];
  
  uint8_t val;
  uint16_t count, addr;

  if (argc == 2 && !strcmp(argv[1], "wd")) {
    buf[0] = 0;
    ret = process_command(CMD_FOR_TEST, buf, 1, rBuf, &rLen);
    GET_RESULT(ret, rBuf, rLen);
  } else if (argc == 6 && !strcmp(argv[1], "eeprom") && !strcmp(argv[2], "write")) {
    addr = (uint16_t)strtoul(argv[3], NULL, 0);
    val = (uint8_t)strtoul(argv[4], NULL, 0);
    count = (uint16_t)strtoul(argv[5], NULL, 0);
    buf[0] = 1;
    buf[1] = (uint8_t)(addr >> 8);
    buf[2] = (uint8_t)addr;
    buf[3] = val;
    buf[4] = (uint8_t)(count >> 8);
    buf[5] = (uint8_t)count;
    ret = process_command(CMD_FOR_TEST, buf, 6, rBuf, &rLen);
    GET_RESULT(ret, rBuf, rLen);
  } else if (argc == 5 && !strcmp(argv[1], "eeprom") && !strcmp(argv[2], "read")) {
    addr = (uint16_t)strtoul(argv[3], NULL, 0);
    count = (uint16_t)strtoul(argv[4], NULL, 0);
    buf[0] = 2;
    buf[1] = (uint8_t)(addr >> 8);
    buf[2] = (uint8_t)addr;
    buf[3] = (uint8_t)(count >> 8);
    buf[4] = (uint8_t)count;
    ret = process_command(CMD_FOR_TEST, buf, 5, rBuf, &rLen);
    GET_RESULT(ret, rBuf, rLen);
    if (!ret) {
      PRINT_HEX("eeprom", &rBuf[CMD_SEQ_DATA], rLen - 2 - 4);
    }
  } else if (argc == 4 && !strcmp(argv[1], "spi") && !strncmp(argv[2], "chan", 4)) {
    val = (uint8_t)strtoul(argv[3], NULL, 0);
    buf[0] = 3;
    buf[1] = val;
    ret = process_command(CMD_FOR_TEST, buf, 2, rBuf, &rLen);
    GET_RESULT(ret, rBuf, rLen);
  } else if (argc == 4 && !strcmp(argv[1], "log")) {
    sscanf(argv[2], "%c", &val);
    count = (uint16_t)strtoul(argv[3], NULL, 0);
    buf[0] = 4;
    buf[1] = val;
    buf[2] = (uint8_t)(count >> 8);
    buf[3] = (uint8_t)count;
    ret = process_command(CMD_FOR_TEST, buf, 4, rBuf, &rLen);
    GET_RESULT(ret, rBuf, rLen);
  } else {
    cmd_help2(argv[0]);
  }

  return ret;
}
*/

int8_t process_command(uint32_t cmd, uint8_t *pdata, uint32_t len, uint8_t *rx_buf, uint32_t *rx_len)
{
  uint32_t cmd_len = 0;
  uint32_t rcv_crc;
  uint32_t rcv_len, err_code;
  uint32_t *trans_msg;

  usart2_tx_buf[cmd_len++] = 0x55;
  *(uint32_t*)(&usart2_tx_buf[cmd_len]) = switch_endian(cmd);
  cmd_len += 4;
  *(uint32_t*)(&usart2_tx_buf[cmd_len]) = switch_endian(4 * 4 + len);
  cmd_len += 4;
  if (len) {
    memcpy(&usart2_tx_buf[cmd_len], pdata, len);
    cmd_len += len;
  }
  *(uint32_t*)(&usart2_tx_buf[cmd_len]) = 0;
  cmd_len += 4;
  rcv_crc = Cal_CRC32((uint8_t*)&usart2_tx_buf[1], 3 * 4 + len);
  *(uint32_t*)(&usart2_tx_buf[cmd_len]) = switch_endian(rcv_crc);
  cmd_len += 4;

  //PRINT("tx crc32 = %#X\n", rcv_crc);
  //PRINT_HEX("tx_buf", usart2_tx_buf, cmd_len);
  if (HAL_UART_Transmit(&huart2, usart2_tx_buf, cmd_len, 0xFF) != HAL_OK) {
    EPT("Transmit failed\n");
    return -1;
  }

  rx_buf[0] = 0;
  while (rx_buf[0] != 0x55) {
    if (HAL_UART_Receive(&huart2, rx_buf, 1, 1000) != HAL_OK) {
      EPT("Receive failed 1\n");
      return -2;
    }
  }
  if (HAL_UART_Receive(&huart2, rx_buf + 1, 8, 0xFF) != HAL_OK) {
    EPT("Receive failed 2\n");
    return -3;
  }
  trans_msg = (uint32_t*)&rx_buf[5];
  rcv_len = switch_endian(*trans_msg);
  if (HAL_UART_Receive(&huart2, rx_buf + 9, rcv_len - 8, 0xFF) != HAL_OK) {
    EPT("Receive failed 3\n");
    PRINT_HEX("Received messages", rx_buf, 9);
    return -4;
  }
  //PRINT_HEX("rx_buf", rx_buf, rcv_len + 1);

  trans_msg = (uint32_t*)&rx_buf[1 + rcv_len - 4];
  rcv_crc = Cal_CRC32(&rx_buf[1], rcv_len - 4);
  if (rcv_crc != switch_endian(*trans_msg)) {
    EPT("Checksum failed\n");
    return -5;
  }

  err_code = switch_endian(*(uint32_t*)&rx_buf[1 + rcv_len - 8]);
  if (err_code != 0) {
    PRINT(CMD_FAILED, err_code);
    return -6;
  }
  *rx_len = rcv_len + 1;

  return 0;
}

uint8_t Cal_Check(uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  uint8_t chk = 0;

  for (i = 0; i < len; ++i) {
    chk ^= pdata[i];
  }
  
  return (chk + 1);
}

/**
  * @brief  Update CRC16 for input byte
  * @param  crc_in input value 
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
  uint32_t crc = crc_in;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }
  
  while(!(in & 0x10000));

  return crc & 0xffffu;
}

/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = p_data+size;

  while(p_data < dataEnd)
    crc = UpdateCRC16(crc, *p_data++);
 
  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

uint32_t Cal_CRC32(uint8_t* packet, uint32_t length)
{
  static uint32_t CRC32_TABLE[] = {
    /* CRC polynomial 0xedb88320 */
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
  };

  uint32_t CRC32 = 0xFFFFFFFF;
  for (uint32_t i = 0; i < length; i++) {
    CRC32 = (CRC32_TABLE[((CRC32) ^ (packet[i])) & 0xff] ^ ((CRC32) >> 8));
  }

  return ~CRC32;
}

void PRINT_HEX(char *head, uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  
  PRINT("************* PRINT HEX *************\n");
  PRINT("%s:\n", head);
  for (i = 0; i < len; ++i) {
    if (i % 0x10 == 0) {
      PRINT("%08X : ", i / 0x10);
    }
    PRINT("0x%02X%s", pdata[i], (i + 1) % 0x10 == 0 ? "\n" : i == len - 1 ? "\n" : " ");
    HAL_Delay(10);
  }
  PRINT("************* PRINT END *************\n");
}

void PRINT_CHAR(char *head, uint8_t *pdata, uint32_t len)
{
  uint32_t i;
  
  PRINT("************* PRINT CHAR *************\n");
  PRINT("%s:\n", head);
  for (i = 0; i < len; ++i) {
    if (i % 0x10 == 0) {
      PRINT("%08X : ", i / 0x10);
    }
    PRINT("%c%s", pdata[i] == '\n' ? 'N' : pdata[i] == '\r' ? 'R' : pdata[i],\
          (i + 1) % 0x10 == 0 ? "\n" : i == len - 1 ? "\n" : " ");
    HAL_Delay(10);
  }
  PRINT("************* PRINT CHAR *************\n");
}

uint32_t switch_endian(uint32_t i)
{
  return (((i>>24)&0xff) | ((i>>8)&0xff00) | ((i<<8)&0xff0000) | ((i<<24)&0xff000000));
}

void BE32_To_Buffer(uint32_t data, uint8_t *pbuf)
{
  pbuf[0] = (uint8_t)(data >> 24);
  pbuf[1] = (uint8_t)(data >> 16);
  pbuf[2] = (uint8_t)(data >> 8);
  pbuf[3] = (uint8_t)(data);
}

uint32_t Buffer_To_BE32(uint8_t *pbuf)
{
  return (pbuf[0] << 24) | (pbuf[1] << 16) | (pbuf[2] << 8) | (pbuf[3]);
}
