#include "common.h"
#include "mm.h"
#include "mm_pub.h"
#include "xglobal.h"
#include "message.h"
#include "klib.h"
#include "xconst.h"
#include "syscall.h"

static int mm_errno;
static int mm_fork(MESSAGE * m);
static int mm_exit(MESSAGE * m);
static int mm_wait(MESSAGE * m);

void init_mm()
{
  struct boot_param_s bps;
  get_boot_param(&bps);
  printk(0, "mem size:%d\nkernel file addr:%d\n", bps.mem_size, bps.kernel_file_addr);
}

void mm_main()
{
  init_mm();
  mm_init_done = 1;
  MESSAGE * m;
  int block_flag = 0;
  while (1) {
    m = recv(NORMAL_MSG);
    switch(m->type) {
    case MSG_fork:
      block_flag = mm_fork(m);
      break;
    case MSG_exit:
      block_flag = mm_exit(m);
      break;
    case MSG_wait:
      block_flag = mm_wait(m);
      break;
    default:
      block_flag = 0;
      break;
    }
    if (block_flag) continue;
    unblock_process(m->sender_pid);
  }
}


/* 
   alloc 固定大小(USER_PROC_MEM_SIZE)的内存.
 */
static void * alloc_mem(u32 pid, u32 size)
{
  assert(size <= USER_PROC_MEM_SIZE);
  return (void*)(USER_PROC_MEM_BASE + USER_PROC_MEM_SIZE * pid);
}

/*  以下 do_xxxx 返回值代表 是否继续阻塞调用者 */
/* ------------------------------ */
/* fork 步骤
   1. 分配进程表
   2. 分配内存， 复制 父进程 内存
   3. 通知 FS
   4. 把寄存器设置好
 */
