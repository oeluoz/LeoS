#include "./../include/memory.h"
#include "./../include/stdint.h"
#include "./../include/printk.h"
#include "./../include/bitmap.h"
#include "./../include/string.h"
#include "./../include/debug.h"
#include "./../include/global.h"

#include <stdio.h>

#define PG_SIZE 4096

/**
0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb
4KB位图可以表示4K*8*4KB=128MB内存空间，位图放置在0xc009a000
最大支持4个page大小位图，即为512MB
*/
/**
|-------------------------------|0x102000 分别放置页目录表和页表
|-------------------------------|0x100000 1MB内存的高端地址

            ...
|-------------------------------|0x9fc00 可用内存的高端地址
|-------------------------------|0x9f000 
|-------------------------------|0x9e000 PCB起始地址(PCB大小为4K)
|                               |
|                               |
|                               |
|                               |
|-------------------------------|0x9e000-4k(0x9a000) bitmap起始地址(四个页框，最大支持512MB内存)
|                               |
                ...
|-------------------------------|0x7e00 可用内存的低端地址
|                               |
|                               |
|                               |
|-------------------------------|0x1500 内核代码的加载地址

*/
#define MEM_BITMAP_BASE 0xc009a000      //0xc009e000-0x4000
#define KERNEL_HEAP_START  0xc0100000
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22) //获取高10bit PDT索引、
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12) //获取中间10bit PT索引
/**
内核从3G开始，低端1MB存放内核代码，所以地址从
0xc0100000开始分配
*/
struct pool{
    struct bitmap pool_bitmap;  //用来管理物理内存的位图
    uint32_t phy_addr_start;    //内存池管理的物理内存的起始地址
    uint32_t pool_size;         //内存池字节容量
};

struct pool kernel_pool,user_pool;     // 内核内存池和用户内存池
struct virtual_addr kernel_vaddr;        // 用来给内核分配虚拟地址

/**
在pf表示的虚拟内存池中申请pg_cnt个虚拟页
成功则返回page的起始地址，失败则返回NULL
*/
static void * vaddr_get(enum pool_flags pf,uint32_t pg_cnt) {
    int vaddr_start = 0,bit_idx_start = -1;
    uint32_t cnt = 0; 
    if(pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap,pg_cnt);
        if(bit_idx_start == -1) {
            println("Here in vaddr_get i find there is no free memory\n");
            return NULL;
        }
        while(cnt < pg_cnt) {
            bitmap_set (&kernel_vaddr.vaddr_bitmap,bit_idx_start + cnt++,1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    } else {
        //用户内存池
    }
    return (void*)vaddr_start; //从内核堆的起始地址+占用内存(bit_idx_start*PG_SIZE) 即为分配可用内存的起始地址
}


/**
*以下两个函数的实际意义是：给我一个虚拟地址，返回给我一个
*新的虚拟地址，我可以通过这个虚拟地址找到对应的pte和pde
*即让新的虚拟地址指向pde以及pte所在的物理地址
*/

/**
得到虚拟地址vaddr对应的pte指针
*/
uint32_t* pte_ptr(uint32_t vaddr) {
    /*
    首先访问到页表自己，再用页目录项pde作为pte的索引访问到页表，再用pte
    的索引作为页内偏移
    */
    /*0xffc访问最后一个页目录项，最后一个页目录项指向的是页目录表自己这个页表，也就是最后一个PDE是 base PDT
    用虚拟地址的高10bit能够找到页目录项中对应的页目录项，找到对应的页表，然后用中间10bit作为页表偏移，找到PTE对应的内存位置

    目标：页表基址+偏移量
    偏移量用：虚拟地址中间10位表示
    页表基址：第一次基址跑到PDT位置，第二次处理器直接认为是在寻找PTE了，实际上寻找的是页表的基址，这时候通过高10bit作为页目录表中的偏移
    取出对应位置的页表基址+偏移量即为PTE的内存位置

    问题：为什么还需要首先找到PDT的基址，不应该是一个寄存器将基址存放起来的吗？，如果没有寄存器将基址存放起来，那么其他的通过高10bit找到对应的
    页目录项也是通过这种方式？
    )
    */
    uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000)>>10) + PTE_IDX(vaddr)*4 );
    return pte;
}
/**
得到虚拟地址vaddr对应的pde指针
*/
uint32_t* pde_ptr(uint32_t vaddr) {
    //PDT 1023 指向的是PDT自己，高10bit全为1首先得到PDT的基址，然后将寻找到的页目录表当做页表，中间10bit作为索引，索引全为1寻找到的是最后一项，即为
    //页目录表的基址，加上页目录项的偏移即为PDE对应的内存位置
    uint32_t*pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr)*4);
}

/**
*在m_pool指向的物理内存池中分配1个物理页
*成功则返回页框的地址，否则返回NULL
*/
static void* palloc(struct pool*m_pool) {
    int bit_idx=bitmap_scan(&m_pool->pool_bitmap,1);
    if(bit_idx == -1){
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap,bit_idx,1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *) page_phyaddr;
}

