#ifndef _CMD_H_
#define _CMD_H_

#define OKA_LEVEL_NOTICE        16
#define OKA_LEVEL_INFO          20
#define OKA_LEVEL_DEBUG         30

void ICACHE_FLASH_ATTR oka_cmd_print_info(int8 debug_level);
void ICACHE_FLASH_ATTR oka_cmd_console_init(int8 debug_level);
void ICACHE_FLASH_ATTR oka_cmd_shell_init();
void ICACHE_FLASH_ATTR oka_cmd_eval(const char *command, size_t len);

#endif // _CMD_H_
