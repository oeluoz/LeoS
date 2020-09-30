#include "./../include/timer.h"
#include "./../include/io.h"
#include "./../include/printk.h"
#include "./../include/thread.h"
#include "./../include/interrupt.h"
#include "./../include/debug.h"

#define IRQ0_FREQUENCY	   100
#define INPUT_FREQUENCY	   1193180
#define COUNTER0_VALUE	   INPUT_FREQUENCY / IRQ0_FREQUENCY
#define CONTRER0_PORT	   0x40
#define COUNTER0_NO	   0
#define COUNTER_MODE	   2
#define READ_WRITE_LATCH   3
#define PIT_CONTROL_PORT   0x43

uint32_t ticks; //内核自中断开启依赖总共的时钟周期
/* 把操作的计数器counter_no、读写锁属性rwl、计数器模式counter_mode写入模式控制寄存器并赋予初始值counter_value */
static void frequency_set(uint8_t counter_port, \
			  uint8_t counter_no, \
			  uint8_t rwl, \
			  uint8_t counter_mode, \
			  uint16_t counter_value) {
/* 往控制字寄存器端口0x43中写入控制字 */
   outb(PIT_CONTROL_PORT, (uint8_t)(counter_no << 6 | rwl << 4 | counter_mode << 1));
/* 先写入counter_value的低8位 */
   outb(counter_port, (uint8_t)counter_value);
/* 再写入counter_value的高8位 */
   outb(counter_port, (uint8_t)counter_value >> 8);
}

/*时间中断处理函数*/
static void intr_timer_handleer() {
   struct task_struct* cur_thread = running_thread();
   ASSERT(cur_thread->stack_magic == 0x12345678); //检查栈是否溢出
   cur_thread->elapsed_ticks++; //已经使用的时间片++
   ticks++; //从内核第一次处理时间中断至今的时间中断数，内核态和用户态总共的滴答数
   if(cur_thread->ticks == 0) {
      schedule();
   } else {
      cur_thread->ticks--;
   }
   // printint(cur_thread->ticks);
   // print('\n');
}
/* 初始化PIT8253 */
void timer_init() {
   println("timer init start\n");
   /* 设置8253的定时周期,也就是发中断的周期*/
   frequency_set(CONTRER0_PORT, COUNTER0_NO, READ_WRITE_LATCH, COUNTER_MODE, COUNTER0_VALUE);
   register_handler(0x20,intr_timer_handleer);
   println("timer init done\n");
}
