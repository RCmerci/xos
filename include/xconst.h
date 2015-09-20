#ifndef _KERNEL_CONST
#define _KERNEL_CONST


#define BOOT_PARAM_ADDR    0x900 /* loader.inc */
#define BOOT_PARAM_MAGIC    0xB007

#define INDEX_LDT_CS 0
#define INDEX_LDT_DS 1

#define SELECTOR_LDT_CS (INDEX_LDT_CS << 3)
#define SELECTOR_LDT_DS (INDEX_LDT_DS << 3)


// 进程 & ldt
#define PROCESS_NUM  10
/* #define TASK_NUM     PROCESS_NUM */
#define TASK_NUM     9
#define LDT_NUM      PROCESS_NUM
#define LDT_SELECTOR 72 /* sizeof(STACKFRAME)  */
#define STACKTOP     LDT_SELECTOR
#define TSS_ESP0     4
#define TASK_STACK_SIZE_ALL  0x48000
#define TASK_STACK_SIZE   0x8000
#define NR_SYS_TASKS     5 		/* 这个数字前的进程属于 系统进程
					   tty,
					   syscall_proc,
					   hd,
					   fs ,
					   mm
					 */

#define GDT_DESC_NUM 128	/*  gdt 中 描述符 的 数量 */

#define IDT_GATE_NUM 255
#define LDT_DESC_NUM 2  	/* 原来是 3 ，忘了原因 */


// 具体的一些进程 pid
//#define RING1_TASK_NUM  5      ;same as NR_SYS_TASKS

#define TASK_MM   4
#define TASK_FS   3
#define TASK_HD   2
#define TASK_SYSCALL  1
#define TASK_TTY      0


//选择子 相关
#define SELECTOR_DUMMY         0
#define SELECTOR_KERNEL_CS     8
#define SELECTOR_KERNEL_DS    16
#define SELECTOR_KERNEL_VIDEO 24
#define SELECTOR_TSS          32
#define SELECTOR_FIRST_LDT    40

#define SA_TI                 4

//
#define INDEX_DUMMY         0
#define INDEX_KERNEL_CS     1
#define INDEX_KERNEL_DS     2
#define INDEX_KERNEL_VIDEO  3
#define INDEX_TSS           4
#define INDEX_FIRST_LDT     5


/* 关于进程 pid */
#define NO_PID              -1

/* color macro */
// 底色//TODO 
#define BLACK_B 0x000
#define WHITE_B 0x0F0
#define GREY_B  0x070
#define RED_B   0x040
#define BLUE_B  0x010
#define GREEN_B 0x020
#define PURPLE_B 0x050
#define BROWN_B 0x060
#define SILVER_B 0x080
#define YELLOW_B 0x0E0
//字的颜色
#define BLACK_C 0x000
#define WHITE_C 0x00F
#define GREY_C  0x007
#define RED_C   0x004
#define BLUE_C  0x001
#define GREEN_C 0x002
#define PURPLE_C 0x005
#define BROWN_C  0x006
#define SILVER_C 0x008
#define YELLOW_C 0x00E
/* color macro*/



// 中断
/* 8259A interrupt controller ports. */
#define INT_M_CTL     0x20 /* I/O port for interrupt controller       <Master> */
#define INT_M_CTLMASK 0x21 /* setting bits in this port disables ints <Master> */
#define INT_S_CTL     0xA0 /* I/O port for second interrupt controller<Slave>  */
#define INT_S_CTLMASK 0xA1 /* setting bits in this port disables ints <Slave>  */

#define EOI           0x20

#define PRIVILEGE_KRNL  0
#define PRIVILEGE_TASK  1
#define PRIVILEGE_USER  3


#define IRQ0_8259          0x20
#define IRQ8_8259          0x28

#define IRQ_HANDLER_NUM    16

//描述符类型
/* 描述符类型值说明 */
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/
#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/
/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值		*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值		*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/
/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值			*/
#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_TSS		0x89	/* 可用 386 任务状态段类型值		*/
#define	DA_CGATE		0x8C	/* 386 调用门类型值			*/
#define	DA_IGATE		0x8E	/* 386 中断门类型值			*/
#define	DA_TGATE		0x8F	/* 386 陷阱门类型值			*/

#define LIMIT_4K_SHIFT          12      /* 对应DA_LIMIT_4K */

// 系统调用
// 也用 int 80h （抄一下 linux ＝ ＝）
#define  SYSCALL_INT_VECTOR     0x80

#define SYSCALL_NUM             5        //系统调用个数（需要的时候加）

#define  SC_get_ticks           0     //SC   == syscall
#define  SC_write               1
#define  SC_writek              2
#define  SC_send                3
#define  SC_recv                4

