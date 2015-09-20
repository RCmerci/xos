#include "xconst.h"
#include "xglobal.h"
#include "xtype.h"
#include "message.h"
#include "syscall.h"

void syscall_proc()
{
  syscall_init_done = 1;
  
  while(1) {
    MESSAGE * msg_p = recv(NORMAL_MSG);
    //    msg_p = va2la(msg_p->sender_pid, msg_p); //地址已经转换好了
    //    printf("syscall");
    struct MSG_BODY_S * body_p = &(msg_p->body);
    switch (msg_p->type) {
    case MSG_get_ticks:
      body_p->arg1 = _get_ticks();
      break;
    case MSG_write:
      _write(body_p->arg1, body_p->arg2, get_process(msg_p->sender_pid));
      break;
    case MSG_writek:
      _writek(body_p->arg1, body_p->arg2, get_process(msg_p->sender_pid));
      break;
    default:
      break;
    }
    unblock_process(msg_p->sender_pid);
  }
}

