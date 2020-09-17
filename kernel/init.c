#include "./../include/printk.h"
#include "./../include/interrupt.h"
#include "./../include/timer.h"
#include "./../include/memory.h"

void init_all(){
    println("init all start! \n");
    idt_init();
    timer_init();
    mem_init();
}