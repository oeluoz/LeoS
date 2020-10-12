#include "./../include/thread.h"
#include "./../include/stdint.h"
#include "./../include/string.h"
#include "./../include/global.h"
#include "./../include/memory.h"
#include "./../include/list.h"
#include "./../include/debug.h"
#include "./../include/interrupt.h"
#include "./../include/printk.h"

#define PG_SIZE 4096

struct task_struct* main_thread;    //主线程PCB
struct List thread_ready_list;  //就绪队列
struct List thread_all_list; //所有任务的队列，队列中存放的不是线程的pcb，而是pcb的tag
static struct ListNode* thread_tag; //用来保存队列中的线程节点
//线程的切换依赖于switch_to函数的执行
extern void switch_to(struct task_struct*cur,struct task_struct* next);

/*kernel_thread去执行function(func_arg)*/
static void kernel_thread(thread_func* function, void* func_arg) {
    /*执行function之前需要开中断，避免后面的时钟中断被屏蔽，
    让function独占处理器，无法调度其他线程*/
    intr_enable();
    function(func_arg);
}

/*获取当前线程的pcb指针*/
struct task_struct* running_thread() {
    /*线程的所用的0级栈都是在自己的PCB当中，PCB大小为4K，
      因此高20都是相同的，高20位是PCB起始的地址
      PCB如果不是从0开始的，这样起始点就不正确，但是PCB是4KB对齐的
    */
    uint32_t esp;
    asm ("mov %%esp,%0":"=g"(esp));     //=约束为只读 g 允许任一寄存器、内存或者立即整形操作数，不包括通用寄存器之外的寄存器。
    return (struct task_struct*)(esp & 0xfffff000);
}

/*初始化线程栈thread_stack，将待执行的函数和参数放到thread_stack中相应的位置*/

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
    /*预留中断使用栈的空间*/
    pthread->self_kstack -= sizeof(struct intr_stack); 
    //线程进入中断，kernel.s的中断代码通过这个栈保存上下文，此外用户进程的信息也会放置在中断栈中

    /*预留线程栈空间*/
    pthread->self_kstack -= sizeof(struct thread_stack);

    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    //调用函数kernel_thread函数参数以及函数地址
    kthread_stack->eip = kernel_thread;         //函数指针eip指向kernel_thread对应的偏移地址
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;

    kthread_stack->ebp = kthread_stack->ebx = \
    kthread_stack->esi = kthread_stack->edi = 0;
}

/*初始化线程基本信息*/

void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name,name);
    if(pthread == main_thread) {
        pthread->status = TASK_RUNNING; 
        //main函数被封装成一个线程，并且一直运行，因此将其设置为TASK_RUNNING状态
    } else {
        pthread->status = TASK_READY;
    }

    /*self_stack是线程自己在内核态下使用的栈顶地址，在这里指定了栈顶地址，之后才能
    在thread_create中未中断和线程栈预空间*/
    pthread->self_kstack = (uint32_t *)((uint32_t)pthread + PG_SIZE);   //地址只能转换成整数之后才能加减，然后将整数转换成相应的地址

    //不理解为什么栈在这个位置 pthread分配的是一页内存，用作PCB，get_kernel_pages(1)
    //返回的是低地址，因此+PG_SIZE相当于指向PCB的顶端

    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL; //线程没有页表，进程pgdir指向的是进程页表的虚拟地址
    pthread->stack_magic = 0x12345678;
} 

/*创建一个优先级为prio的线程，name，执行函数为function(func_arg)*/

struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg ) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    /*确保之前不在队列中*/
    ASSERT(!elem_find(&thread_ready_list,&thread->general_tag));
    /*加入就绪队列*/
    list_append(&thread_ready_list,&thread->general_tag);

    /*确保之前不在队列中*/
    ASSERT(!elem_find(&thread_all_list,&thread->all_list_tag));
    /*加入全部任务队列*/
    list_append(&thread_all_list,&thread->all_list_tag);

    // asm volatile ("movl %0,%%esp;\
    // pop %%ebp;pop %%ebx;pop %%edi;pop %%esi;\
    // ret" : : "g"(thread->self_kstack) : "memory");

    //这几条语句实际上被综合在了switch_to函数里面，next为首次使用时候，栈中存放的是kernel_thread函数指针
    //因此跳转到kernel_thread函数执行

    //mov esp,[eax] esp指向了next对应的位置


    //一个进程时的测试，通过ret执行


    //movl %0,%%esp %0是thread->self_kstack的占位符，
    //因此这条命令是将thread->self_kstack的值作为栈顶

    //pop指令的说明，在thread_create函数中已经将esp减了对应栈的大小，所以
    //现在esp指向的是线程栈的栈顶，线程栈的低地址到高地址
    //依次为ebp ebx edi esi (*eip) unused_retaddr function func_arg

    //之后执行的ret指令从内核栈中弹出 kernel_thread对应的地址，跳转到对应位置去
    //执行

    return thread;
}

/*将kernel中的main函数完善为主线程*/
static void make_main_thread() {
    /*
    main线程已经运行，在setup.s进入内核时候mov esp,0xc009f000
    为pcb预留了4KB空间，pcb地址为0xc009e000，不需要你从内核内存中申请
    新的一页，有点奇怪，栈向下生长，这样就将pcb对应的内存空间破坏了    
    */ 
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);  //会不会出现将之前的栈破坏的情况？
    //现在main线程的esp在什么位置，一开始设置的esp是不是在pcb的低端？main线程的栈顶设置在PCB的顶端，esp仍然指向的是
    //原来的位置，
    println("Here in make_main_thread()\n");
    ASSERT(!elem_find(&thread_all_list,&main_thread->all_list_tag));
    list_append(&thread_all_list,&main_thread->all_list_tag);
    //为什么没有将main线程添加到thread_ready_list，在进行调度的时候，有TASK_RUNNING转变为TASK_READY状态，并将main线程添加到ready_list
    println("Add main thread to ready list \n");
}

/*调度器的实现*/
void schedule() {
    // println("Here I am in schedule()\n");

    ASSERT(intr_get_status() == INTR_OFF); //陷入中断的时候进程调度，此时中断被关闭
    struct task_struct*current=running_thread();
    //当前有进程正在执行，将其从CPU上换下来
    if(current->status == TASK_RUNNING){
        ASSERT(!elem_find(&thread_ready_list,&current->general_tag));
        list_append(&thread_ready_list,&current->general_tag);
        //重置时间片
        current->ticks=current->priority;
        current->status=TASK_READY;
    } else {
       
    }
    
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag=NULL; //thread_tag清空
    /*从就绪队列中pop出来一个就绪线程，它只是一个general_tag
    或者all_list_tag，需要转换成PCB才行
    */
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next=elem2entry(struct task_struct,\
    general_tag,thread_tag);
    //thread_tag对应的地址-结构体中general_tag对应的偏移量
    next->status = TASK_RUNNING;
    switch_to(current,next);

}

/*初始化线程环境*/
void thread_init() {
    println("thread init!\n");
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    /*将main函数创建为线程*/
    make_main_thread();
    println("thread init done!\n");
}

/*将节点元素转换成实际元素
#define offset(struct_type,member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type,struct_member_name,elem_ptr) \
        (struct_type*)((int)elem_ptr-offset(struct_type,struct_member_name))
*/
/*
struct_type:elem_ptr所在结构体的类型
struct_member_name: elem_ptr所在结构体中对应地址的成员名字
elem_ptr:待转换的地址

作用:将elem_ptr指针转换成struct_type对应类型的指针

原理:elem_ptr的地址-elem_ptr在结构体struct_type中的偏移量
地址差即为结构体struct_type的起始地址

这种转换非常巧妙。通过结构体中的一个扣子将结构体串起来
这样链表占用的空间就非常小，与此同时还可以通过这个扣子
找到
*/

  
//注意task_struct即为进程控制块pcb
//esp一开始指向0xc009f000是有原因的，0xc009e000是PCB的起点，4k的终点是
//0xc009f000，与栈指针指向PCB的高地址相符合


// 线程第一次测试，在对主线程进行初始化时候，没有初始化stack_magic魔数
