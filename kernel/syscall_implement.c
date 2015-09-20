#include "xglobal.h"
#include "xtype.h"
#include "xconst.h"
#include "common.h"
#include "message.h"

u32 _get_ticks()
{
  return ticks;
}


void _write(char* buf, int len, PROCESS * process)
{

  assert(process != NULL);
  assert(process->tty_num >=0 && process->tty_num < 3);
  int i = 0, line;
  //  buf = va2la(process->pid, buf);
  for (;i<len;i++) {
    if (buf[i] == '\n') {
      line = tty_table[process->tty_num].console_p->cursor / (80 * 2);
      tty_table[process->tty_num].console_p->cursor = (line + 1) * (80 * 2);
      continue;
    }
    xdisp_word_at(0x0f00|buf[i], tty_table[process->tty_num].console_p->cursor);
    tty_table[process->tty_num].console_p->cursor += 2;
    if (current_console_p == tty_table[process->tty_num].console_p) {
      set_cursor(tty_table[process->tty_num].console_p->cursor);
    }
  }
}

void _writek(char * str, int magic_num, PROCESS * process)
{
  char * addr;
  if (reenter > 0) {		/* ring 0 */
    addr = str;
  }else {			/* ring 1-3 */
    addr = (char *)va2la(process->pid, str);    
  }
  _write(addr, strlen(addr), process);
  if (magic_num == PANIC_MAGIC || (magic_num == ASSERT_MAGIC && process->pid < NR_SYS_TASKS)) {
    __asm__ __volatile__ ("ud2");
  }
  return;  
}









/* -------------------------------------------------------------------------------- */
static void block(PROCESS * p)
{
  assert((p->status == STAT_WAIT_MSG) || (p->status == STAT_WAIT_BACK_MSG));
  schedule();
}

static unblock(PROCESS * p)
{
  assert((p->status == STAT_RUN) || (p->status == STAT_PAUSE));
}


static int deadlock(PROCESS * process, u32 to)
{
  PROCESS * tmp_proc = get_process(to);
  while(1) {
    if (tmp_proc->status != STAT_WAIT_BACK_MSG) break;
    if (tmp_proc == process) return 1;
    tmp_proc = get_process(tmp_proc->send_pid);
  }

return 0;
}



/* 
@return : 
0 正常
1 recv_queue满了
 */
u32 _send(MESSAGE * message, u32 to, PROCESS * process)
{
  disable_int();		/* 里面的处理有操作进程表里的数据结构，如果多个进程修改同一个结构会有问题 */

  if (process) {  /* !!!重要!!!.....,地址的转换 */
    message = va2la(process->pid, message);
  }
    /* ------------ */

  
  PROCESS * dest_proc = get_process(to);
  if (! message->is_int_msg) {

    assert(process->pid != to);
    if (deadlock(process, to)) {
      assert("deadlock" == 0);
    }
    /*  普通 进程间 消息 */
    if (dest_proc->msg_count >= MSG_BUF_SIZE) { /* recv_queue满了 */
      enable_int();
      return 1;
    }

    message->sender_pid = process->pid;
    //    assert(process->status < 5);
    if (dest_proc->status == STAT_WAIT_BACK_MSG) {
      *(dest_proc->head_p) = message;
      dest_proc->head_p ++ ;
      if (dest_proc->head_p == dest_proc->recv_queue + MSG_BUF_SIZE) {
	dest_proc->head_p = dest_proc->recv_queue;
      }
      dest_proc->msg_count += 1;
      process->status = STAT_WAIT_BACK_MSG;
    }
    else if (dest_proc->status == STAT_WAIT_MSG) {
      *(dest_proc->head_p) = message;    
      dest_proc->head_p ++ ;
      if (dest_proc->head_p == dest_proc->recv_queue + MSG_BUF_SIZE) {
	dest_proc->head_p = dest_proc->recv_queue;
      }
      dest_proc->msg_count += 1;
      process->status = STAT_WAIT_BACK_MSG;
      dest_proc->status = STAT_PAUSE;
      unblock(dest_proc);
    }
    else if (dest_proc->status == STAT_PAUSE ||
	     dest_proc->status == STAT_RUN) {
      *(dest_proc->head_p) = message;    
      dest_proc->head_p ++ ;
      if (dest_proc->head_p == dest_proc->recv_queue + MSG_BUF_SIZE) {
	dest_proc->head_p = dest_proc->recv_queue;
      }
      dest_proc->msg_count += 1;
      process->status = STAT_WAIT_BACK_MSG;
    } else {
      assert("wrong dest proc status"==NULL);
    }
    assert((process->status == STAT_WAIT_MSG) || (process->status == STAT_WAIT_BACK_MSG));
    block(process);
    //    process->send_pid = dest_proc->pid;
  }
  else {
    /* 来自中断的消息 */
    assert(process == 0); // 如果是来自中断的消息,那么中断处理函数本身在 ring0 ，
                           //所以调用本函数并不是 int 80h来的，那么process参数就是中断处理函数自己加的，应为NULL
                           // 而且在ring0也不应该有进程。
    
    if (dest_proc->int_msg_count >= MSG_BUF_SIZE) {
      enable_int();
      return 1;
    }
    message->sender_pid = -1;

    *(dest_proc->int_head_p) = message;
    dest_proc->int_head_p ++ ;
    if (dest_proc->int_head_p == dest_proc->int_recv_queue + MSG_BUF_SIZE) {
      dest_proc->int_head_p = dest_proc->int_recv_queue;
    }
    dest_proc->int_msg_count++;
    
    if (dest_proc->status == STAT_WAIT_BACK_MSG) {
      ;
    }
    if (dest_proc->status == STAT_WAIT_MSG) {
      dest_proc->status = STAT_PAUSE;
      unblock(dest_proc);
    }
    if (dest_proc->status == STAT_PAUSE ||
	dest_proc->status == STAT_RUN) {
      ;
    }
    
  }
  enable_int();
  return 0;
}