/**
*页表中增加虚拟地址_vaddr与物理地址_page_phyaddr的映射
*/
static void page_table_add(void*_vaddr,void*_page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr;
    uint32_t page_phyaddr = (uint32_t) _page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

    /*
      页目录内判断页目录项的P位，若为1，则表示该表已经存在
      *pte的过程实际是对pte进行寻址的操作，寻址会首先用到一级页表中的pde
      因此pde不存在需要首先创建pde
    */
    if(*pde & 0x00000001) {
        ASSERT(!(*pte & 0x00000001)); //页表项已经存在
        if(!(*pte & 0x00000001))  {
            *pte = (page_phyaddr | PG_US_U | PG_RW_W |PG_P_1);
        }else { //上面ASSERT断言导致不会执行到这里
            PANIC("pte already exist!");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W |PG_P_1);
        }
    } else {
        //页目录项不存在的情况下需要先创建页目录项，再创建页表项
        uint32_t pde_phyaddr = (uint32_t) palloc(&kernel_pool);
        /*页表用到的页框从内核空间分配*/
        *pde = (pde_phyaddr|PG_US_U|PG_RW_W|PG_P_1);

        /*将pte对应的内存空间清空，为了到达指定的内存地址，只需要取高20bit即可*/
        memset((void *)((int)pte & 0xfffff000),0,PG_SIZE);
        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr|PG_US_U|PG_RW_W|PG_P_1);
    }
}

/**
*分配pg_cnt个页空间，成功则返回起始虚拟地址，失败则返回NULL
*/
void * malloc_page(enum pool_flags pf,uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt <3480);
    /* 1.通过vaddr_get在虚拟内存池中申请虚拟地址
       2.通过palloc在物理内存池中申请物理页
       3.通过page_table_add 将以上得到的虚拟地址和物理地址在页表中完成映射
    */

    void * vaddr_start = vaddr_get(pf,pg_cnt);
    // 内存第一次调试
    // println("In malloc_page()!After vaddr_get()!: ");
    // printint((uint32_t)vaddr_start);
    // print('\n');
    if(vaddr_start == NULL){
        return NULL;
    }
    uint32_t vaddr = (uint32_t) vaddr_start;
    uint32_t cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    /*虚拟地址连续，但是物理地址不连续，因此需要逐个做映射*/

    while (cnt-- > 0){
        void * page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL) {
            return NULL;
        }
        page_table_add((void*)vaddr,page_phyaddr);
        vaddr+=PG_SIZE; /*地址不能直接加法运算，所以先转换成32位整数*/
    }
    return vaddr_start;
}

/**
*从内核内存池中申请pg_cnt页内存，成功则返回其虚拟地址，失败则返回NULL
*/
void * get_kernel_pages(uint32_t pg_cnt) {
    void * vaddr = malloc_page(PF_KERNEL,pg_cnt);
    if(vaddr != NULL) {
        memset(vaddr,0,pg_cnt * PG_SIZE);
    }
    return vaddr;
}

/**
初始化内存池,all_mem通过e820读取硬件得到
初始化了 
kernel pool
user pool
kernel_vaddr
三个内存池
*/
static void mem_pool_init(uint32_t all_mem) {
    println("\ninit memory pool\n");

    /*计算已经使用的内存*/
    uint32_t page_table_size = PG_SIZE *256;
    // 页表大小 = 1页的页目录表 + 第 0和第 768个页目录项指向同一个页表  
    // 第 769～1022个页目录项共指向 254个页表，共 256个页框，PDT最后一项指向
    //PDT，不重复计算
    uint32_t used_mem = page_table_size+0x100000;
    uint32_t free_mem = all_mem - used_mem;
    uint32_t all_free_pages = free_mem / PG_SIZE; //只考虑4K大小的内存，少于4k内存忽略

    /*内核和用于可用page*/
    uint32_t kernel_free_pages = all_free_pages /2;
    uint32_t user_free_pages = all_free_pages -kernel_free_pages;

    /*内核和用户bitmap*/
    uint32_t kernel_bitmap_length = kernel_free_pages / 8;
    uint32_t user_bitmap_length = user_free_pages / 8;

    /*内核内存池和用户内存池起始位置*/
    uint32_t kernel_pool_start = used_mem;
    uint32_t user_pool_start = kernel_pool_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kernel_pool_start;
    user_pool.phy_addr_start = user_pool_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE ;

    kernel_pool.pool_bitmap.btmp_bytes_len = kernel_bitmap_length;
    user_pool.pool_bitmap.btmp_bytes_len = user_bitmap_length;
    
    //内核内存池的位图先定在0xc009a000位置
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;
    
    //用户内存池的位图放置内核内存池之后
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kernel_bitmap_length);

    println("kernel pool bitmap start at: ");
    printint((int)(kernel_pool.pool_bitmap.bits));
    print('\n');
    println("kernel pool physical address start at: ");
    printint((int)kernel_pool.phy_addr_start);
    print('\n');
    println("user pool bitmap start at: ");
    printint((int)(user_pool.pool_bitmap.bits));
    print('\n');
    println("user pool physical address start at: ");
    printint((int)user_pool.phy_addr_start);
    print('\n');

    /*位图置零*/
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /*初始化内核虚拟地址的位图，按实际物理地址大小生成数组*/
    //用来维护内核堆的虚拟地址，需要和内核内存池大小一致
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kernel_bitmap_length ;

    /*位图的数组指向一块未使用的内存
    当前定位在内核内存池和用户内存池之外*/
    kernel_vaddr.vaddr_bitmap.bits = \
    (void *)(MEM_BITMAP_BASE + kernel_bitmap_length +user_bitmap_length);

    kernel_vaddr.vaddr_start = KERNEL_HEAP_START;

    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    println("memmory pool init done!\n");
}

/*内存管理部分初始化入口*/
void mem_init(){
    println("mem init start!\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0x983));
    println("mem bytes total: ");
    printint(mem_bytes_total);
    mem_pool_init(mem_bytes_total);
    // print('\n');
    println("mem init done!\n");

}


//内存第一次测试：位图已经初始化，但是出现bitmap scan的时候返回是空，初步判断是因为位图没有初始化成功，问题出现在bitmap的初始化
//问题，初始化应该在bitmap->bits对应的内存空间，实际上初始化成了bitmap对应的内存空间，导致位图没有初始化成功；
//在执行elephantOS的代码时，可能由于初始化内存还没有分配导致page fault
