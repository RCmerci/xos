#include "xconst.h"
#include "xtype.h"
#include "message.h"
#include "syscall.h"


#define SYSCALL_PROC_PID     1	/* syscall_proc -> pid */

int get_ticks()
{
  MESSAGE msg;
  msg.type = MSG_get_ticks;
  msg.is_int_msg = 0;
  while(1){
    if (!send(&msg, SYSCALL_PROC_PID)) { /* 0 for success */
      return msg.body.arg1;
    }
  }
}

/* void write(char * buf, int len) */
/* { */
/*   MESSAGE msg; */
/*   msg.type = MSG_write; */
/*   msg.is_int_msg = 0; */
/*   msg.body.arg1 = (u32)buf; */
/*   msg.body.arg2 = (u32)len; */
/*   while(1) { */
/*     if (!send(&msg, SYSCALL_PROC_PID)) { */
/*       return; */
/*     } */
/*   } */
/* } */


/* 不依赖文件系统的write */
void writek(char * buf, int magic_num) 
{
  MESSAGE msg;
  msg.type = MSG_writek;
  msg.is_int_msg = 0;
  msg.body.arg1 = (u32)buf;
  msg.body.arg2 = (u32)magic_num;
  while(1) {
    if (!send(&msg, SYSCALL_PROC_PID)) {
      return;
    }
  }
}