/* 
@return
-1: 错误
>0: 正常
-------------------------
优先接收hard int msg
 */
MESSAGE* _recv(int type, void * _unused1, PROCESS * process)
{
  disable_int();
  MESSAGE * m;
  if (type & INT_MSG) {	/* 接受 hard int msg*/
    /* interrupt msg */
    if (process->int_msg_count == 0) {
      if (!(type & NORMAL_MSG)) { /* 如果不接受normal msg ， 那么就结束了，block之 */
	process->status = STAT_WAIT_MSG;
	block(process);
	enable_int();
	return NULL;
      }
      goto RECV_NORMAL_MSG;
    }
    assert(process->int_msg_count <= MSG_BUF_SIZE);
    m = *(process->int_tail_p);
    process->int_tail_p ++ ;
    if (process->int_tail_p == process->int_recv_queue + MSG_BUF_SIZE) {
      process->int_tail_p = process->int_recv_queue;
    }
    process->int_msg_count--;
    goto RECV_END;
  }
RECV_NORMAL_MSG:
  if (type & NORMAL_MSG) {
    if (process->msg_count == 0) {
      process->status = STAT_WAIT_MSG;
      enable_int();
      block(process);
      return NULL;
    }
    assert(process->msg_count <= MSG_BUF_SIZE);
    m = *(process->tail_p);
    process->tail_p++;
    if (process->tail_p == process->recv_queue + MSG_BUF_SIZE) {
      process->tail_p = process->recv_queue;
    }
    process->msg_count--;
  }
  else {
    assert("no such type msg" == 0);
  }
RECV_END:
  enable_int();
  return m;
}

/* run at ring1-3,
   这个是用户进程调用的recv函数，
   系统调用是recv_， 如果当进程没有msg的时候调用recv_,
   进程会进入STAT_WAIT_MSG状态，但是当下次status变为pause被调度的时候会从上次的返回的地方之后继续执行下去。
   会导致返回值是0，所以必须外面包一层判断。如果是0，再recv_()一遍。
   
 */
MESSAGE * recv(int type)
{
  /* if (type & INT_MSG) {		/\* 先试着接收 INT_MSG *\/ */
  /*   m = (MESSAGE*)recv_(INT_MSG); */
  /* } */
  /* if (m == NULL && (type & NORMAL_MSG)) { */
  /*   m = (MESSAGE*)recv_(NORMAL_MSG); */
  /* } */
  MESSAGE * m = NULL;
  m = (MESSAGE*)recv_(type);
  if (m == NULL) {
    return recv(type);
  }
  return m;
}
