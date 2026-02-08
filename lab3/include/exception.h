#ifndef EXCEPTION_H
#define EXCEPTION_H
#define CORE0_IRQ_SOURCE       ((volatile unsigned int*)(0x40000060))
#define IRQ_PENDING_1          ((volatile unsigned int*)(0x3F00B204))
#define IRQ_PENDING_2          ((volatile unsigned int*)(0x3F00B208))
#endif