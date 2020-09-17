#include "./../include/bitmap.h"
#include "./../include/stdint.h"
#include "./../include/string.h"
#include "./../include/printk.h"
#include "./../include/debug.h"
#include "./../include/interrupt.h"
#include <stdio.h>
/*初始化bitmap*/
void bitmap_init(struct bitmap*bitmap) {
    memset(bitmap->bits,0,bitmap->btmp_bytes_len);
} 

/*判断bit_idx位是否为1，if为1返回true，否则返回false*/
uint8_t bitmap_scan_test(struct bitmap* bitmap,uint32_t bit_idx) {
    uint32_t byte_idx = bit_idx / 8;      //索引数组下标
    uint32_t bit_odd = bit_idx % 8;       //索引具体的bit
    return (bitmap->bits[byte_idx] & (MASK << bit_odd));
}

/*在位图中申请 连续 的countbit，if success return index else return -1*/
int bitmap_scan(struct bitmap* bitmap,uint32_t count) {
    uint32_t idx_byte = 0;
    /*每一个字节进行比较，查看是否字节被全部占用*/
    while((0xff ==bitmap->bits[idx_byte]) && (idx_byte<bitmap->btmp_bytes_len)){
        idx_byte++;
    }
    /*找到一个没有被标记的byte，说明里面对应的4k内存还没有被分配*/
    //ASSERT(idx_byte < bitmap->btmp_bytes_len); 在这里ASSERT内存耗尽将会导致暂停
    if(idx_byte == bitmap->btmp_bytes_len) {   //内存中已经没有可以分配的内存
        return -1;
    }
    int idx_bit=0;
    /*对没有全部标记的byte进行逐位比较*/
    while((uint8_t)(MASK<<idx_bit) & bitmap->bits[idx_byte]) {
        idx_bit++;
    }
    int bit_idx_start = idx_byte * 8 + idx_bit; //空闲位在图中的下标
    if(count==1) {
        return bit_idx_start;
    }
    uint32_t bit_left = bitmap->btmp_bytes_len * 8 - bit_idx_start;
    uint32_t free=1;
    uint32_t next_bit=bit_idx_start+1;

    bit_idx_start =-1;

    while(bit_left-->0) {
        if(!(bitmap_scan_test(bitmap,next_bit))) {
            free++;
        } else {
            free = 0 ; //中间被占用导致不连续
        }
        if(count==free) {
            bit_idx_start=next_bit-count+1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;
}

/*将位图bit_idx对应位设置成value*/
void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,int8_t value) {
    ASSERT(value ==0 || value ==1);
    uint32_t byte_idx =bit_idx / 8 ;
    uint32_t bit_odd = bit_idx % 8;
    if(value) {
        btmp->bits[byte_idx] |= (MASK << bit_odd);
    } else {
        btmp->bits[byte_idx] &=~(MASK << bit_odd);
    }
}

