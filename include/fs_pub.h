#ifndef _KERNEL_FS_PUB
#define _KERNEL_FS_PUB


#define FILE_NOT_FOUND        1	/* open,unlink ， 文件没找到 */
#define OPEN_TOO_MANY_FILES   2	/* open，  进程开了太多文件 */
#define CACHE_TOO_MANY_FILES  3	/* open，  fs模块的inode cache满了 */
#define FILE_NOT_OPENED       4	/* read, write, close， 文件没打开 */
#define FULL_INODE_MAP        5	/* create， inode map 满了 */
#define FILE_HAS_EXIST        6	/* create, 文件已存在 */
#define WRITE_BEYOND_MAX_SIZE 7 /* write , 写的内容太多，超过了最大大小(FILE_DEFAULT_SECTOR_NUM * SECTOR_SIZE) */
#define SOME_PROC_OPENED_IT   8 /* unlink, 有进程开着这个文件不能unlink */
#define FULL_ROOT_DIR         9 /* create, 根目录满了 */
#define FILE_HAS_OPENED       10 /* open， 文件已经打开 */
#define FULL_OF_BUF           11 /* write(tty), tty buf满了 */

#define FILE_RD               01
#define FILE_WR               02



#endif
