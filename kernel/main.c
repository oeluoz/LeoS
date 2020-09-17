#include"./../include/printk.h"
#include"./../include/init.h"
#include "./../include/debug.h"
#include "./../include/memory.h"
int main(void ){
    // int i;
    // print('\r');
    // print('W'); 
    // print('e'); 
    // print('l'); 
    // print('c'); 
    // print('o'); 
    // print('m'); 
    // print('e'); 
    // print(' '); 
    // print('T'); 
    // print('o'); 
    // print(' '); 
    // print('L');
    // print('e');
    // print('o');
    // print('S');
    // print('!');
    // print('\n');
    // println("\n\n\n\n\n\n\n\n\n\n\n");
    // for(i=0;i<10;i++){
    //     printint(i);
    //     print(' ');
    //     println("Welcome To LeoS!\n");
    // }
    // println("Welcome To LeoS!");
    // print('\r');
    // printint(0x1234);
    // print('\n');
     println("\nThis is kernel!\n"); 
    // print('\n');
    // print('\n');
    // print('\n');
    // println("\n\n\n\n\n\n");
    init_all();
    ASSERT(1==1); //用来测试ASSERT函数
    println("After init_all()\n");
    //while(1);
    void * addr = get_kernel_pages(3);
    println("\nget kernel pages start vaddr is: ");
    printint((uint32_t)addr);
    print('\n');
    while(1);
    asm volatile("cli;");    /*打开中断之后就会执行kernel.s在里面初始化中断函数*/
                                /*在8259A已经通过IMR将除时钟之外的所有外部中断都屏蔽了，开启中断之后，处理器收到来自时钟的中断*/
    println("\nafter clock interrupt!\n");
    while(1); 
    return 0;
} 
// 中断第一次测试0x0008:0xc0001773位置出现死循环，代码中并没有设置死循环 jmp $ 不知道为什么会导致这个错误