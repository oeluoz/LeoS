#include "./../include/stdint.h"
#include "./../include/interrupt.h"
#include "./../include/global.h"
#include "./../include/printk.h"
#include "./../include/io.h"

#define PIC_M_CTRL 0x20	       // 这里用的可编程中断控制器是8259A,主片的控制端口是0x20
#define PIC_M_DATA 0x21	       // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0	       // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1	       // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21	 // 目前总共支持的中断数

/*中断门描述符*/
struct  gate_desc
{
    uint16_t offsetHigh16;
    uint8_t attribute;
    uint8_t dcount; //4th B默认值
    uint16_t selector;
    uint16_t offsetLow16;
};
// 静态函数声明,非必须
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];   // idt是中断描述符表,本质上就是个中断门描述符数组

extern intr_handler intr_entry_table[20];	 //数组中存放的是指针 void *类型 

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
    p_gdesc->offsetLow16=((uint32_t)function & 0xFFFF0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
   int i;
   for (i = 0; i < IDT_DESC_CNT; i++) {
      make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]); 
   }
   println("idt_desc_init done\n");
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
   println("idt_init start\n");
   idt_desc_init();	   // 初始化中断描述符表
   pic_init();		   // 初始化8259A

   /* 加载idt */
   uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
   asm volatile("lidt %0" : : "m" (idt_operand));
   
   println("idt_init done\n");

   // 之前测试在这里设置了断点
   println("break\n");
   println("break\n");
   println("break\n");
   println("break\n");
   println("break\n");
   // while(1);

}
// 中断第二次测试：没有执行kernel中的内容