#include "xconst.h"
#include "xtype.h"
#include "xglobal.h"
#include "message.h"
#include "common.h"
#include "fs_pub.h"


typedef struct wait_msg_s {
  MESSAGE m;
  int ext_flag;
} WAIT_MSG;

static WAIT_MSG wait_read_msgs[TTY_NUM];
//static int read_msg_count;
static WAIT_MSG wait_write_msgs[TTY_NUM];
//static int write_msg_count;


void init_console()
{
  int i = 0;
  for (;i < CONSOLE_NUM; i++) {
    console_table[i].start_addr = FIRST_CONSOLE_ADDR + i * CONSOLE_ADDR_LEN;
    console_table[i].end_addr   = FIRST_CONSOLE_ADDR + (i+1) * CONSOLE_ADDR_LEN - 2;//每个字符占了2byte
    console_table[i].current_screen_addr = console_table[i].start_addr;
    console_table[i].cursor = console_table[i].start_addr;
    
  }
  current_console_p = &console_table[0];
  set_cursor(0);
}

void init_tty()
{
  init_keyboard();
  int i = 0;
  for (;i < TTY_NUM;i++) {
    tty_table[i].console_p = &console_table[i];
    tty_table[i].head_p = tty_table[i].read_buf;
    tty_table[i].tail_p = tty_table[i].read_buf;
    tty_table[i].buf_count = 0;

    tty_table[i].in_head_p = tty_table[i].tty_in_cache;
    tty_table[i].in_tail_p = tty_table[i].tty_in_cache;
    tty_table[i].in_count = 0;
    
    tty_table[i].out_head_p = tty_table[i].tty_out_cache;
    tty_table[i].out_tail_p = tty_table[i].tty_out_cache;
    tty_table[i].out_count = 0;

    tty_table[i].line_in_head_p = tty_table[i].line_in_cache;
    tty_table[i].line_in_tail_p = tty_table[i].line_in_cache;
    tty_table[i].line_in_count = 0;

  }
}

static int is_current_console(CONSOLE * console_p)
{
  if (console_p == current_console_p) return 1;
  return 0;
}

static CONSOLE * get_current_console()
{
  return current_console_p;
}

static int is_tty_buf_empty(TTY * tty_p)
{
  if (tty_p->buf_count == 0) return 1;
  return 0;
}




static void add_key_to_in_cache(TTY * tty_p)
{
  while (tty_p->line_in_count > 0) {
    *(tty_p->in_head_p) = *(tty_p->line_in_tail_p);
    tty_p->in_head_p ++ ;
    tty_p->line_in_tail_p ++ ;
    if (tty_p->in_head_p == tty_p->tty_in_cache + TTY_CHAR_CACHE_SIZE)
      tty_p->in_head_p = tty_p->tty_in_cache;
    if (tty_p->line_in_tail_p == tty_p->line_in_cache + TTY_CHAR_CACHE_SIZE)
      tty_p->line_in_tail_p = tty_p->line_in_cache;
    if (tty_p->in_count == TTY_CHAR_CACHE_SIZE) { /* in_cache full */
      ;
    } else {
      tty_p->in_count ++ ;
    }
    tty_p->line_in_count -- ;
  }
}


/* @return
   -1 -> fail
   0  -> succ
 */
static int add_key_to_line_in_cache(u32 k, TTY * tty_p)
{
  char c;
  u32 key;
  
  if (k & FLAG_SHIFT_L || k & FLAG_SHIFT_R) {
    key = keymap[(k&0xFF)*3+1];
  } else {
    key = keymap[(k&0xFF)*3];
  }
  
  if (key & FLAG_EXT) {
    switch (key) {
    case ENTER:
      c = '\n';
      break;
    default:
      return -1;
    }
  } else {
    c = key;
  }
  
  if (tty_p->in_count >= TTY_CHAR_CACHE_SIZE) {
    return -1;			/* 满了 */
  }
  *(tty_p->line_in_head_p) = c;
  tty_p->line_in_head_p ++ ;
  if (tty_p->line_in_head_p == tty_p->line_in_cache + TTY_CHAR_CACHE_SIZE) {
    tty_p->line_in_head_p = tty_p->line_in_cache;
  }
  tty_p->line_in_count ++ ;


  if (c == '\n')
    add_key_to_in_cache(tty_p);

  return 0;
}


