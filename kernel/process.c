#include "xconst.h"
#include "xglobal.h"
#include "xtype.h"
#include "syscall.h"
#include "common.h"
#include "fs_pub.h"


extern int fs_errno;		/* just for test */

void ring1_task_schedule ()
{
  u32 max_ticks;
  int i, index;
    
 select_max_ticks:
  max_ticks = 0;
  i = 0;
  index = 0;
  for (;i<NR_SYS_TASKS;i++){
    if (process_table[i].ext_flag &&
	process_table[i].ticks > max_ticks &&
	(process_table[i].status == STAT_PAUSE ||
	 process_table[i].status == STAT_RUN)){
      index = i;
      max_ticks = process_table[i].ticks;
    }
  }
  if (max_ticks == 0){
    int i = 0;
    for(;i<NR_SYS_TASKS;i++){
      if (process_table[i].ext_flag &&
	  (process_table[i].status == STAT_RUN ||
	   process_table[i].status == STAT_PAUSE))
	process_table[i].ticks = process_table[i].nice;
    }
    goto select_max_ticks;
  }
  
  {
    if (ready_process_p->status == STAT_RUN) { /* 可能status是STAT_WAIT_MSG,因为可能从block来 */
      ready_process_p->status = STAT_PAUSE;
    }
    ready_process_p = &process_table[index];
    ready_process_p->ticks--;
    ready_process_p->status = STAT_RUN;
  }
  assert(ready_process_p->status == STAT_RUN);
  assert(ready_process_p->pid < NR_SYS_TASKS);

}

void schedule()
{
  u32 max_ticks;
  int i, index;
  if (!(fs_init_done && hd_init_done && tty_init_done && syscall_init_done && mm_init_done)) {
    ring1_task_schedule();
    return;
  }
 select_max_ticks:
  max_ticks = 0;
  i = 0;
  index = 0;
  for (;i<PROCESS_NUM;i++){
    if (process_table[i].ext_flag &&
	process_table[i].ticks > max_ticks &&
	(process_table[i].status == STAT_PAUSE ||
	 process_table[i].status == STAT_RUN)){
      index = i;
      max_ticks = process_table[i].ticks;
    }
  }
  if (max_ticks == 0){
    int i = 0;
    for(;i<PROCESS_NUM;i++){
      if (process_table[i].ext_flag &&
	  (process_table[i].status == STAT_RUN ||
	   process_table[i].status == STAT_PAUSE))
	process_table[i].ticks = process_table[i].nice;
    }
    goto select_max_ticks;
  }
  
  {
    if (ready_process_p->status == STAT_RUN) { /* 可能status是STAT_WAIT_MSG,因为可能从block来 */
      ready_process_p->status = STAT_PAUSE;
    }
    ready_process_p = &process_table[index];
    ready_process_p->ticks--;
    ready_process_p->status = STAT_RUN;
  }
  assert(ready_process_p->status == STAT_RUN);
    
}


void irq_schedule()
{
  ticks++;
  
  if(reenter > 0) {
    return ;
  }
  if (fs_init_done && hd_init_done && tty_init_done && syscall_init_done && mm_init_done)
    schedule();
  else
    ring1_task_schedule();
}

PROCESS * get_process(u32 pid)
{
  int i;
  for (i=0;i<PROCESS_NUM;i++) {
    if (process_table[i].ext_flag && process_table[i].pid == pid)
      return & process_table[i];
  }
  return NULL;
}

void block_process(u32 pid, u32 stat)
{
  PROCESS * proc;
  if (NULL == (proc = get_process(pid))) return ; /* 并没有找到这个pid的进程 */
  proc->status = stat;
  assert(proc->status == STAT_WAIT_MSG || proc->status == STAT_WAIT_BACK_MSG);
}

void unblock_process(u32 pid)
{
  PROCESS * proc;
  if (NULL == (proc = get_process(pid))) return;
  assert(proc->status != STAT_HANGING);
  proc->status = STAT_PAUSE;
}




/* ---------------------------------- */
void delay(int t)
{
  u32 start = get_ticks();
  while(!((get_ticks()-start) > 100*t)){}
}
void testA()
{
  /* char buf[100]; */
  /* int f = create("test1"); */
  /* if (f != -1) { */
  /*   assert(open("test1", FILE_WR|FILE_RD)==-1); */
  /*   assert(fs_errno==FILE_HAS_OPENED); */
  /* } else { */
  /*   f = open("test1", FILE_RD|FILE_WR); */
  /* } */
  /* write(f, "fuck it", strlen("fuck it")+1); */
  /* assert(close(f)==0); */
  /* assert(close(f)==-1); */
  /* assert(fs_errno==FILE_NOT_OPENED); */
  /* assert(-1!= (f=open("test1", FILE_WR|FILE_RD))); */
  
  /* assert(read(f,buf, 5)==5); */
  /* buf[5]=0; */
  /* printf("test1:%s", buf); */
  /* assert(close(f)==0); */
  /* assert(unlink("test1")==0); */
  int f=open("tty0", FILE_RD|FILE_WR);
  while(1){
    write(f, "A!", 2);
    delay(5);
  }
  return;
}

void testB()
{
  char buf [50];
  int r;
  int f = open("tty2", FILE_WR|FILE_RD);
  assert(f!=-1);
  while (1) {
    write(f, "$ ", 2);
    int i=0;
    while(1) {
      r = read(f, buf+i, 1);  	/* readline */
      if (buf[i] == '\n') {
  	buf[i] = 0;
  	break;
      }
      i++;
    }
    if (!strcmp(buf, "hello")) {
      char * ss = "world\n";
      write(f, ss, strlen(ss));
    }
    else {
      write(f, "{", 1);
      write(f, buf, strlen(buf));
      write(f, "}\n", 2);
    }
  }
  while (1) {
    /* printk(0, "B!"); */
    /* delay(1); */
  }
}
void testC()
{
  int i;
  PROCESS * proc;
  while(1) {
    for(i=0;i<PROCESS_NUM;i++) {
      if (NULL != (proc = get_process(i))) {
	if (process_table[i].stack_frame.esp <= task_stack + TASK_STACK_SIZE * (i + 1) &&
	    process_table[i].stack_frame.esp >= task_stack + TASK_STACK_SIZE * i){
	  continue;
	}
	
	printf("process %s use illegal stack memory.\n", process_table[i].name);
      }
    }
  }
}





int test_kid;
int test_p;
void init_proc()
{
  assert(-1 != open("tty0", FILE_RD|FILE_WR));
  test_kid = 666;
  int rst;
  rst = fork();

  if (rst) {			/* father */
    u32 p = wait();
    printf("sub proc exit:%d\n", p);

    while(1){
      printf("INIT process");
      delay(5);
    }
  } else {			/* child */
    printf("fs test:\n");
    int f2 = create("test_kid");
    assert(f2!=-1);
    write(f2, "666kid", 6);
    close(f2);
    char buf[10];
    assert(-1 !=(f2= open("test_kid", FILE_RD|FILE_WR)));
    read(f2, buf, 6);
    buf[6] = 0;
    printf("read:%s ", buf);
    assert(-1 != close(f2));
    assert(-1 != unlink("test_kid"));
    assert(-1 == open("test_kid", FILE_RD|FILE_WR));
    printf("init kid process exit\n");
    exit();
    while (1) {
      
      printf("INIT_KID");
      delay(3);
    }
  }
}
