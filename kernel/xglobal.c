#include "xtype.h"
#include "xconst.h"
#include "xprototype.h"


u8 gdt_ptr[6];
DESCRIPTOR gdt[GDT_DESC_NUM];
u8 idt_ptr[6];
GATE idt[IDT_GATE_NUM];

u32 disp_position;

TSS tss;

/* DESCRIPTOR ldt[LDT_DESC_NUM]; */
/* 增加进程的步骤： 		 
   1. task_table 里 加上 相应项目
   2. xconst.h 里面 增大 PROCESS_NUM 
   3. 就是这样
 */
PROCESS process_table[PROCESS_NUM];
PROCESS * ready_process_p;
TASK    task_table[TASK_NUM] = {
  {tty_process, "tty", 10, 0},
  {syscall_proc, "syscall_proc", 10, 0},          /* pid = 1, 在syscall_c.c 的 SYSCALL_PROC_PID=1 */
  {task_hd, "hd", 10, 1},
  {fs_main, "fs", 10, 0},
  {mm_main, "mm", 10, 1},
  {testA, "a", 5, 0},
  {testB, "b", 10, 0},
  {testC, "c", 20, 0},
  {init_proc, "INIT", 10, 0},

};
u8     task_stack[TASK_STACK_SIZE_ALL];
int    reenter=-1;

irq_handler irq_table[IRQ_HANDLER_NUM] = {
  irq_schedule, irq_keyboard, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

u32 ticks;

SYSCALL syscall_table[SYSCALL_NUM] = {
  _get_ticks, _write, _writek, _send, _recv,
};


CONSOLE  console_table[CONSOLE_NUM];
TTY     tty_table[TTY_NUM];

CONSOLE * current_console_p;

/* 各个task进程的初始化状态 */
int fs_init_done;
int hd_init_done;
int syscall_init_done;
int tty_init_done;
int mm_init_done;
