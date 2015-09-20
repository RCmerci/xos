#ifndef _KERNEL_COMMON
#define _KERNEL_COMMON

#include "xtype.h"
/* 常用函数 */


void printf(char * format, ...);

void sprintf(char * s, char * format, ...);

void itoa(char * buf, int i);

int strlen(char * s);

void strcpy(char * from, char * to);

int strcmp(char * a, char * b);

void * va2la(int pid, void * va);

#define assert(exp)   if(exp);					\
  else assert_fail(#exp, __FILE__, __BASE_FILE__, __LINE__)

void xmemcpy(void * dest, void * src, int len);

void* memset(void* dst, u8 c, int len);

#define min(a, b)      (a<b?a:b)
#define max(a, b)      (a>b?a:b)

u8 gen_byte(int start, int end); /* gen_byte(1, 5) -> 00111110*/

void delay(int s);		/* 大概是秒 */

PROCESS * get_process(u32 pid);

void block_process(u32 pid, u32 stat);
void unblock_process(u32 pid);


int open(char * name, int mode);

int read(int inode_nr, void * buf, int nbytes);

int create(char * name);

int write(int inode_nr, void * buf, int nbytes);

int close(int inode_nr);

int unlink(char * name);

u32 fork();

int exit();

int wait();

#endif
