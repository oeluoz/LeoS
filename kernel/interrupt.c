#include "./../include/stdint.h"
#include "./../include/interrupt.h"
#include "./../include/global.h"
#include "./../include/printk.h"
#include "./../include/io.h"
#include "./../include/thread.h"
#include "./../include/debug.h"

#define PIC_M_CTRL 0x20	       // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21	       // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0	       // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1	       // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21	 // 目前总共支持的中断数

#define EFLAGS_IF 0x00000200  //eflags中断位置if为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl ; popl %0" : "=g"(EFLAG_VAR)) //读取eflags寄存器 define symbol需要紧跟括号，不然会编译错误

/*中断门描述符*/
struct  gate_desc
{
   uint16_t offsetLow16;
   uint16_t selector;
   uint8_t dcount; //4th B默认值
   uint8_t attribute; 
   uint16_t offsetHigh16;
};
//静态函数声明,非必须
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];   // idt是中断描述符表,本质上就是个中断门描述符数组

//中断函数的入口，最终调用的是idt_table中的处理程序
extern intr_handler intr_entry_table[20];	 //数组中存放的是指针 void *类型 
char * intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];      //

//线程相关
uint32_t ticks; //内核自中断开启

/*初始化可编程中断控制器8259A */
static void pic_init(void) {

   /* 初始化主片 */
   outb (PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
   outb (PIC_M_DATA, 0x04);   // ICW3: IR2接从片. 
   outb (PIC_M_DATA, 0x01);   // ICW4: 8086模式, 正常EOI

   /* 初始化从片 */
   outb (PIC_S_CTRL, 0x11);	// ICW1: 边沿触发,级联8259, 需要ICW4.
   outb (PIC_S_DATA, 0x28);	// ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
   outb (PIC_S_DATA, 0x02);	// ICW3: 设置从片连接到主片的IR2引脚
   outb (PIC_S_DATA, 0x01);	// ICW4: 8086模式, 正常EOI

   /* 打开主片上IR0,也就是目前只接受时钟产生的中断 */
   outb (PIC_M_DATA, 0xfe);
   outb (PIC_S_DATA, 0xff);

   println("pic_init done\n");
}

//创建idt描述符
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) { 
    p_gdesc->offsetLow16=(uint32_t)function&0x0000ffff;
    p_gdesc->selector=SELECTOR_K_CODE;
    p_gdesc->dcount=0;
    p_gdesc->attribute=attr;
    p_gdesc->offsetHigh16=((uint32_t)function & 0xFFFF0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
   for (int i = 0; i < IDT_DESC_CNT; i++) {
      make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]); 
   }
   println("idt_desc_init done\n");
}
/*通用中断处理函数*/
static void general_intr_handler(uint8_t vec_nr) {
   if(vec_nr == 0x27 || vec_nr == 0x2f){
      return ;
   }
   set_cursor(0); //重置光标到屏幕左上角
   int cursor = 0;
   while(cursor<320){   //清空4行页面
      print(' ');
      cursor++;
   }
   set_cursor(0);
   println("-----------Exception---------------\n");
   println("-----------Exception---------------\n");
   println("-----------Exception---------------\n");
   println("-----------Exception---------------\n");
   println("-----------Exception---------------\n");
   
   set_cursor(88); //第二行第8个字符开始打印
   printint(vec_nr);
   println(" ");
   println(intr_name[vec_nr]);
   if(vec_nr == 14) { //若为PageFault，将缺失的地址打印出来并且悬停
      int page_fault_vaddr = 0;
      asm("mov %%cr2,%0":"=r"(page_fault_vaddr)); //cr2存放的是造成page_fault的地址
      println("page fault addr is: ");
      printint(page_fault_vaddr);
   }
   while(1);
}

/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init() {
   int i;
   for (i = 0; i < IDT_DESC_CNT; i++) {

/** 
 *idt_table数组中的函数是在进入中断后根据中断向量号调用的,
 * 见kernel/kernel.S的call [idt_table + %1*4]
 */
      idt_table[i] = general_intr_handler;		    // 默认为general_intr_handler。
							                               // 以后会由register_handler来注册具体处理函数。
      intr_name[i] = "unknown";				          // 先统一赋值为unknown 
   }
   intr_name[0] = "#DE Divide Error";
   intr_name[1] = "#DB Debug Exception";
   intr_name[2] = "NMI Interrupt";
   intr_name[3] = "#BP Breakpoint Exception";
   intr_name[4] = "#OF Overflow Exception";
   intr_name[5] = "#BR BOUND Range Exceeded Exception";
   intr_name[6] = "#UD Invalid Opcode Exception";
   intr_name[7] = "#NM Device Not Available Exception";
   intr_name[8] = "#DF Double Fault Exception";
   intr_name[9] = "Coprocessor Segment Overrun";
   intr_name[10] = "#TS Invalid TSS Exception";
   intr_name[11] = "#NP Segment Not Present";
   intr_name[12] = "#SS Stack Fault Exception";
   intr_name[13] = "#GP General Protection Exception";
   intr_name[14] = "#PF Page-Fault Exception";
   // intr_name[15] 第15项是intel保留项，未使用
   intr_name[16] = "#MF x87 FPU Floating-Point Error";
   intr_name[17] = "#AC Alignment Check Exception";
   intr_name[18] = "#MC Machine-Check Exception";
   intr_name[19] = "#XF SIMD Floating-Point Exception";

}

/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable() {
   enum intr_status old_status;
   if (INTR_ON == intr_get_status()) {
      println("old status on,now we turn it on\n");
      old_status = INTR_ON;
      return old_status;
   } else {
      println("old status off,now we turn it on\n");
      old_status = INTR_OFF;
      asm volatile("sti");
      return old_status;
   }
}
/*关闭中断并返回关闭中断前的状态*/
enum intr_status intr_disable() {
   enum intr_status old_status;
   if(INTR_ON == intr_get_status()) {
      old_status = INTR_ON;
      asm volatile("cli": : : "memory");
      return old_status;
   } else {
      old_status = INTR_OFF;
      return old_status;
   }
}
/*将中断状态设置为status*/
enum intr_status intr_set_status(enum intr_status status) {
   return status & INTR_ON ? intr_enable() : intr_disable();
}
/*获取当前中断状态*/
enum intr_status intr_get_status() {
   uint32_t eflags = 0 ;
   GET_EFLAGS(eflags);
   return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
/*在中断处理数组中第vector_no个元素中注册安装中断处理程序function*/
void register_handler(uint8_t vector_no,intr_handler function) {
   idt_table[vector_no] = function;
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
   println("idt_init start\n");
   idt_desc_init();	   // 初始化中断描述符表
   exception_init();
   pic_init();		   // 初始化8259A

   /* 加载idt */
   //--------------------------
   //| 32bit表基址|16bit表界限 |
   //--------------------------
   //idt指针需要先转换成整数，指针只能先转换成相同bit的整数，转成整数然后继续转成64bit
   uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
   asm volatile("lidt %0;" : : "m" (idt_operand)); //idtr在这里指向了idt数组
   
   println("idt_init done\n");


}
// 中断第二次测试：没有执行kernel中的内容，错误原因1：中断描述符没有按照指定格式
// 错误原因2：中断描述符low 16bits初始化错误
// 中断第三次测试，现在的内核代码已经超出20k，在interrupt.c中增加内容导致扇区写入出错，将扇区增加到
// 200(100KB)，内核代码不会超过100KB