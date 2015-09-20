#ifndef _KERNEL_MSG
#define _KERNEL_MSG

/* 消息的类型 普通 和 中断 */
#define NORMAL_MSG       1
#define INT_MSG          2


#define MSG_get_ticks    0
#define MSG_write        MSG_get_ticks+1
#define MSG_writek       MSG_write+1

#define MSG_open         MSG_writek+1
#define MSG_read         MSG_open+1 
#define MSG_create       MSG_read+1
#define MSG_fwrite       MSG_create+1
#define MSG_close        MSG_fwrite+1
#define MSG_unlink       MSG_close+1

#define MSG_tty_read     MSG_unlink+1
#define MSG_tty_write    MSG_tty_read+1

#define MSG_keyboard     MSG_tty_write+1 /* hard int */

#define MSG_fork         MSG_keyboard+1
#define MSG_MM2FS_fork   MSG_fork+1
#define MSG_exit         MSG_MM2FS_fork+1
#define MSG_MM2FS_exit   MSG_exit+1
#define MSG_wait         MSG_MM2FS_exit+1


/* MSG_open 消息体 */
#define open_NAME            body.arg1
#define open_MODE            body.arg2
#define open_RST             body.arg3
#define open_ERRNO           body.arg4

/* MSG_read 消息体 */
#define read_INODE           body.arg1
#define read_BUF             body.arg2
#define read_NBYTES          body.arg3
#define read_RST             body.arg4
#define read_ERRNO           body.arg5

/* MSG_create */
#define create_NAME          body.arg1
#define create_RST           body.arg3
#define create_ERRNO         body.arg4


/* MSG_write */
#define write_INODE          body.arg1
#define write_BUF            body.arg2
#define write_NBYTES         body.arg3
#define write_RST            body.arg4
#define write_ERRNO          body.arg5

/* MSG_close */
#define close_INODE          body.arg1
#define close_RST            body.arg2
#define close_ERRNO          body.arg3

/* MSG_unlink */
#define unlink_NAME          body.arg1
#define unlink_RST           body.arg2
#define unlink_ERRNO         body.arg3

/* MSG_tty_read, 普通进程不会用这个 */
#define tty_read_REQUEST_M   body.arg1   /* 发起者的message， 不是指task_fs的 */
#define tty_read_TTY_NR      body.arg2
#define tty_read_RST         body.arg3
#define tty_read_ERRNO       body.arg4

/* MSG_tty_write, 普通进程不会用这个 */
#define tty_write_REQUEST_M   body.arg1   /* 发起者的message， 不是指task_fs的 */
#define tty_write_TTY_NR      body.arg2
#define tty_write_RST         body.arg3
#define tty_write_ERRNO       body.arg4

/* MSG_fork  */
#define fork_RST              body.arg1
#define fork_ERRNO            body.arg2

/* MSG_MM2FS_fork */
#define MM2FS_fork_PARPID     body.arg1
#define MM2FS_fork_KIDPID     body.arg2
#define MM2FS_fork_RST        body.arg3
#define MM2FS_fork_ERRNO      body.arg4

/* MSG_exit */
#define exit_RST              body.arg1
#define exit_ERRNO            body.arg2

/* MSG_MM2FS_exit */
#define MM2FS_exit_PARPID     body.arg1
#define MM2FS_exit_KIDPID     body.arg2
#define MM2FS_exit_RST        body.arg3
#define MM2FS_exit_ERRNO      body.arg4

/* MSG_wait */
#define wait_RST              body.arg1
#define wait_ERRNO            body.arg2

#endif
