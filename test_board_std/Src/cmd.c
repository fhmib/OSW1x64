#include "cmd.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

int8_t cmd_help(uint8_t argc, char **argv)
{
  uint32_t i;
  char buf[17];

  memset(buf, ' ', 16);
  for (i=0; i < cmd_count; ++i) {
    if (!strcmp(cmdlist[i].info, "INTERNAL")) continue;

    buf[12 - strlen(cmdlist[i].cmd)] = 0;
    PRINT("cmd: ");
    PRINT("%s%s- %s\n", cmdlist[i].cmd, buf, cmdlist[i].desc);
    buf[12 - strlen(cmdlist[i].cmd)] = ' ';
    HAL_Delay(30);
  }
  return 0;
}

int8_t cmd_help2(char *cmd_name)
{
  uint32_t i;

  for (i=0; i < cmd_count; ++i) {
    if (strcasecmp(cmdlist[i].cmd, cmd_name) == 0) {
      PRINT("cmd: ");
      PRINT("%s\n", cmdlist[i].cmd);
      PRINT("     usage:   ");
      PRINT("%s\n", cmdlist[i].info); 
      PRINT("     example: ");
      PRINT("%s\n", cmdlist[i].example);
      break;
    }
  }
  return 0;
}
int8_t process_cmd(const char *buf)
{
  int32_t i, argc = 0;
  int8_t ret;
  char *_argv[32];
  char **argv = &_argv[0];

  char* p = strtok((char*)buf," \t");
  while (p) {
    argv[argc++] = p;
    p = strtok(NULL, " \t");
  }

  if (argc == 0)
    return -1;

  for (i=0; i < cmd_count; ++i) {
    if (strcasecmp(cmdlist[i].cmd, argv[0]) == 0) {
      ret = cmdlist[i].callfn(argc, argv); 
      if (ret) {
        PRINT(CMD_FAILED, ret, ret);
      } else {
        PRINT(CMD_SUCCESS);
      }
      return ret;
    }
  }

  PRINT("Unknown command\n");
  return 0;
}
