#ifndef __BITMAP_H__
#define __BITMAP_H__
#include "./../include/global.h"
#define MASK 1

struct bitmap
{
    uint32_t btmp_bytes_len;
    /*按照字节进行寻址，寻找到字节进行位操作*/
    uint8_t * bits;         //字节型指针，定位到bitmap的首字节，uint32_t导致每次寻址4B，会出现问题
};

void bitmap_init(struct bitmap* btmp);
uint8_t bitmap_scan_test(struct bitmap*btmp,uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp,uint32_t count);
void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,int8_t value);

#endif