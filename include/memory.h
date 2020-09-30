#ifndef __MEMORY_H__
#define __MEMORY_H__
#include "stdint.h"
#include "bitmap.h"

struct virtual_addr {
    struct bitmap vaddr_bitmap; //虚拟地址位图
    uint32_t vaddr_start;       //虚拟地址起始地址
    // uint32_t pool_size;      // 虚拟内存最大4GB，接近无限，不需要再定义大小
};
enum pool_flags {
    PF_KERNEL = 1,  //内核内存池
    PF_USER = 2    //用户内存池
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void malloc_init(void);
uint32_t* pte_ptr(uint32_t vaddr);
uint32_t* pde_ptr(uint32_t vaddr);

/*以下都是通过位次进行定义，修改属性时候执行或操作*/
#define	 PG_P_1	  1	// 页表项或页目录项存在属性位
#define	 PG_P_0	  0	// 页表项或页目录项存在属性位
#define	 PG_RW_R  0	// R/W 属性位值, 读/执行
#define	 PG_RW_W  2	// R/W 属性位值, 读/写/执行
#define	 PG_US_S  0	// U/S 属性位值, 系统级
#define	 PG_US_U  4	// U/S 属性位值, 用户级
#endif