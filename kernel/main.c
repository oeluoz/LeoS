#include"./../include/printk.h"
#include"./../include/init.h"
#include "./../include/debug.h"
#include "./../include/memory.h"
#include "./../include/thread.h"
#include "./../include/interrupt.h"

void k_thread_a(void* arg);
void k_thread_b(void* arg);

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

    /*这里是申请内核page的函数，在多线程第一次测试中注释掉*/
    //while(1);
    // void * addr = get_kernel_pages(3);
    // println("\nget kernel pages start vaddr is: ");
    // printint((uint32_t)addr);

    print('\n');
    println("This is thread test!!!\n");
    println("This is thread test!!!\n");
    println("This is thread test!!!\n");


    thread_start("k_thread_a",8,k_thread_a,"argA ");
    /*现在值测试主线程，其他线程被注释掉*/
    thread_start("k_thread_b",31,k_thread_a,"argB ");
   
   
    // while(1);
    // asm volatile("sti;");    /*打开中断之后就会执行kernel.s在里面初始化中断函数*/
    //                             /*在8259A已经通过IMR将除时钟之外的所有外部中断都屏蔽了，开启中断之后，处理器收到来自时钟的中断*/
    intr_enable();
    while(1){
         println("Main");
    }
    return 0;
} 

/*线程中运行的测试函数*/
void k_thread_a(void* arg) {
    char* para =(char*)arg;
    while(1) {
        println(para);
    }
}

/*线程中运行的测试函数*/
void k_thread_b(void* arg) {
    char* para =(char*)arg;
    while(1) {
        println(para);
    }
}



// 中断第一次测试0x0008:0xc0001773位置出现死循环，代码中并没有设置死循环 jmp $ 不知道为什么会导致这个错误
// 线程第一次测试：在关闭中断的情况下，执行完线程开启函数之后，出现6号中断，操作码错误，怀疑是线程部分执行了错误代码
// 在线程第一次测试中没有出现申请空间错误的问题

// 多线程第一次测试，进入了thread_start函数但是线程没有正常运行，出现的问题malloc申请的一页显示对应的pte已经被使用
// 多线程第二次测试，在start_thead()函数中，get_kernel_pags(1)增加while(1)申请空间没有问题，能否说明是其他线程申请空间出现的问题

//多线程第三次测试，注释掉其他线程，主线程正常运行，说明线程调度正常

//时钟函数没有正常运行，感觉是因为设置栈指针的问题，在拿个地方设置栈指针时出现了问题，导致kernel.s运行出现了问题
//对kernel.s的运行机制不是非常清楚，在idt每一个地方都定义了一个宏展开，对应中断产生的时候执行相应的宏

//线程第四次测试，不运行线程相关的代码，测试中断能否正常执行，测试结果，中断函数根本没有运行：确认中断已经打开

//与时钟中断相关的代码是kerne.s和interrupt.c 
//在kernel.s的call 代码后面打断点会出现问题


//对中断函数interrupt.c进行测试，注册函数如果编译会出现异常中断的问题，如果不编译相应的函数正常响应中断


//多线程第五次测试，GP异常时正常情况，Main线程能够正常执行相应的内容，我觉得导致GP E的问题可能是输出函数，输出了错误的值