/* delete one char from line_in_cache */
static void del_key_from_line_in_cache(TTY * tty_p)
{
  if (tty_p->line_in_count == 0) return;
  tty_p->line_in_head_p -- ;
  if (tty_p->line_in_head_p == tty_p->line_in_cache - 1) {
    tty_p->line_in_head_p = tty_p->line_in_cache + TTY_CHAR_CACHE_SIZE - 1;
  }
  tty_p->line_in_count -- ;
  assert(tty_p->line_in_count >= 0);
}

/* @return
   0->read something
   -1->not
 */
static int keyboard_read(TTY * tty_p)
{
  u32 key;
  int has_read_something = -1;
  /* 对应 当前console 而且 键盘的buf里不是空 */
  while (is_current_console(tty_p->console_p) && !is_keyboard_buf_empty()) { 
    key = keyboard_get_key();
    if (tty_p->buf_count < TTY_BUF_SIZE) {
      *(tty_p->head_p) = key;
      tty_p->head_p ++ ;
      if (tty_p->head_p == tty_p->read_buf + TTY_BUF_SIZE)
	tty_p->head_p = tty_p->read_buf;
      tty_p->buf_count ++ ;
      
    }
    has_read_something = 0;
  }
  return has_read_something;
}


/* 
   fresh tty buf to screen
*/
static void keyboard_write(TTY * tty_p)
{
  int i = 0;
  u16 print;
  u32 key = 0;
  
  while (!is_tty_buf_empty(tty_p)) {
    key = keymap[(*(tty_p->tail_p)&0xff)*3];
    if (*(tty_p->tail_p) & FLAG_CTRL_L) {
      if (key == 'p') {
	scroll(tty_p->console_p, SCROLL_UP, 1);
      }
      if (key == 'n') {
	scroll(tty_p->console_p, SCROLL_DOWN, 1);
      }
      if (key == F1) {
	switch_console(0);
	current_console_p = &console_table[0];
      }
      if (key == F2) {
	switch_console(1);
	current_console_p = &console_table[1];	
      }
      if (key == F3) {
	switch_console(2);
	current_console_p = &console_table[2];	
      }
      goto CLEAR;
    }
    int line;
    if (key & FLAG_EXT) {
      if (key == ENTER) {
	line = tty_p->console_p->cursor / (80 * 2);
	tty_p->console_p->cursor = (line + 1) * (80 * 2);
    	goto ADD_KEY_TO_LINE_CACHE;
      }
      if (key == BACKSPACE) {
	tty_p->console_p->cursor -= 2;
	xdisp_word_at(0|0x0F00, tty_p->console_p->cursor);
	del_key_from_line_in_cache(tty_p);
	goto CLEAR;
      }
    }
    assert(tty_p != NULL);
    xdisp_word_at(key|0x0f00, tty_p->console_p->cursor);
    tty_p->console_p->cursor += 2;    //write do it

  ADD_KEY_TO_LINE_CACHE:
    add_key_to_line_in_cache(*(tty_p->tail_p), tty_p);
  CLEAR:
    if (is_current_console(tty_p->console_p))
      set_cursor(tty_p->console_p->cursor);
    tty_p->buf_count--;
    tty_p->tail_p++;
    if (tty_p->tail_p == tty_p->read_buf + TTY_BUF_SIZE)
      tty_p->tail_p = tty_p->read_buf;
  }
}


static void flush_tty_out_cache(TTY * tty_p)
{
  char c;
  int line;
  while (tty_p->out_count > 0) {
    c = *(tty_p->out_tail_p);
    if (c == '\n') {
      line = tty_p->console_p->cursor / (80 * 2);
      tty_p->console_p->cursor = (line + 1) * (80 * 2);
    } else {
      xdisp_word_at(c|0x0F00, tty_p->console_p->cursor);
      tty_p->console_p->cursor += 2;
    }
    tty_p->out_tail_p ++ ;
    tty_p->out_count -- ;
    if (tty_p->out_tail_p == tty_p->tty_out_cache + TTY_CHAR_CACHE_SIZE)
      tty_p->out_tail_p = tty_p->tty_out_cache;


    if (is_current_console(tty_p->console_p))
      set_cursor(tty_p->console_p->cursor);
    
  }
}