// PIT
#define TIMER0             0x40
#define TIMER_MODE         0x43
#define RATE_GENERATOR     0x34
#define TIMER_FREQ         1193182L
#define HZ                 100 






/* 以下关于 键盘 ................. */

#define KB_8042_IN          0x60   //8042 输出缓冲区       read
#define KB_8042_OUT         0x60   // 8042 输出缓冲区      write
#define KB_8042_STATUS      0x64   // 8042 状态寄存器      read
#define KB_8042_CTRL        0x64   // 8042 控制寄存器      write


/************************************************************************/
/*                          Macros Declaration                          */
/************************************************************************/
#define	KB_IN_BYTES	32	/* size of keyboard input buffer */
#define MAP_COLS	3	/* Number of columns in keymap */
#define NR_SCAN_CODES	0x80	/* Number of scan codes (rows in keymap) */

#define FLAG_BREAK	0x0080		/* Break Code			*/
#define FLAG_EXT	0x0100		/* Normal function keys		*/
#define FLAG_SHIFT_L	0x0200		/* Shift key			*/
#define FLAG_SHIFT_R	0x0400		/* Shift key			*/
#define FLAG_CTRL_L	0x0800		/* Control key			*/
#define FLAG_CTRL_R	0x1000		/* Control key			*/
#define FLAG_ALT_L	0x2000		/* Alternate key		*/
#define FLAG_ALT_R	0x4000		/* Alternate key		*/
#define FLAG_PAD	0x8000		/* keys in num pad		*/

#define MASK_RAW	0x01FF		/* raw key value = code passed to tty & MASK_RAW
					   the value can be found either in the keymap column 0
					   or in the list below */

/* Special keys */
#define ESC		(0x01 + FLAG_EXT)	/* Esc		*/
#define TAB		(0x02 + FLAG_EXT)	/* Tab		*/
#define ENTER		(0x03 + FLAG_EXT)	/* Enter	*/
#define BACKSPACE	(0x04 + FLAG_EXT)	/* BackSpace	*/

#define GUI_L		(0x05 + FLAG_EXT)	/* L GUI	*/
#define GUI_R		(0x06 + FLAG_EXT)	/* R GUI	*/
#define APPS		(0x07 + FLAG_EXT)	/* APPS	*/

/* Shift, Ctrl, Alt */
#define SHIFT_L		(0x08 + FLAG_EXT)	/* L Shift	*/
#define SHIFT_R		(0x09 + FLAG_EXT)	/* R Shift	*/
#define CTRL_L		(0x0A + FLAG_EXT)	/* L Ctrl	*/
#define CTRL_R		(0x0B + FLAG_EXT)	/* R Ctrl	*/
#define ALT_L		(0x0C + FLAG_EXT)	/* L Alt	*/
#define ALT_R		(0x0D + FLAG_EXT)	/* R Alt	*/

/* Lock keys */
#define CAPS_LOCK	(0x0E + FLAG_EXT)	/* Caps Lock	*/
#define	NUM_LOCK	(0x0F + FLAG_EXT)	/* Number Lock	*/
#define SCROLL_LOCK	(0x10 + FLAG_EXT)	/* Scroll Lock	*/

/* Function keys */
#define F1		(0x11 + FLAG_EXT)	/* F1		*/
#define F2		(0x12 + FLAG_EXT)	/* F2		*/
#define F3		(0x13 + FLAG_EXT)	/* F3		*/
#define F4		(0x14 + FLAG_EXT)	/* F4		*/
#define F5		(0x15 + FLAG_EXT)	/* F5		*/
#define F6		(0x16 + FLAG_EXT)	/* F6		*/
#define F7		(0x17 + FLAG_EXT)	/* F7		*/
#define F8		(0x18 + FLAG_EXT)	/* F8		*/
#define F9		(0x19 + FLAG_EXT)	/* F9		*/
#define F10		(0x1A + FLAG_EXT)	/* F10		*/
#define F11		(0x1B + FLAG_EXT)	/* F11		*/
#define F12		(0x1C + FLAG_EXT)	/* F12		*/

/* Control Pad */
#define PRINTSCREEN	(0x1D + FLAG_EXT)	/* Print Screen	*/
#define PAUSEBREAK	(0x1E + FLAG_EXT)	/* Pause/Break	*/
#define INSERT		(0x1F + FLAG_EXT)	/* Insert	*/
#define DELETE		(0x20 + FLAG_EXT)	/* Delete	*/
#define HOME		(0x21 + FLAG_EXT)	/* Home		*/
#define END		(0x22 + FLAG_EXT)	/* End		*/
#define PAGEUP		(0x23 + FLAG_EXT)	/* Page Up	*/
#define PAGEDOWN	(0x24 + FLAG_EXT)	/* Page Down	*/
#define UP		(0x25 + FLAG_EXT)	/* Up		*/
#define DOWN		(0x26 + FLAG_EXT)	/* Down		*/
#define LEFT		(0x27 + FLAG_EXT)	/* Left		*/
#define RIGHT		(0x28 + FLAG_EXT)	/* Right	*/

