#include "./../include/debug.h"
#include "./../include/printk.h"
#include "./../include/interrupt.h"

/*输出文件名 行号 函数名 条件使得程序悬停*/
void panic_spin(char *filename,int line, const char*func,const char *condition) {
    intr_disable();
    println("\n\n\n\nERROR!\n\n\n");
    println("filename : ");println(filename);print('\n');
    println("line : 0x");printint(line);print('\n');
    println("func : ");println(func);print('\n');
    println("condition : ");println((char *)condition); print('\n');
    while(1);
}