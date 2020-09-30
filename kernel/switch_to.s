 [bits 32]
 section .text
 global switch_to
 switch_to:
    ; 栈中此处是返回地址
    push esi
    push edi
    push ebx
    push ebp

    mov eax, [esp + 20] ;四个保护的寄存器+一个返回地址，因此参数current在[esp+20]
    mov [eax],esp   ;保存栈顶指针esp task_struct的self_kstack字段
                    ;self_kstack在task_struct中的偏移量为0
                    ;
                    
                    ;这里有一点奇怪，这样的话esp是保存在在了PCB的低地址，那里应该
                    ;空闲的区域，这里的理解出现问题，内核站是在结构体的低地址，
                    ;怎样和最先的将栈顶指针质量PCB的高端地址处对应
                    ;解释：kstack是一个指针，指向的是PCB的高地址，那里预留下了
                    ;
                    
                    ;将esp存在[eax]内存地址，相当于就是将kstack的内存空间覆盖了

                    ;这样其实是没有问题的，因为kstack存放的本来就是一个栈顶指针
    ;以上是备份当前线程环境，下面是恢复下一个线程的环境

    ;由于esp指向next对应的栈空间，此时从里面pop
    ;出来的寄存器已经不是上面入栈的那些寄存器
    mov eax,[esp + 24] ;参数next所在的地址
    mov esp,[eax]      ;pcb的第一个成员是self_kstack，它用来记录0级栈顶指针
                       ;被换上CPU时用来恢复0级栈 0级栈中保存了进程或者线程的所有信息
                       ;包括3级栈指针
   pop ebp
   pop ebx
   pop edi
   pop esi
   ret                 ;返回switch_to下面注释位置的返回地址，这里是next线程入栈的返回地址
                       ;未由中断进入，第一次执行时会返回到kernel_thread

    
;这一节里面很多关于PCB里面栈的存放和转换有很多不明白的地方，需要重新看代码，画图理解 