/* ACPI keys */
#define POWER		(0x29 + FLAG_EXT)	/* Power	*/
#define SLEEP		(0x2A + FLAG_EXT)	/* Sleep	*/
#define WAKE		(0x2B + FLAG_EXT)	/* Wake Up	*/

/* Num Pad */
#define PAD_SLASH	(0x2C + FLAG_EXT)	/* /		*/
#define PAD_STAR	(0x2D + FLAG_EXT)	/* *		*/
#define PAD_MINUS	(0x2E + FLAG_EXT)	/* -		*/
#define PAD_PLUS	(0x2F + FLAG_EXT)	/* +		*/
#define PAD_ENTER	(0x30 + FLAG_EXT)	/* Enter	*/
#define PAD_DOT		(0x31 + FLAG_EXT)	/* .		*/
#define PAD_0		(0x32 + FLAG_EXT)	/* 0		*/
#define PAD_1		(0x33 + FLAG_EXT)	/* 1		*/
#define PAD_2		(0x34 + FLAG_EXT)	/* 2		*/
#define PAD_3		(0x35 + FLAG_EXT)	/* 3		*/
#define PAD_4		(0x36 + FLAG_EXT)	/* 4		*/
#define PAD_5		(0x37 + FLAG_EXT)	/* 5		*/
#define PAD_6		(0x38 + FLAG_EXT)	/* 6		*/
#define PAD_7		(0x39 + FLAG_EXT)	/* 7		*/
#define PAD_8		(0x3A + FLAG_EXT)	/* 8		*/
#define PAD_9		(0x3B + FLAG_EXT)	/* 9		*/
#define PAD_UP		PAD_8			/* Up		*/
#define PAD_DOWN	PAD_2			/* Down		*/
#define PAD_LEFT	PAD_4			/* Left		*/
#define PAD_RIGHT	PAD_6			/* Right	*/
#define PAD_HOME	PAD_7			/* Home		*/
#define PAD_END		PAD_1			/* End		*/
#define PAD_PAGEUP	PAD_9			/* Page Up	*/
#define PAD_PAGEDOWN	PAD_3			/* Page Down	*/
#define PAD_INS		PAD_0			/* Ins		*/
#define PAD_MID		PAD_5			/* Middle key	*/
#define PAD_DEL		PAD_DOT			/* Del		*/





  
/* 关于 屏幕 （字符模式的显示）  */

#define CRT_ADDR     0x3D4
#define CRT_DATA     0x3D5
#define CRT_DATA_CURSOR_LOCATION_HIGH   0x0E
#define CRT_DATA_CURSOR_LOCATION_LOW    0x0F  
#define CRT_DATA_START_ADDR_HIGH        0x0C
#define CRT_DATA_START_ADDR_LOW         0x0D

#define SCROLL_UP     0
#define SCROLL_DOWN   1

// tty , console
#define CONSOLE_NUM  3   // console数
#define TTY_NUM      3   // tty数

#define TTY_BUF_SIZE   32
#define TTY_CHAR_CACHE_SIZE  256

/* 字符模式 一共 32kb， 3个console， 第一个从 2 kb 开始，每个10kb  */
#define FIRST_CONSOLE_ADDR    0
#define CONSOLE_ADDR_LEN      10240


/* assert , panic magic number */
#define ASSERT_MAGIC         2333333
#define PANIC_MAGIC          23333333



/* ipc */
#define MSG_BUF_SIZE      32

//                                         (直到status变为pause)
//        schedule      send msg               reschedule                 schedule
// pause----------->run---------->wait_back_msg------------------->pause-------------->run
//                     \  recv msg, then handle it
//                      `---------------------------->run(pause)
// 
//      schedule        send msg
// pause---------->run---------->wait_back_msg
//    当这个进程还在等自己发出的msg时，   ^
//     有msg进来                      |
//                  ................./
// 然而， 当一个服务进程没有收到msg的时候如果时‘run’或者 ‘pause’的话好浪费cpu，
// 所以加一个‘STAT_WAIT_MSG’，这个状态在调用_recv（系统调用）时，如果待处理队列空的话被设置。
// 就是这样。

#define STAT_WAIT_MSG              0
#define STAT_WAIT_BACK_MSG         1
#define STAT_RUN          2
#define STAT_PAUSE        3
#define STAT_HANGING      4
#define STAT_WAITING      5
/* FS */
#define MAX_FILE_OPEN_PER_PROC           32

/* MM */
#define NORMAL_NICE           10

#endif
