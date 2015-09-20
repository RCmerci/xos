#include "xglobal.h"
#include "xconst.h"
#include "xtype.h"


void init_tasks_flag ()
{
  fs_init_done = 0;
  mm_init_done = 0;
  tty_init_done = 0;
  syscall_init_done = 0;
  hd_init_done = 0;
}

void kmain()
{
  disable_int();
  init_process();
  init_console();
  init_tty();
  init_tasks_flag();
  
  ready_process_p = process_table;
  enable_int();
  restart();
}

