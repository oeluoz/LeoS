#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "stdint.h"
typedef void* intr_handler;
void idt_init(void);

#endif