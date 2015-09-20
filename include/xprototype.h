#ifndef _KERNEL_PROTOTYPE
#define _KERNEL_PROTOTYPE
#include "xtype.h"

// process
void testA();
void testB();
void testC();
void tty_process();
void syscall_proc();
// 进程调度
void irq_schedule();

// 键盘中断处理
void irq_keyboard();

// 硬盘驱动
void task_hd();
//fs
void fs_main();
//mm
void mm_main();
//init
void init_proc();

//系统调用 的 实现
u32 _get_ticks();
void _write(char* buf, int len, PROCESS * process);
u32 _writek(char * str, int magic_num, PROCESS * process);
u32 _send(MESSAGE * message, u32 to, PROCESS * process);
MESSAGE * _recv(void * _unused1, void * _unused2, PROCESS * process);
#endif
