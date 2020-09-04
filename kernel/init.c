#include "./../include/printk.h"
#include "./../include/interrupt.h"

void init_all(){
    println("init all \n");
    idt_init();
}