static int mm_fork(MESSAGE * m)
{
  PROCESS * p_proc_p = get_process(m->sender_pid);
  assert(p_proc_p != NULL);
  
  u32 kid_pid = get_next_pid();
  u16 kid_ldt_selector;
  u32 kid_nice = NORMAL_NICE;
  u32 kid_tty_num = p_proc_p->tty_num;

  /* search for empty process table */
  int i;
  int empty_pcb_idx = -1;
  for (i = 0; i < PROCESS_NUM; i++) {
    if (! process_table[i].ext_flag) {
      empty_pcb_idx = i;
      break;
    }
  }
  if (empty_pcb_idx == -1) {	/* 没有找到空的进程表 */
    m->fork_RST = -1;
    m->fork_ERRNO = TOO_MANY_PROCESS;
    return 0;
  }
  /* ---------------------------------- */
  /* init  process table */
  PROCESS * kid_p = &process_table[empty_pcb_idx];
  xmemcpy(&(kid_p->stack_frame), &(p_proc_p->stack_frame), sizeof(STACKFRAME));
  kid_p->ldt_selector = ((INDEX_FIRST_LDT + empty_pcb_idx) << 3);
  kid_p->pid = kid_pid;
  kid_p->nice = kid_nice;
  kid_p->ticks = kid_p->nice;
  kid_p->tty_num = p_proc_p->tty_num;
  kid_p->status = p_proc_p->status; /* 与父进程一样是STAT_WAIT_BACK_MSG,因为子进程也需要fork的返回值 */
  kid_p->send_pid = NO_PID;
  kid_p->msg_count = 0;
  kid_p->head_p = kid_p->recv_queue;
  kid_p->tail_p = kid_p->recv_queue;
  kid_p->int_msg_count = 0;
  kid_p->int_head_p = kid_p->int_recv_queue;
  kid_p->int_tail_p = kid_p->int_recv_queue;
  kid_p->parent_pid = m->sender_pid;
  /* --------------------------------------- */
  /* 复制 父进程内存 , 初始化 子进程 ldt */
  DESCRIPTOR * p_cs_p = &(p_proc_p->ldt[INDEX_LDT_CS]);
  DESCRIPTOR * p_ds_p = &(p_proc_p->ldt[INDEX_LDT_DS]);
  
  u32 p_cs_base = p_cs_p->base_0_15 | p_cs_p->base_16_23 << 16 | p_cs_p->base_24_31 << 24;
  u32 p_ds_base = p_ds_p->base_0_15 | p_ds_p->base_16_23 << 16 | p_ds_p->base_24_31 << 24;
  u32 p_cs_limit = p_cs_p->limit_0_15 | (p_cs_p->attr2 & 0x0F) << 16;
  u32 p_ds_limit = p_ds_p->limit_0_15 | (p_ds_p->attr2 & 0x0F) << 16;
  u32 p_cs_size = p_cs_p->attr2 & (DA_LIMIT_4K >> 8)?
    (p_cs_limit + 1) << LIMIT_4K_SHIFT : (p_cs_limit + 1);
  u32 p_ds_size = p_ds_p->attr2 & (DA_LIMIT_4K >> 8) ?
    (p_ds_limit + 1) << LIMIT_4K_SHIFT : (p_ds_limit + 1);

  assert(p_cs_base == p_ds_base);
  assert(p_cs_size == p_ds_size);
  
  void * kid_base = alloc_mem(kid_p->pid, p_cs_size);
  xmemcpy(kid_base, (void*)p_cs_base, p_cs_size); 

  init_descriptor(&(kid_p->ldt[INDEX_LDT_CS]),
		  (u32)kid_base,
  		  USER_PROC_MEM_SIZE >> LIMIT_4K_SHIFT,
  		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
  init_descriptor(&(kid_p->ldt[INDEX_LDT_DS]),
  		  (u32)kid_base,
  		  USER_PROC_MEM_SIZE >> LIMIT_4K_SHIFT,
  		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);
  /* init_descriptor(&(kid_p->ldt[INDEX_LDT_CS]), */
  /* 		  0, */
  /* 		  USER_PROC_MEM_SIZE >> LIMIT_4K_SHIFT, */
  /* 		  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5); */
  /* init_descriptor(&(kid_p->ldt[INDEX_LDT_DS]), */
  /* 		  0, */
  /* 		  USER_PROC_MEM_SIZE >> LIMIT_4K_SHIFT, */
  /* 		  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5); */
  
  /* 通知 FS */
  MESSAGE m_2_fs;
  m_2_fs.type = MSG_MM2FS_fork;
  m_2_fs.MM2FS_fork_PARPID = m->sender_pid;
  m_2_fs.MM2FS_fork_KIDPID = kid_p->pid;
  m_2_fs.is_int_msg = 0;
  send(&m_2_fs, TASK_FS);
  assert(m_2_fs.MM2FS_fork_RST != -1);

  kid_p->ext_flag = 1;		/* 把 ext_flag 的设置放到最后是防止：
				   schedule的时候把未初始化完的子进程给
				   unblock了.*/

  /* 现在，子进程的初始化完了，子进程的状态目前处于STAT_WAIT_BACK_MSG */
  MESSAGE * kid_m = (MESSAGE*)va2la(kid_p->pid, m);
  kid_m->fork_RST = 0;	/* 给子进程返回0 */
  m->fork_RST = kid_p->pid; /* 给父进程返回子进程的pid */


  
  unblock_process(kid_p->pid);
  //  unblock_process(p_proc_p->pid);
    
  return 0;
  
}


/* ------------ 
   exit步骤：
   1. 通知 FS
   2. 把进程的状态变成 HANGING （就是变僵尸进程）
   
 ------------ */
static int mm_exit(MESSAGE * m)
{
  PROCESS * proc = get_process(m->sender_pid);

  MESSAGE m_2_fs;
  m_2_fs.type = MSG_MM2FS_exit;
  m_2_fs.is_int_msg = 0;
  m_2_fs.MM2FS_exit_KIDPID = m->sender_pid;
  m_2_fs.MM2FS_exit_PARPID = proc->parent_pid;
  send(&m_2_fs, TASK_FS);
  assert(m_2_fs.MM2FS_exit_RST != -1);

  proc->status = STAT_HANGING;

  if (get_process(proc->parent_pid)->status == STAT_WAITING) {
    unblock_process(proc->parent_pid);
  }
  return 1;			/* block proc */
}


/* 
   wait
 */
static int mm_wait(MESSAGE * m)
{
  PROCESS * proc = get_process(m->sender_pid);

  int i;
  for (i = 0; i < PROCESS_NUM; i++) {
    if (process_table[i].ext_flag &&
	process_table[i].parent_pid == m->sender_pid &&
	process_table[i].status == STAT_HANGING) {
      process_table[i].ext_flag = 0;
      m->wait_RST = process_table[i].pid;
      return 0;
    }
  }
  /* 没有hanging的子进程 */
  proc->status = STAT_WAITING;
  m->wait_RST = -1;
  m->wait_ERRNO = NO_HANGING_SUB_PROC;
  return 1;			/* 保持这个进程 STAT_WAITING 的状态 */
}



/* --------------------------------------------------- */
/* syscall followed */
/* --------------------------------------------------- */
#define MM_PID TASK_MM

u32 fork()
{
  MESSAGE msg;
  msg.type = MSG_fork;
  msg.is_int_msg = 0;
  while (1) {
    if (! send(&msg, MM_PID)) {
      mm_errno = msg.fork_ERRNO;
      return msg.fork_RST;
    }
  }
}

int exit()
{
  MESSAGE msg;
  msg.type = MSG_exit;
  msg.is_int_msg = 0;
  while (1) {
    if (! send(&msg, MM_PID)) {
      mm_errno = msg.exit_ERRNO;
      return msg.exit_RST;
    }
  }
}

int wait()
{
  MESSAGE msg;
  msg.type = MSG_wait;
  msg.is_int_msg = 0;
  while (1) {
    if (! send(&msg, MM_PID)) {
      /* 如果没有hanging的子进程， block */
      if (msg.wait_RST == -1 && msg.wait_ERRNO == NO_HANGING_SUB_PROC) { 
	continue;
      }
      mm_errno = msg.wait_ERRNO;
      return msg.wait_RST;
    }
  }
    
}
