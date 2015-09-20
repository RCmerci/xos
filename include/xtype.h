#ifndef _KERNEL_TYPE
#define _KERNEL_TYPE
#include "xconst.h"


typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
#define NULL               (void*)0

typedef void(* u32_handler )();

typedef struct DESCRIPTOR_S
{
  u16 limit_0_15;
  u16 base_0_15;
  u8  base_16_23;
  u8  attr1;
  u8  attr2;			/* 16-19位的limit在attr2里面*/
  u8  base_24_31;
}DESCRIPTOR;

typedef struct GATE_S
{
  u16 offset_0_15;
  u16 selector;
  u8  param_count;
  u8  attr;
  u16 offset_16_31;
}GATE;


typedef struct TSS_S
{
  u32 backlink;
  u32 esp0;
  u32 ss0;
  u32 esp1;
  u32 ss1;
  u32 esp2;
  u32 ss2;
  u32 cr3;
  u32 eip;
  u32 flags;
  u32 eax;
  u32 ecx;
  u32 edx;
  u32 ebx;
  u32 esp;
  u32 ebp;
  u32 esi;
  u32 edi;
  u32 es;
  u32 cs;
  u32 ss;
  u32 ds;
  u32 fs;
  u32 gs;
  u32 ldt;
  u16 trap;
  u16 iobase;
}TSS;



// 关于进程
typedef struct STACKFRAME_S
{
  u32 gs;
  u32 fs;
  u32 es;
  u32 ds;
  u32 edi;
  u32 esi;
  u32 ebp;
  u32 _esp;			/* pushad 会跳过这个  */
  u32 ebx;
  u32 edx;
  u32 ecx;
  u32 eax;
  u32 retaddr;
  u32 eip;
  u32 cs;
  u32 eflags;
  u32 esp;
  u32 ss;
}STACKFRAME;


typedef void*(*TASK_ENTRY)();
typedef struct TASK_S
{
  TASK_ENTRY task_entry;
  char name[50];
  u32 nice;
  u32 tty_num;
} TASK;

typedef   void*(*irq_handler)();

typedef void *  SYSCALL;

typedef struct KB_S {
  u8*	p_head;			/* 指向缓冲区中下一个空闲位置 */
  u8*	p_tail;			/* 指向键盘任务应处理的字节 */
  int	count;			/* 缓冲区中共有多少字节 */
  u8    buf[KB_IN_BYTES];	/* 缓冲区 */
}KB_INPUT;


//
typedef struct CONSOLE_S {
  u32 start_addr;
  u32 end_addr;
  u32 current_screen_addr;	
  u32 cursor;
} CONSOLE;

typedef struct TTY_S
{
  CONSOLE * console_p;

  u32 read_buf[TTY_BUF_SIZE];   /* 用于从keyboard缓冲区拿来键位的存储 */
  u32 * head_p;
  u32 * tail_p;
  u32 buf_count;

  /* char tty_char_cache[TTY_CHAR_CACHE_SIZE]; /\* tty 的 可显示字符的缓冲 , 不过好像名字不应该叫cache*\/  */
  /* char * cache_head_p; */
  /* char * cache_tail_p; */
  /* char * cache_print_tail_p; */
  /* u32 cache_count; */


  char line_in_cache[TTY_CHAR_CACHE_SIZE];
  char * line_in_head_p;
  char * line_in_tail_p;
  u32 line_in_count;
  
  char tty_in_cache[TTY_CHAR_CACHE_SIZE];
  char * in_head_p;
  char * in_tail_p;
  u32 in_count;

  char tty_out_cache[TTY_CHAR_CACHE_SIZE];
  char * out_head_p;
  char * out_tail_p;
  u32 out_count;
  
  
} TTY;


// IPC
struct MSG_BODY_S {
  u32 arg1;
  u32 arg2;
  u32 arg3;
  u32 arg4;
  u32 arg5;
  u64 arg6;
};


typedef struct MESSAGE_S {
  u32 sender_pid;
  u32 type;
  struct MSG_BODY_S body;
  u32 is_int_msg;
} MESSAGE;


#define    MAX_FILENAME_LEN   28    /* 与 fs.h:MAX_FILENAME_LEN 相同, 
				       但是这个头文件包含fs.h感觉怪怪的，
				    */
typedef struct OPEN_FILE_S {
  int inode;
  u32 mode;			/* R, W */
  char name[MAX_FILENAME_LEN];
  u32 ext_flag;
  u32 pos;			/* 当前读写位置 */
} OPEN_FILE;

typedef struct PROCESS_S
{
  STACKFRAME stack_frame;
  u16 ldt_selector;
  DESCRIPTOR ldt[LDT_DESC_NUM];
  u32 pid;
  char name[50];
  u32 nice;               	/* 用 nice初始化ticks (linux 也叫 nice)*/
  u32 ticks;			/* 剩余 ticks 数 */
  u32 tty_num; 			/* 对应 tty  */

  MESSAGE* recv_queue[MSG_BUF_SIZE];
  MESSAGE* int_recv_queue[MSG_BUF_SIZE];
  
  u8 status;			/* sending, recving, running, pausing, hanging ,waiting */
  u32 send_pid;
  
  u32 msg_count;
  MESSAGE ** head_p;		/* 下一个空闲 */
  MESSAGE ** tail_p;		/* 下一个待处理 */

  u32 int_msg_count;		/* 中断的消息的数量 */
  MESSAGE ** int_head_p;       /* 下一个空闲 */
  MESSAGE ** int_tail_p;       /* 下一个待处理 */


  /* 当全部sizeof(OPEN_FILE)的字节是0时，表示这个结构体为空 */
  OPEN_FILE open_files[MAX_FILE_OPEN_PER_PROC];  /* 打开的文件 们 */
  //  u32 file_count;

  u32 parent_pid;		/* 父进程pid */
  int ext_flag;			/* 表明这个结构体是否占用 */
  
} PROCESS;

#endif
