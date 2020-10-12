[bits 32]
%define ERROR_CODE nop	 ; 若在相关的异常中cpu已经自动压入了错误码,为保持栈中格式统一,这里不做操作.
%define ZERO push 0		 ; 若在相关的异常中cpu没有压入错误码,为了统一栈中格式,就手工压入一个0

extern println			 ;声明外部函数
extern idt_table      ;在interrupt.c中初始化的中断处理函数

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table
intr_entry_table:
; jmp $
; jmp $
; jmp $

%macro VECTOR 2
section .text
intr%1entry:		 ; 每个中断处理程序都要压入中断向量号，
                   ; 所以一个中断类型一个中断处理程序，
                   ; 自己知道自己的中断向量号是多少
   %2              ;中断若有错误码会压在eip后面，根据参数展开为nop或者push 0 指令
   ; mov byte [gs:160],'p'
   ; mov byte [gs:162],'p'
   ; mov byte [gs:164],'p'
   ; mov byte [0xb8000],'A'
   ; jmp $
   ;保存上下文环境
   push ds
   push es
   push fs
   push gs
   pushad          ;EAX ECX EDX EBX ESP EBP ESI EDI
   
   ; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI 
   mov al,0x20                   ; 中断结束命令EOI
   out 0xa0,al                   ; 向从片发送
   out 0x20,al                   ; 向主片发送

   push %1                       ;不管idt_table中的目标程序是否需要参数
                                 ;一律push 中断向量号

   ; 时钟中断导致原来的执行流被改变，因此需要将原来的寄存器进行压栈保存，当call进入到调度程序
   ; 里面再一次调用switch_to函数，需要将switch_to之前的寄存器进行压栈保存，执行一次任务切换需要两次保护现场
   
   call [idt_table + %1*4]
   
   jmp intr_exit

section .data
   dd    intr%1entry	 ; 存储各个中断入口程序的地址，形成intr_entry_table数组
%endmacro

section .text
global intr_exit

intr_exit:
   add esp,4                     ;跳过中断号
   popad
   pop gs
   pop fs
   pop es
   pop ds
   add esp,4                     ;跳过error_code
   iretd
   
;以下是宏调用，
VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO
