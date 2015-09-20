#ifndef _KERNEL_SYSCALL
#define _KERNEL_SYSCALL
#include "xtype.h"

int get_ticks();

int write(int inode_nr, void * buf, int len);

void writek(char * buf, int magic_num);

MESSAGE * recv(int type);

u32 send(MESSAGE * m, u32 to_pid);


#endif
