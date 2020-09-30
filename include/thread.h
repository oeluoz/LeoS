#ifndef __THREAD_H__
#define __THREAD_H__

#include "stdint.h"
#include "list.h"
/*通用函数类型，在线程函数中作为形参类型*/
typedef void thread_func(void *);

/*进程或者线程的状态*/
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/**中断栈
*用来保存中断发生时的上下文环境
*进程或者线程被外部中断或者线程中断，按照这个结构顺序入栈
*栈的位置固定，放置在页的最顶端

*进入中断后，kernel.s中的中断入口程序 intr%entry 所执行的
*上下文保护的一系列操作都是压入这个结构
*
*初始时在页的最顶端，每次进入中断就会发生变化
*中断不涉及特权级的变化，它的位置就会在当前的esp之下，否则处理器会从
*TSS中获得新的esp的值，然后栈在新的esp之下
*/

struct intr_stack {
    uint32_t vec_no;	 // kernel.S 宏VECTOR中push %1压入的中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;	 // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32_t ebx;             
    uint32_t edx;
    uint32_t ecx;  
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /*以下由cpu从低特权级进入高特权级时压入*/
    uint32_t err_code;      //err_code会被压入在eip之后
    void (*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/**线程栈
*线程自己的栈，用于存储线程中待执行的函数
*线程栈在自己的内核栈中位置不固定，仅用在switch_to时保存线程环境。
*实际位置取决于实际运行环境
*/

struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;
    /*线程第一次执行时，eip指向待调用的函数kernel_thread
      其他时候eip指向switch_to的返回地址*/
    void (*eip) (thread_func* func,void*func_arg); // (*eip)是一个函数指针，指向的函数 返回值是void 参数是 thread_func * 和 void*

    /*以下仅供第一次被调度上CPU时使用*/

    /*参数unused_ret只为占位置充数为返回地址*/
    void (*unused_retaddr);
    thread_func* function;  //kernel_thread所调用的函数名
    void* func_arg;         //kernel_thread所调用的函数所需的参数

    //在线程中调用的是func(func_arg)
};

/*进程或者线程的pcb*/
struct task_struct {
    uint32_t* self_kstack;  //内核线程自己的内核栈
    enum task_status status;
    uint8_t priority;   //线程优先级
    char name[16];

    uint8_t ticks;  //每次在处理器上执行的时钟数量
    uint32_t elapsed_ticks; //进程被送上cpu已经执行的时钟数

    struct ListNode general_tag ; //用于线程在一般队列中的节点
    struct ListNode all_list_tag; //用于线程在thread_all_list队列中的节点

    /*以上来个tag是PCB被加入队列的一个基础，
    实现的方式是将对应的tag加入到就绪等待队列
    有这两个tag才能实现pcb形成一个调度队列
    */
    uint32_t* pgdir;    //进程自己页表的虚拟地址

    uint32_t stack_magic; //栈的边界标记，用来检测栈的溢出
};

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, char* name, int prio);
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);
struct task_struct* running_thread(void);
void schedule(void);
void thread_init(void);
struct task_struct* running_thread();
#endif
