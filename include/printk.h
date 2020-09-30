#ifndef __PRINTK_H__
#define __PRINTK_H__
#include "stdint.h"
void print(uint8_t character);
void println(char*str);
void printint(uint32_t number);
void set_cursor(uint32_t cursor_pos);
#endif