#define LAST_READ_POS  body.arg5
static int do_read(MESSAGE * m)
{
  TTY * tty_p = &(tty_table[m->tty_read_TTY_NR]);
  int nbytes  = ((MESSAGE*)(m->tty_read_REQUEST_M))->read_NBYTES;
  u8 * buf    = va2la(((MESSAGE*)(m->tty_read_REQUEST_M))->sender_pid,
		      (u8*)((MESSAGE*)(m->tty_read_REQUEST_M))->read_BUF);
  int curr_rst = ((MESSAGE*)(m->tty_read_REQUEST_M))->read_RST;
  char * last_read_p = (char*)(m->LAST_READ_POS);
  
  char c;
  if (curr_rst == 0) {
    last_read_p = tty_p->in_tail_p;
  }
  while (last_read_p != tty_p->in_head_p) {
    c = *last_read_p;
    /* if (curr_rst == nbytes && c != '\n') { /\* 已经足够，但是本次char不是\n *\/ */
    /*   last_read_p++; */
    /*   if (last_read_p == tty_p->tty_in_cache + TTY_CHAR_CACHE_SIZE) { */
    /* 	last_read_p = tty_p->tty_in_cache; */
    /*   } */
    /*   m->LAST_READ_POS = (u32)last_read_p; */
    /*   return 0; */
    /* } */
    /* if (curr_rst == nbytes && c == '\n') { /\* 已经足够而且本次char==\n *\/ */
    /*   return 1; */
    /* } */
    buf[curr_rst] = c;
    tty_p->in_tail_p ++ ;
    if (tty_p->in_tail_p == tty_p->tty_in_cache + TTY_CHAR_CACHE_SIZE) {
      tty_p->in_tail_p = tty_p->tty_in_cache;
    }
    last_read_p = tty_p->in_tail_p;
    tty_p->in_count -- ;
    curr_rst ++ ;
    ((MESSAGE*)(m->tty_read_REQUEST_M))->read_RST ++ ;
    
    if (curr_rst == nbytes) {	/* 加上本次char刚好足够nbytes */
      //      if (c == '\n') {		/* 本次char ==\n, 直接成功返回 */
      ((MESSAGE*)(m->tty_read_REQUEST_M))->read_RST = nbytes;
      return 1;
	//      }
    }
  }
  m->LAST_READ_POS = (u32)last_read_p;
  return 0;
}

#define LAST_WRITE_POS  body.arg5
static int do_write(MESSAGE * m)
{
  TTY * tty_p = &(tty_table[m->tty_write_TTY_NR]);
  int nbytes  = ((MESSAGE*)(m->tty_write_REQUEST_M))->write_NBYTES;
  u8 * buf    = va2la(((MESSAGE*)(m->tty_write_REQUEST_M))->sender_pid,
		      (u8*)((MESSAGE*)(m->tty_write_REQUEST_M))->write_BUF);
  int curr_rst = ((MESSAGE*)(m->tty_write_REQUEST_M))->write_RST;
  char * last_write_p = (char*)(m->LAST_WRITE_POS);
  
  if(nbytes == 0) {
    ((MESSAGE*)(m->tty_write_REQUEST_M))->write_RST = 0;
    return 1;
  }
  
  if (curr_rst == 0) last_write_p = tty_p->out_head_p;
  
  while(1) {
    *last_write_p = buf[curr_rst];
    last_write_p ++ ;
    if (last_write_p == tty_p->tty_out_cache + TTY_CHAR_CACHE_SIZE)
      last_write_p = tty_p->tty_out_cache;
    curr_rst ++ ;
    tty_p->out_head_p ++ ;
    if (tty_p->out_head_p == tty_p->tty_out_cache + TTY_CHAR_CACHE_SIZE)
      tty_p->out_head_p = tty_p->tty_out_cache;
    if (tty_p->out_tail_p == tty_p->out_head_p) {
      assert(tty_p->out_count == TTY_CHAR_CACHE_SIZE);
      tty_p->out_tail_p ++ ;
      if (tty_p->out_tail_p == tty_p->tty_out_cache + TTY_CHAR_CACHE_SIZE)
	tty_p->out_tail_p = tty_p->tty_out_cache;
    } else {
      tty_p->out_count ++ ;
    }
    if (curr_rst == nbytes) {
      ((MESSAGE*)(m->tty_write_REQUEST_M))->write_RST = curr_rst;
      return 1;
    }
  }
  m->LAST_WRITE_POS = last_write_p;
  return 0;
}

