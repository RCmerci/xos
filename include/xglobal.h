#ifndef _KERNEL_GLOBAL
#define _KERNEL_GLOBAL

#include "xtype.h"
#include "xconst.h"

//extern GDTPTR gdt_ptr;
extern u8 gdt_ptr[6];
extern DESCRIPTOR gdt[GDT_DESC_NUM];
extern u8 idt_ptr[6];
extern GATE idt[IDT_GATE_NUM];
extern DESCRIPTOR ldt[LDT_DESC_NUM];
//TSS
extern TSS tss;
/* 关于 显示 */
extern u32 disp_position;

//进程
extern PROCESS process_table[PROCESS_NUM];
extern PROCESS * ready_process_p;
extern TASK    task_table[TASK_NUM];
extern u8     task_stack[TASK_STACK_SIZE_ALL];
extern int    reenter;  	/* fuck, 之前的u32，怪不得。。－1会大于0 */
   //开机至此的ticks数 
extern u32    ticks;
//关于中断
extern irq_handler   irq_table[IRQ_HANDLER_NUM];


//系统调用
extern SYSCALL syscall_table[SYSCALL_NUM];



extern u32 keymap[];


// console 
extern CONSOLE console_table[];
// tty
extern TTY  tty_table[];

extern CONSOLE * current_console_p;


/* 各个task进程的初始化状态 */
extern int fs_init_done;
extern int hd_init_done;
extern int syscall_init_done;
extern int tty_init_done;
extern int mm_init_done;
#endif
