#ifndef _KERNEL_FS
#define _KERNEL_FS
/*  
 +-----------------------+
 |     	               	 |
 |     	               	 |
 |      ...            	 |
 |     	               	 |
 |                     	 |
 |	               	 |
 |-----------------------|
 |                     	 |    FILE_DEFAULT_SECTOR_NR
 |   root dir          	 |
 |-----------------------|
 |                     	 |    512 * 8 *	INODE_SIZE
 |  inode array        	 |
 |-----------------------|
 |                     	 |    all_sector_num / 8 + 1
 |  sector map         	 |
 |-----------------------|
 |  inode map          	 |    1
 |-----------------------|
 |  super block        	 |    1
 |-----------------------|
 |  boot sector        	 |    1 sector
 +-----------------------+
  */
#include "xtype.h"
#include "xconst.h"

#define FILE_DEFAULT_SECTOR_NUM   512//2048
#define SECTOR_SIZE      512
#define MAX_FILENAME_LEN   28 	/* 这个指定 28 是因为：
				    DIR_ENTRY 结构体 的大小为32B时被512(一个扇区)整除,512/32 = 16,
				    另 4 B 是inode_nr(见fs.h:DIR_ENTRY)，然后一个扇区就有完整个数的目录项。
				    ！！！！这个宏在xtype.h里也有同名的宏，记得一起改。
				 */

typedef struct inode_s
{
  u32 mode;
  u32 start_sect;
  u32 file_size;
  u32 sect_nr;
  u8 _unused[16];

  // just in memory
  /* u32 dev;                    	/\* not used *\/ */
  /* u32 count;			/\* not used *\/ */
  /* u32 inode_num;		/\* not used *\/ */
} INODE;

#define INODE_SIZE (sizeof(INODE))       //-12) // 减去 只在mem的参数

/* inode.mode */
#define FILE_DIR     0
#define FILE_REGULAR 1
#define FILE_TTY     2

typedef struct superblock_s
{
  u32 magic_num;
  u32 imap_sect_nr;
  u32 smap_sect_nr;
  u32 inode_sect_nr;
  u32 first_sect;

  // just in mem
  u32 dev;
  u32 imap_p;
  u32 smap_p;
  u32 inode_p;
} SUPERBLOCK;

#define MAGIC_NUM           233
#define SUPERBLOCK_SIZE     (sizeof(SUPERBLOCK) - 4*4)



typedef struct dir_entry_s
{
  int inode_nr;
  char name [MAX_FILENAME_LEN];
} DIR_ENTRY;

#define DIR_ENTRY_SIZE    sizeof(DIR_ENTRY)
typedef struct dir_entry_cache_s
{
  DIR_ENTRY de;
  int open_count;
} DIR_ENTRY_CACHE;
#define DIR_ENTRY_CACHE_SIZE    sizeof(DIR_ENTRY_CACHE)
  
/* 根据设备的主设备号 得出对应的处理函数 */
typedef u32                  fs_handler;
#define FS_HANDLER_NUM       3

extern fs_handler fs_handlers[FS_HANDLER_NUM];   


#define MAX_OPEN_FILE_NR     128


#define HD_MAJOR             0
#define TTY_MAJOR            1



#endif