static void queue_read_msg(MESSAGE * m)
{
  ((MESSAGE *)(m->tty_read_REQUEST_M))->read_RST = 0;
  int i;
  for (i=0;i<TTY_NUM;i++){
    if (! wait_read_msgs[i].ext_flag) {
      xmemcpy(&(wait_read_msgs[i].m), m, sizeof(MESSAGE));
      wait_read_msgs[i].ext_flag = 1;
      break;
    }
  }
}

static void queue_write_msg(MESSAGE * m)
{
  ((MESSAGE*)(m->tty_write_REQUEST_M))->write_RST = 0;
  int i;
  for (i=0;i<TTY_NUM;i++){
    if (! wait_write_msgs[i].ext_flag) {
      xmemcpy(&(wait_write_msgs[i].m), m, sizeof(MESSAGE));
      wait_write_msgs[i].ext_flag = 1;
      break;
    }
  }  
}

void tty_process()
{
  MESSAGE m;
  TTY * tty_p;
  int i=0;
  PROCESS * self = get_process(TASK_TTY);


  tty_init_done = 1;
  
  while(1){
    xmemcpy(&m, recv(NORMAL_MSG|INT_MSG), sizeof(MESSAGE)); /* 这里memcpy是因为, 收到的msg可能来自fs_task
							       ，fs_task作为发送者不能被block，
							       会马上被唤醒，那么栈上的变量会没有，
							       所以要复制一份
							    */

    switch(m.type) {
    case MSG_tty_read:
      queue_read_msg(&m);
      break;
    case MSG_tty_write:
      queue_write_msg(&m);
      break;
    default:			/* 这个是来自键盘中断的消息，因为中断函数不能block，所以消息内容不能确定
				   所以，不能用type来判断
				 */
      for (i=0, tty_p=&tty_table[i];i<TTY_NUM;i++, tty_p=&(tty_table[i])) {
	if (! keyboard_read(tty_p)) break;
      }
      
      break;
    }
    
    if (m.type == MSG_tty_read || m.type == MSG_tty_write) { /* unblock fs_task */
      unblock_process(m.sender_pid);
    }
    for (i=0;i<TTY_NUM;i++) {
      if (wait_read_msgs[i].ext_flag) {
	if (do_read(&(wait_read_msgs[i].m))) {
	  wait_read_msgs[i].ext_flag = 0;
	  unblock_process(((MESSAGE*)(wait_read_msgs[i].m.tty_read_REQUEST_M))->sender_pid);
	}
      }
    }
    for (i=0;i<TTY_NUM;i++) {
      if (wait_write_msgs[i].ext_flag) {
	if (do_write(&(wait_write_msgs[i].m))) {
	  wait_write_msgs[i].ext_flag = 0;
	  unblock_process(((MESSAGE*)(wait_write_msgs[i].m.tty_write_REQUEST_M))->sender_pid);
	}
      }
    }
    
    for (i=0, tty_p=&tty_table[i];i<TTY_NUM;i++, tty_p=&(tty_table[i])) {
      keyboard_write(tty_p);
      flush_tty_out_cache(tty_p);
    }

    /* 清理来自keyboard中断的信息 */
    /* while(self->int_msg_count > 0) { */
    /*   recv(INT_MSG); */
    /* } */
  
    
  }
}
