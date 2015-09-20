#ifndef _KERNEL_HD
#define _KERNEL_HD
#include "xtype.h"
#include "xconst.h"



struct part_ent {
	u8 boot_ind;		/**
				 * boot indicator
				 *   Bit 7 is the active partition flag,
				 *   bits 6-0 are zero (when not zero this
				 *   byte is also the drive number of the
				 *   drive to boot so the active partition
				 *   is always found on drive 80H, the first
				 *   hard disk).
				 */

	u8 start_head;		/**
				 * Starting Head
				 */

	u8 start_sector;	/**
				 * Starting Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Starting
				 *   Cylinder field.
				 */

	u8 start_cyl;		/**
				 * Starting Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Starting cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u8 sys_id;		/**
				 * System ID
				 * e.g.
				 *   01: FAT12
				 *   81: MINIX
				 *   83: Linux
				 */

	u8 end_head;		/**
				 * Ending Head
				 */

	u8 end_sector;		/**
				 * Ending Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Ending
				 *    Cylinder field.
				 */

	u8 end_cyl;		/**
				 * Ending Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Ending cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u32 start_sect;	/**
				 * starting sector counting from
				 * 0 / Relative Sector. / start in LBA
				 */

	u32 nr_sects;		/**
				 * nr of sectors in partition
				 */

} PARTITION_ENTRY;

#define MAX_PRIM                4 /* 0-4 */
#define	MAX_DRIVES		2
#define	NR_PART_PER_DRIVE	4
#define	NR_SUB_PER_PART		16
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1)
#define MINOR_hd1a              16
#define MINOR_hd2a              MINOR_hd1a + NR_SUB_PER_PART


/* main drive struct, one entry per drive */
struct part_info {
	u32	base;	/* # of start sector (NOT byte offset, but SECTOR) */
	u32	size;	/* how many sectors in this partition */
};

struct hd_info
{
	int			open_cnt;
	struct part_info	primary[NR_PRIM_PER_DRIVE];
	struct part_info	logical[NR_SUB_PER_DRIVE];
};


#define HD_IRQ_NUM     14	/* 硬盘的中断号 */

#define REG_DATA       0x1f0	/* IO */
#define REG_ERROR      0x1f1	/* I */
#define REG_FEATURES   REG_ERROR /* O */
#define REG_SECTOR_COUNT  0x1f2	 /* IO */
#define REG_LBA_LOW    0x1f3	 /* IO */
#define REG_LBA_MID    0x1f4     /* IO */
#define REG_LBA_HIGH   0x1F5	 /* IO */
#define REG_DEVICE     0x1f6	 /* IO */
#define REG_STATUS     0x1f7	 /* I */
#define REG_COMMAND    REG_STATUS /* O */
#define REG_ALTERNATE_STATUS  0x3f6 /* I */
#define REG_DEVICE_CONTROL    REG_ALTERNATE_STATUS /* O */



#define MSG_HD     0 		/* 硬盘中断程序发的信号类型 */

/* 普通消息类型 */
#define HD_OPEN    0	
#define HD_CLOSE   1
#define HD_READ    2
#define HD_WRITE   3
#define HD_IOCTL   4

typedef struct hd_cmd_s
{
  u8	features;
  u8	count;
  u8	lba_low;
  u8	lba_mid;
  u8	lba_high;
  u8	device;
  u8	command;
}HD_CMD;


#define STATUS_DRQ              0x08
#define STATUS_BSY              0x80
#define	HD_TIMEOUT		10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE
#define ATA_IDENTIFY		0xEC
#define ATA_READ		0x20
#define ATA_WRITE		0x30
/* for DEVICE register. */
#define	MAKE_DEVICE_REG(lba,drv,lba_highest) (((lba) << 6) |		\
					      ((drv) << 4) |		\
					      (lba_highest & 0xF) | 0xA0)



#define P_PRIMARY     0		/* 主分区 */
#define P_EXTENDED    1		/* 扩展分区 */

#define NO_PART       0   	/* 分区表 空的条目  ( 应该是 0 把)*/
#define EXT_PART      5       	/* extended （扩展分区）*/


/* 关于 read write 消息的 arg 宏 */
/* msg.type == HD_READ or HD_WRITE */
#define POSITION    body.arg6  	/* u64 */
#define DEVICE      body.arg1
#define CNT         body.arg2
#define BUF         body.arg3

/* 关于 ioctl 消息的宏 */
#define IOCTL_BASE body.arg1
#define IOCTL_SIZE body.arg2

/* 设备号 : 0x110  --> major : 0x1, minor : 0x10
                 ---> 对应TASK_HD,         hd1a
*/
#define MAJOR(dev)                 (((u32)dev>>31)&0xF)
#define MINOR(dev)                 (dev&0xffff)
#define MAKE_DEV(major, minor)     ((major<<31)|minor)

/* 这里暂时设定是第2个分区的第一个逻辑分区作为ROOT  */
#define HD_ROOT_MINOR              MINOR_hd2a                  
#define HD_ROOT_DEV                (MAKE_DEV(TASK_HD, MINOR_hd2a))



/* misc */

#define SECTOR_SIZE    512
#define SECTOR_SIZE_SHIFT     9

#endif
