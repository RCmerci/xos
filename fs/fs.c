#include "xconst.h"
#include "fs.h"
#include "hd.h"
#include "syscall.h"
#include "xglobal.h"
#include "xtype.h"
#include "common.h"
#include "message.h"
#include "fs_pub.h"

static void hd_read(u32 sect, u32 len);
static void hd_write(u32 sect, u32 len);
static void mkfs();
static int get_inode_nr(char * name);
static int del_cache_inode(int inode_nr);

/* fs_xxxx 返回值是请求进程是否继续block  */
static int fs_open(MESSAGE * m);
static int fs_read(MESSAGE * m);
static int fs_create(MESSAGE * m);
static int fs_write(MESSAGE * m);
static int fs_close(MESSAGE * m);
static int fs_unlink(MESSAGE * m);

static int fs_mm2fs_fork(MESSAGE * m);
static int fs_mm2fs_exit(MESSAGE * m);

fs_handler fs_handlers[FS_HANDLER_NUM];

static u8 fsbuf [SECTOR_SIZE * 10];
static SUPERBLOCK sb;
static INODE      root_dir_inode;
/* static u8 * imap_p; */
/* static u8 * smap_p; */
/* static INODE * inode_p; */
/* static DIR_ENTRY * root_dir_p; */

/* 文件系统的cache， 打开的文件inode_nr放在这里 */
static DIR_ENTRY_CACHE opened_file[MAX_OPEN_FILE_NR];
static int       opened_count;

int fs_errno = 0;	/* just for debug */

#define CONSOLE_P_LEGAL(console_p)   (console_p >= console_table && \
				      console_p <= console_table+CONSOLE_NUM && \
				      ((void*)console_table-(void*)console_p)%sizeof(CONSOLE)==0)
#define IS_TTY_INODE(inode_nr)       (inode_nr > 0 && \
				      inode_nr <= TTY_NUM)



void fs_main()
{
  while(1) {
    MESSAGE * start_m = recv(NORMAL_MSG);
    if (start_m->body.arg1 == 233) {
      unblock_process(start_m->sender_pid);
      break;
    }
  }
  MESSAGE m;
  m.type = HD_OPEN;
  m.is_int_msg = 0;
  send(&m, TASK_HD); 

  mkfs();	     
  
  opened_count = CONSOLE_NUM;	

  fs_init_done = 1;		/* fs_task 初始化完成 */
  
  int block_flag = 0;
  while(1) {
    MESSAGE * m = recv(NORMAL_MSG);
    switch (m->type) {
    case MSG_open:
      block_flag = fs_open(m);
      break;
    case MSG_read:
      block_flag = fs_read(m);
      break;
    case MSG_create:
      block_flag = fs_create(m);
      break;
    case MSG_fwrite:
      block_flag = fs_write(m);
      break;
    case MSG_close:
      block_flag = fs_close(m);
      break;
    case MSG_unlink:
      block_flag = fs_unlink(m);
      break;
    case MSG_MM2FS_fork:
      block_flag = fs_mm2fs_fork(m);
      break;
    case MSG_MM2FS_exit:
      block_flag = fs_mm2fs_exit(m);
      break;
    default:
      break;
    }
    if (block_flag) continue;
    unblock_process(m->sender_pid);
  }
}

/* 初始化硬盘 */
void mkfs()
{
  /* init fs_handlers */
  fs_handlers[HD_MAJOR] = TASK_HD;
  fs_handlers[TTY_MAJOR] = TASK_TTY;
  /* superblock */
  MESSAGE m;
  m.type = HD_IOCTL;
  m.DEVICE = HD_ROOT_MINOR;
  m.is_int_msg = 0;
  send(&m, TASK_HD);
  
  SUPERBLOCK * tmp_sb = (SUPERBLOCK*)fsbuf;
  memset(tmp_sb, 0, SECTOR_SIZE);
  tmp_sb->magic_num = MAGIC_NUM;
  tmp_sb->imap_sect_nr = 1;
  tmp_sb->smap_sect_nr = (m.IOCTL_SIZE + (SECTOR_SIZE*8) - 1)/(SECTOR_SIZE*8);
  tmp_sb->inode_sect_nr = tmp_sb->imap_sect_nr * SECTOR_SIZE * 8 * INODE_SIZE / SECTOR_SIZE; // 8 -> 1byte->8bits
  tmp_sb->first_sect = 1 + 1 + tmp_sb->smap_sect_nr + tmp_sb->inode_sect_nr; // 1+1 -> superblock, imap_sect
  hd_write(1, SECTOR_SIZE); 	/* 1->boot sect */

  xmemcpy(&sb, tmp_sb, SUPERBLOCK_SIZE);
  sb.dev = 0;
  sb.imap_p = 1 + 1;
  sb.smap_p = sb.imap_p + sb.imap_sect_nr;
  sb.inode_p = sb.smap_p + sb.smap_sect_nr;
  //  return;			/* TODO :!!!!!!!!!!!!!!!!!!!!delete!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  printk(0, "superblock init done\n");
  
  /* inode map */
  u8 * imap = (u8*)fsbuf;
  memset(imap, 0, SECTOR_SIZE);
  int i;
  for (i=0;i<CONSOLE_NUM + 1;i++) { /* 1 -> root dir */
    imap[0] |= 1<<i;
  }
  hd_write(2, SECTOR_SIZE);	/* 2 -> boot sect + superblock */
  printk(0, "inode map init done\n");

  /* sect map */
  u8 * smap = (u8*)fsbuf;
  
  int default_file_sect_nr = FILE_DEFAULT_SECTOR_NUM; /* for / (root dir)*/
  memset(smap, 0, SECTOR_SIZE);
  for (i=0;i<default_file_sect_nr/8;i++) {
    smap[i] = 0xFF;
  }
  hd_write(2 + sb.imap_sect_nr, SECTOR_SIZE);
  
  memset(smap, 0, SECTOR_SIZE);
  for (i=0;i<sb.smap_sect_nr-1;i++) { /* sect map  其他地方都是 0 */
    hd_write(2 + sb.imap_sect_nr + i + 1, SECTOR_SIZE);
  }
  printk(0, "sect map init done\n");

  /* inode array */

  /* ----> root dir inode */
  INODE * root_dir_node = (INODE*)fsbuf;
  memset(fsbuf, 0, SECTOR_SIZE);
  
  root_dir_node->mode = FILE_DIR;
  root_dir_node->start_sect = sb.first_sect;
  root_dir_node->file_size  = DIR_ENTRY_SIZE * 4; /* 4 -> 3 consoles + ./  */
  root_dir_node->sect_nr = FILE_DEFAULT_SECTOR_NUM; /* 文件默认都这么大 ->1MB*/
  /* ----> tty 0-2 */
  for (i=0;i<CONSOLE_NUM;i++) {
    INODE * console_inode = (INODE*)(root_dir_node + 1 + i); /* 1 -> root_dir inode */
    console_inode->mode = FILE_TTY;
    console_inode->start_sect = MAKE_DEV(TTY_MAJOR, i);
    console_inode->file_size = 0;
    console_inode->sect_nr = 0;
  }
  hd_write(2 + sb.imap_sect_nr + sb.smap_sect_nr, SECTOR_SIZE);
  
  memset(fsbuf, 0, SECTOR_SIZE);
  for (i=0;i<sb.inode_sect_nr-1;i++) {
    hd_write(2 + sb.imap_sect_nr + sb.smap_sect_nr + i + 1, SECTOR_SIZE);
  }
  printk(0, "inode array init done\n");

  /* root dir */
  DIR_ENTRY * dir_entry = (DIR_ENTRY *)fsbuf;
  memset(fsbuf, 0, SECTOR_SIZE);
  dir_entry[0].inode_nr = 0;
  sprintf(dir_entry[0].name, ".");

  for (i=1;i<=CONSOLE_NUM;i++) {
    dir_entry[i].inode_nr = i;
    sprintf(dir_entry[i].name, "tty%d", i-1);
  }

  hd_write(sb.first_sect, SECTOR_SIZE);
  printk(0, "root dir init done\n");
}




static int get_inode(int inode_nr, INODE * buf)
{
  int sect_nr = inode_nr * INODE_SIZE / SECTOR_SIZE;
  int index   = inode_nr - sect_nr * SECTOR_SIZE / INODE_SIZE;
  hd_read(sb.inode_p + sect_nr, SECTOR_SIZE);
  xmemcpy(buf, (void*)fsbuf + index * INODE_SIZE, INODE_SIZE);
  return 0;
}

static int put_inode(int inode_nr, INODE * buf)
{
  int sect_nr = inode_nr * INODE_SIZE / SECTOR_SIZE;
  int index   = inode_nr - sect_nr * SECTOR_SIZE / INODE_SIZE;
  hd_read(sb.inode_p + sect_nr, SECTOR_SIZE);
  xmemcpy((void*)fsbuf + index * INODE_SIZE, buf, INODE_SIZE);
  hd_write(sb.inode_p + sect_nr, SECTOR_SIZE);
  return 0;
}


/* 
   得到root dir 的inode， 现在是每次请求都从hd读，但是可以cache。以后再说
 */
static INODE * get_root_dir_inode()
{
  hd_read(sb.inode_p, SECTOR_SIZE);
  xmemcpy(&root_dir_inode, fsbuf, INODE_SIZE);
  return &root_dir_inode;
}

/* 
   在 [buf, buf+len)的地址查找空的DIR_ENTRY位置
   len -> 指字节数，不是 DIR_ENTRY 个数。
 */
static DIR_ENTRY * search_empty_dir_entry(DIR_ENTRY * buf, int len)
{
  DIR_ENTRY * tmp_p = buf;
  while((void*)tmp_p+DIR_ENTRY_SIZE < (void*)buf+len) {
    if (tmp_p->inode_nr == 0 && ! strcmp(tmp_p->name, "")) {
      return tmp_p;
    }
    tmp_p ++ ;
  }
  return NULL;
}


/* @return 
   inode
   -1 -> not found
*/
static int get_inode_nr(char * name)
{
  int i;
  INODE * root_dir_inode_p = get_root_dir_inode();
  int loop_sect_num = (root_dir_inode_p->file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
  for (i=0 ; i<loop_sect_num ; i++) { // 把root dir加载到内存
    hd_read(sb.first_sect + i, SECTOR_SIZE);
  
  DIR_ENTRY * root_dir_p = (DIR_ENTRY*)fsbuf;
  DIR_ENTRY * tmp_p = root_dir_p;
  while(1) {
    if (tmp_p >= root_dir_p + SECTOR_SIZE) break; /* 这个sect遍历完了 */
    /* if (tmp_p->inode_nr == 0 && strcmp(tmp_p->name, ".")) {	/\* 普通文件inode从1开始 *\/ */
    /*   return -1; */
    /* } */
    if (!strcmp(tmp_p->name, name)) {
      return tmp_p->inode_nr;
    }
    tmp_p ++ ;
  }
  }
  return -1;
}

/* 删除root dir里的指定 inode_nr 的 Dir Entry */
static int unset_root_dir(int inode_nr)
{
  int i;
  for (i=0;i<FILE_DEFAULT_SECTOR_NUM;i++) {
    hd_read(sb.first_sect + i, SECTOR_SIZE);

    DIR_ENTRY * root_dir_p = (DIR_ENTRY*)fsbuf;
    DIR_ENTRY * tmp_p = root_dir_p;
    while(1) {
      if (tmp_p >= root_dir_p + SECTOR_SIZE) break; /* 这个sect 遍历完了 */
      if (tmp_p->inode_nr == inode_nr) {
	memset(tmp_p, 0, DIR_ENTRY_SIZE);
	hd_write(sb.first_sect + i, SECTOR_SIZE);
	return 0;
      }
      tmp_p ++ ;
    }
  }
  return -1;
}

/* @return 
   0 -> success
   -1 -> fail
*/
static int add_cache_inode (int inode_nr, char * name)
{
  assert(opened_count < MAX_OPEN_FILE_NR);

  int i=0;
  
  while(1) {
    if (i == opened_count) {	/* 前 opened_count+1 个里一定有空位 */
      return -1;
    }
    
    if (strcmp(opened_file[i].de.name, "")==0 &&
	opened_file[i].de.inode_nr == 0) { /* 空的 */
      strcpy(name, opened_file[i].de.name);
      opened_file[i].de.inode_nr = inode_nr;
      opened_file[i].open_count = 1;
      break;
    }
    i++;
  }
  opened_count ++ ;
  return 0;
}

static int incr_cache_inode_count(int inode_nr)
{
  int i = 0;
  while(1) {
    if (i == MAX_OPEN_FILE_NR) {
      return -1;
    }
    if (opened_file[i].de.inode_nr == inode_nr) {
      assert(opened_file[i].open_count > 0);
      opened_file[i].open_count += 1;
      return 0;
    }
    i++;
  }
}

static int desc_cache_inode_count(int inode_nr)
{
  int i = 0;
  while(1) {
    if (i == MAX_OPEN_FILE_NR) {
      return -1;
    }
    if (opened_file[i].de.inode_nr == inode_nr) {
      opened_file[i].open_count -= 1;
      if (opened_file[i].open_count == 0) { /* 没有哪个进程开着这个文件了 */
	del_cache_inode(inode_nr);
      }
      return 0;
    }
    i++;
  }
}


static int del_cache_inode (int inode_nr)
{
  assert(opened_count > 0);
  assert(inode_nr > 0);
  
  int i=0;
  
  while(1) {
    if (i == MAX_OPEN_FILE_NR) {
      return -1;
    }
    if (opened_file[i].de.inode_nr == inode_nr) { /* 找到 */
      assert(opened_file[i].open_count == 0);
      memset(&opened_file[i], 0, DIR_ENTRY_CACHE_SIZE);
      break;
    }
    i++;
  }
  opened_count--;
  return 0;
}


/* @return
   -1 -> fail
---------------------
XXXXXX用完这个函数后，要加到pcb之前记得把 对应的open_count++
 */
static int search_cache_inode (char * name)
{
  assert(opened_count >= 0);
  assert(opened_count <= MAX_OPEN_FILE_NR);

  int i=0;
  while(1) {
    if (i == MAX_OPEN_FILE_NR) {
      return -1;
    }
    if (! strcmp(opened_file[i].de.name,name)) { /* 找到 */
      return opened_file[i].de.inode_nr;
    }
    i ++ ;
  }
}

/* @return
   -1 -> 满了
 */
static int add_file_in_pcb(int inode_nr, u32 mode, char * name, PROCESS * proc)
{
  int i = 0;
  while(1) {
    if (i == MAX_FILE_OPEN_PER_PROC) { /* 满了 */
      return -1;
    }
    if (! proc->open_files[i].ext_flag) { 
      (proc->open_files)[i].inode = inode_nr;
      (proc->open_files)[i].mode = mode;
      strcpy(name, (proc->open_files)[i].name);
      (proc->open_files)[i].pos = 0;
      (proc->open_files)[i].ext_flag = 1;
      return 0;
    }
    i++;
  }
}

/* @reutrn 
   -1 -> not found
 */
static int del_file_in_pcb(int inode_nr, PROCESS * proc)
{
  int i=0;
  while (1) {
    if (i == MAX_FILE_OPEN_PER_PROC) { /* not found */
      return -1;
    }
    if (proc->open_files[i].ext_flag == 0) { /* 空的 */
      i++;
      continue;
    }
    if ((proc->open_files)[i].inode == inode_nr) {
      proc->open_files[i].ext_flag = 0;
      //      memset(&((proc->open_files)[i]), 0, sizeof(OPEN_FILE));
      return 0;
    }
    i++;
  }
}

static OPEN_FILE * search_file_in_pcb(char * name, int inode_nr, PROCESS * proc)
{
  int i=0;
  if (name != NULL) {
    while(1) {
      if (i == MAX_FILE_OPEN_PER_PROC) {
	return 0;
      }
      if (! proc->open_files[i].ext_flag) { /* 这个位置为空 */
	i++;
	continue;
      }
      if (! strcmp((proc->open_files)[i].name, name)) {
	return &((proc->open_files)[i]);
      }
      i++;
    }
  }else { 			/* search by "inode_nr" */
    while(1) {
      if (i == MAX_FILE_OPEN_PER_PROC) {
	return NULL;
      }
      if (! proc->open_files[i].ext_flag) { /* 这个位置为空 */
	i++;
	continue;
      }
      if ((proc->open_files)[i].inode == inode_nr) {
	if (inode_nr == 0 && strcmp((proc->open_files)[i].name, ".")) { /* inode=0, name!="." */
	  continue;
	}
	return &((proc->open_files)[i]);
      }
      i++;
    }

  }
}

/* 找一个空的inode
   @return 
   -1 -> inode全都被占据了
   inode
 */
static int search_empty_inode_slot()
{
  hd_read(sb.imap_p, SECTOR_SIZE);
  u8 * imap_p = (u8*)fsbuf;
  int i = 0, j;
  while(i < SECTOR_SIZE / 8) {
    for (j=0;j<8;j++) {
      if (!((imap_p[i] >> j) & 0x01)) return (8 * i + j);
    }
    i++;
  }
  return -1; 			/* 满了 */
}


/* 
   inode map 置位
 */
static int set_inode_map(int nr)
{
  hd_read(sb.imap_p, SECTOR_SIZE);
  int byte_num = nr / 8;
  int bit_num  = nr % 8;
  u8 tmp = fsbuf[byte_num];
  fsbuf[byte_num] = tmp | (0x01<<bit_num);
  hd_write(sb.imap_p, SECTOR_SIZE);
  return 0;
}

static int unset_inode_map(int nr)
{
  hd_read(sb.imap_p, SECTOR_SIZE);
  int byte_num = nr / 8;
  int bit_num  = nr % 8;
  u8 tmp = fsbuf[byte_num];
  fsbuf[byte_num] = tmp & ~(0x01<<bit_num);
  hd_write(sb.imap_p, SECTOR_SIZE);
  return 0;
}

static int set_sect_map(u32 start_sect, u32 sect_num)
{
  u32 smap_start_sect = (start_sect - sb.first_sect) / (8 * SECTOR_SIZE);
  u32 start_byte_index = ((start_sect - sb.first_sect) - (smap_start_sect * 8 * SECTOR_SIZE)) / 8;
  u32 start_bit_index  = ((start_sect - sb.first_sect) - (smap_start_sect * 8 * SECTOR_SIZE)) % 8;
  u32 smap_end_sect   = (start_sect + sect_num - sb.first_sect) / (8 * SECTOR_SIZE);
  u32 end_byte_index = ((start_sect + sect_num - sb.first_sect) - (smap_end_sect * 8 * SECTOR_SIZE)) / 8;
  u32 end_bit_index  = ((start_sect + sect_num - sb.first_sect) - (smap_end_sect * 8 * SECTOR_SIZE)) % 8;

  u32 curr_sect = smap_start_sect;
  u32 curr_start_byte_index;
  u32 curr_start_bit_index;
  u32 curr_end_byte_index;
  u32 curr_end_bit_index;
  while(curr_sect >= smap_start_sect && curr_sect <= smap_end_sect) {
    if (curr_sect == smap_start_sect) {
      curr_start_byte_index = start_byte_index;
      curr_start_bit_index  = start_bit_index;
    } else {
      curr_start_byte_index = 0;
      curr_start_bit_index  = 0;
    }
    if (curr_sect == smap_end_sect) {
      curr_end_byte_index = end_byte_index;
      curr_end_bit_index  = end_bit_index;
    } else {
      curr_end_byte_index = SECTOR_SIZE;
      curr_end_bit_index  = 0;
    }
    
    hd_read(sb.smap_p + curr_sect, SECTOR_SIZE);
    u8 * smap_p = (u8*)fsbuf;
    u32 byte_index = curr_start_byte_index;
    u32 start_bit_index  = curr_start_bit_index;
    if (byte_index == curr_end_byte_index) {
      *(smap_p + byte_index) |= gen_byte(curr_start_bit_index, curr_end_bit_index-1);
      byte_index ++ ;
    } else {
      *(smap_p + byte_index) |= gen_byte(curr_start_bit_index, 7);
      byte_index ++ ;
      while(byte_index < curr_end_byte_index) {
	*(smap_p + byte_index) = 0xFF;
	byte_index ++ ;
      }
      *(smap_p + byte_index) |= gen_byte(0, curr_end_bit_index-1);
    }
    hd_write(sb.smap_p + curr_sect, SECTOR_SIZE);
    curr_sect ++ ;
  }
}

static int unset_sect_map(u32 start_sect, u32 sect_num)
{
  u32 smap_start_sect = (start_sect - sb.first_sect) / (8 * SECTOR_SIZE);
  u32 start_byte_index = ((start_sect - sb.first_sect) - (smap_start_sect * 8 * SECTOR_SIZE)) / 8;
  u32 start_bit_index  = ((start_sect - sb.first_sect) - (smap_start_sect * 8 * SECTOR_SIZE)) % 8;
  u32 smap_end_sect   = (start_sect + sect_num - sb.first_sect) / (8 * SECTOR_SIZE);
  u32 end_byte_index = ((start_sect + sect_num - sb.first_sect) - (smap_end_sect * 8 * SECTOR_SIZE)) / 8;
  u32 end_bit_index  = ((start_sect + sect_num - sb.first_sect) - (smap_end_sect * 8 * SECTOR_SIZE)) % 8;

  u32 curr_sect = smap_start_sect;
  u32 curr_start_byte_index;
  u32 curr_start_bit_index;
  u32 curr_end_byte_index;
  u32 curr_end_bit_index;
  while(curr_sect >= smap_start_sect && curr_sect <= smap_end_sect) {
    if (curr_sect == smap_start_sect) {
      curr_start_byte_index = start_byte_index;
      curr_start_bit_index  = start_bit_index;
    } else {
      curr_start_byte_index = 0;
      curr_start_bit_index  = 0;
    }
    if (curr_sect == smap_end_sect) {
      curr_end_byte_index = end_byte_index;
      curr_end_bit_index  = end_bit_index;
    } else {
      curr_end_byte_index = SECTOR_SIZE;
      curr_end_bit_index  = 0;
    }
    
    hd_read(sb.smap_p + curr_sect, SECTOR_SIZE);
    u8 * smap_p = (u8*)fsbuf;
    u32 byte_index = curr_start_byte_index;
    u32 start_bit_index  = curr_start_bit_index;
    if (byte_index == curr_end_byte_index) {
      *(smap_p + byte_index) &= ~(gen_byte(curr_start_bit_index, curr_end_bit_index-1));
      byte_index ++ ;
    } else {
      *(smap_p + byte_index) &= ~(gen_byte(curr_start_bit_index, 7));
      byte_index ++ ;
      while(byte_index < curr_end_byte_index) {
	*(smap_p + byte_index) = 0x0;
	byte_index ++ ;
      }
      *(smap_p + byte_index) &= ~(gen_byte(0, curr_end_bit_index-1));
    }
    hd_write(sb.smap_p + curr_sect, SECTOR_SIZE);
    curr_sect ++ ;
  }
}

/* 找一连串空的sector。
   因为每个文件默认都占据了 FILE_DEFAULT_SECTOR_NUM 个扇区。
   所以默认找到一个空的sector代表后面FILE_DEFAULT_SECTOR_NUM－1个扇区都是空的。
 */
static int search_empty_sect()
{
}


static int get_empty_sect(int inode_nr)
{
  return sb.first_sect + (inode_nr - CONSOLE_NUM) * FILE_DEFAULT_SECTOR_NUM;
}

static int init_inode(int inode_nr, INODE * buf)
{
  int inode_sect = inode_nr / (SECTOR_SIZE / INODE_SIZE) + sb.inode_p;
  int inode_index = inode_nr % (SECTOR_SIZE / INODE_SIZE);
  hd_read(inode_sect, SECTOR_SIZE);
  INODE * inode_p = (INODE*)((void*)fsbuf + inode_index * INODE_SIZE);
  inode_p->mode = FILE_REGULAR;
  inode_p->start_sect = get_empty_sect(inode_nr);
  inode_p->file_size = 0;
  inode_p->sect_nr = FILE_DEFAULT_SECTOR_NUM;
  hd_write(inode_sect, SECTOR_SIZE);

  xmemcpy(buf, inode_p, INODE_SIZE);
  return 0;
}
/* ----------------------------------------------------- */



static int fs_open(MESSAGE * m)
{
  int inode_nr;
  char * name = va2la(m->sender_pid, (char*)m->open_NAME);
  u32 mode    = m->open_MODE;
  PROCESS * proc = get_process(m->sender_pid);

  if (NULL != search_file_in_pcb(name, 0, proc)) { /* 已经打开 */
    m->open_RST = -1;
    m->open_ERRNO = FILE_HAS_OPENED;
    return 0;
  }
  
  if (-1 == (inode_nr = search_cache_inode(name))) { //not found in cache
    if (-1 == (inode_nr = get_inode_nr(name))) { // not found in hd
      m->open_ERRNO = FILE_NOT_FOUND;
      m->open_RST   = -1;
      return 0;
    }
    if (-1 == add_cache_inode(inode_nr, name)) { /* cache 满了 */
      m->open_ERRNO = CACHE_TOO_MANY_FILES;
      m->open_RST   = -1;
      return 0;
    }
  } else {
    incr_cache_inode_count(inode_nr);
  }
  /* FOUND file inode  */



  if (-1 == add_file_in_pcb(inode_nr, mode, name, proc)) {
    /* target process open too many files */
    m->open_ERRNO = OPEN_TOO_MANY_FILES;
    m->open_RST = -1;
    return;
  } else {			/* succ */
    m->open_ERRNO = 0;
    m->open_RST = inode_nr;
  }
  return 0;
}


/* 
   读 nbytes 放入 buf
   @return
   
 */
static int fs_read(MESSAGE * m)
{
  int inode_nr = m->read_INODE;
  assert(m->sender_pid != -1);
  u8 * buf     = va2la(m->sender_pid, (u8*)m->read_BUF);
  int nbytes   = m->read_NBYTES;
  u32 pos      = -1;		/* to be init */
  PROCESS * proc = get_process(m->sender_pid);
  //  PROCESS * proc = &process_table[m->sender_pid]; /* 请求进程 */
  OPEN_FILE * file_in_pcb;

  
  INODE inode;
  if (inode_nr == -1) {		/* call from scanf之类, write to stdout */
    CONSOLE * console_p = tty_table[proc->tty_num].console_p;
    if (!CONSOLE_P_LEGAL(console_p)) {
      m->write_RST = 0;
      return 0;
    }
    inode_nr = MAKE_DEV(TTY_MAJOR, proc->tty_num+1); /* 0=>root inode */
    /* inode_nr = proc->tty_num + 1; /\* 1=>root inode *\/ */
  } else if (IS_TTY_INODE(inode_nr)) {
    inode_nr = MAKE_DEV(TTY_MAJOR, inode_nr);
  } else {
    get_inode(inode_nr, &inode);
  }

  
  
  if (NULL == (file_in_pcb = search_file_in_pcb(0, MINOR(inode_nr), proc))) { /* 文件没打开 */
    m->read_ERRNO = FILE_NOT_OPENED;
    m->read_RST   = -1;
    return 0;
  }
  //else if (MAJOR(inode.start_sect) == TTY_MAJOR) { /* tty read */
  else if (MAJOR(inode_nr) == TTY_MAJOR) {
    MESSAGE tty_m;
    tty_m.type = MSG_tty_read;
    tty_m.is_int_msg = 0;
    tty_m.tty_read_REQUEST_M = (u32)m;
    //    tty_m.tty_read_TTY_NR = MINOR(inode.start_sect);
    tty_m.tty_read_TTY_NR = MINOR(inode_nr) - 1;
    send(&tty_m, TASK_TTY);
    return 1;			/* 请求进程继续block, 由tty_task 负责唤醒 */
    
  } else {			/* init pos */
    pos = file_in_pcb->pos;
  }

  if (pos + nbytes > inode.file_size)  /* 要读的字节大于文件大小 */
    nbytes = inode.file_size - pos;

  m->read_RST = nbytes;

  assert(nbytes>=0);

  
  int first_read_sect  = inode.start_sect + pos/SECTOR_SIZE;
  int first_read_index = pos - SECTOR_SIZE * (first_read_sect - inode.start_sect);
  int last_read_sect   = inode.start_sect + (pos + nbytes) / SECTOR_SIZE;
  int last_read_index  = pos + nbytes - SECTOR_SIZE * (last_read_sect - inode.start_sect);
  /* last index 不读 */

  
  int curr_sect = first_read_sect;
  int curr_index = first_read_index;
  int start_index, end_index;
  while(curr_sect >= first_read_sect && curr_sect <= last_read_sect) {
    if (curr_sect == first_read_sect) start_index = first_read_index;
    else                              start_index = 0;
    if (curr_sect == last_read_sect)  end_index   = last_read_index;
    else                              end_index   = SECTOR_SIZE;

    hd_read(curr_sect, SECTOR_SIZE);
    xmemcpy(buf, fsbuf+start_index, end_index - start_index);
    buf += end_index - start_index;
    curr_sect += 1;
  }

  file_in_pcb->pos += nbytes;	/* increase seek position in PCB */
  return 0;
}

static int fs_create(MESSAGE * m)
{
  char * name = va2la(m->sender_pid, (char*)m->create_NAME);
  int inode_nr;

  if (-1 != search_cache_inode(name) || -1 != get_inode_nr(name)) { /* 已经存在 */
    m->create_RST = -1;
    m->create_ERRNO = FILE_HAS_EXIST;
    return 0;
  }
  
  if(-1 == (inode_nr = search_empty_inode_slot())) { /* inode map 满了 */
    m->create_ERRNO = FULL_INODE_MAP;
    m->create_RST = -1;
    return 0;
  }
  set_inode_map(inode_nr);
  INODE inode;
  init_inode(inode_nr, &inode);
  set_sect_map(inode.start_sect, FILE_DEFAULT_SECTOR_NUM);
  
  /* 在目录加上这个文件 */
  DIR_ENTRY write_buf;;
  write_buf.inode_nr = inode_nr;
  strcpy(name, write_buf.name);
  MESSAGE write_m;
  write_m.write_INODE = 0;   /* 0 -> root dir inode_nr */
  write_m.write_BUF   = (u32)&write_buf;
  write_m.write_NBYTES = DIR_ENTRY_SIZE;
  write_m.sender_pid = -1;	/* no sender */
  fs_write(&write_m);


  /* 创建后打开文件 */
  MESSAGE open_m;
  open_m.open_NAME = (u32)m->create_NAME; /* 因为地址转换的关系，这里要用最初的地址，不是va2la之后的 */
  open_m.open_MODE = FILE_RD | FILE_WR;
  open_m.sender_pid = m->sender_pid;
  fs_open(&open_m);		
  u32 rst = open_m.open_RST;
  u32 errno = open_m.open_ERRNO;
  m->create_RST = rst;
  m->create_ERRNO = errno;
  return 0;
}


static int fs_write(MESSAGE * m)
{
  int inode_nr = m->write_INODE;
  u8 * buf     = m->sender_pid == -1? (u8*)m->write_BUF : va2la(m->sender_pid, (u8*)m->write_BUF);
  int nbytes   = m->write_NBYTES;
  int pos      = -1;		/* to be init */
  PROCESS * proc = m->sender_pid == -1 ? NULL : get_process(m->sender_pid);
  OPEN_FILE * file_in_pcb = NULL;

  INODE inode;
  if (inode_nr == -1) {		/* call from printf, write to stdout */
    CONSOLE * console_p = tty_table[proc->tty_num].console_p;
    if (!CONSOLE_P_LEGAL(console_p)) {
      m->write_RST = 0;
      return 0;
    }
    inode_nr = MAKE_DEV(TTY_MAJOR, proc->tty_num+1); /* 1=>root inode */
    /* inode_nr = proc->tty_num + 1; /\* 1=>root inode *\/ */
  } else if (IS_TTY_INODE(inode_nr)) {
    inode_nr = MAKE_DEV(TTY_MAJOR, inode_nr);
  } else {
    get_inode(inode_nr, &inode);
  }

  if (proc) {			/* 有对应proc */
    if (NULL == (file_in_pcb = search_file_in_pcb(0, MINOR(inode_nr), proc))) {
      m->write_ERRNO = FILE_NOT_OPENED;
      m->write_RST   = -1;
      return 0;
    } else {
      /* if (MAJOR(inode.start_sect) == TTY_MAJOR) { */
      if (MAJOR(inode_nr) == TTY_MAJOR) {
	MESSAGE tty_m;
	tty_m.type = MSG_tty_write;
	tty_m.is_int_msg = 0;
	tty_m.tty_write_REQUEST_M = (u32)m;
	tty_m.tty_write_TTY_NR = MINOR(inode_nr) - 1; /* 1->root inode ,, MINOR(tty0_nr)->1*/
	send(&tty_m, TASK_TTY);
	return 1;               /* 请求进程继续block, 由tty_task 负责唤醒 */
      }
      else {
	pos = file_in_pcb->pos;
      }
    }
  } else {			/* 根目录 添加 文件 的情况*/
    int root_dir_inode_nr;
    if (-1 == (root_dir_inode_nr = search_cache_inode("."))) {
      if (-1 == (root_dir_inode_nr = get_inode_nr("."))) {
	assert("can't find root dir inode."==0);
      }
    }
    //    INODE * root_dir_inode_p = get_root_dir_inode();
    //    get_inode(root_dir_inode_nr, &root_inode);
    int i;
    DIR_ENTRY * empty_entry;
    for (i = 0;i < FILE_DEFAULT_SECTOR_NUM; i++) {
      hd_read(sb.first_sect+i, SECTOR_SIZE);
      if (NULL != (empty_entry = search_empty_dir_entry((DIR_ENTRY*)fsbuf, SECTOR_SIZE))){
	pos = (u32)((void*)empty_entry - (void*)fsbuf) + i * SECTOR_SIZE;
	break;
      }
    }
    if (pos == -1) {
      m->write_RST = -1;
      m->write_ERRNO = FULL_ROOT_DIR;
      return 0;
    }
  }

  int final_file_size = inode.file_size;
  if (pos + nbytes > inode.file_size) { /* 写的内容超过了文件原本大小 */
    if (pos + nbytes > (FILE_DEFAULT_SECTOR_NUM * SECTOR_SIZE)) { /* 写的内容超了最大大小 */
      m->write_ERRNO = WRITE_BEYOND_MAX_SIZE;
      m->write_RST   = -1;
      return 0;
    }
    final_file_size = pos + nbytes;
  }
  
  m->write_RST = nbytes;

  int first_write_sect = inode.start_sect + pos/SECTOR_SIZE;
  int first_write_index = pos - SECTOR_SIZE * (first_write_sect - inode.start_sect);
  int last_write_sect  = inode.start_sect + (pos + nbytes) / SECTOR_SIZE;
  int last_write_index  = pos + nbytes - SECTOR_SIZE * (last_write_sect - inode.start_sect);

  int curr_sect = first_write_sect;
  int curr_index = first_write_index;
  int start_index, end_index;
  while(curr_sect >= first_write_sect && curr_sect <= last_write_sect) {
    if (curr_sect == first_write_sect) start_index = first_write_index;
    else                               start_index = 0;
    if (curr_sect == last_write_sect)  end_index   = last_write_index;
    else                               end_index   = SECTOR_SIZE;

    hd_read(curr_sect, SECTOR_SIZE);
    xmemcpy(fsbuf+start_index, buf, end_index - start_index);
    hd_write(curr_sect, SECTOR_SIZE);
    buf += end_index - start_index;
    curr_sect += 1;
  }
  inode.file_size = final_file_size;

  put_inode(inode_nr, &inode);
  if (file_in_pcb)  file_in_pcb->pos += nbytes;
  return 0;
}

static int fs_close(MESSAGE * m)
{
  int inode_nr = m->close_INODE;
  PROCESS * proc = get_process(m->sender_pid);
  OPEN_FILE * opened_file;
  if (NULL == (opened_file = search_file_in_pcb(0, inode_nr, proc))) { /* 文件没打开 */
    m->close_RST = -1;
    m->close_ERRNO = FILE_NOT_OPENED;
    return 0;
  }
  /* 删除pcb里的这个文件 */
  del_file_in_pcb(inode_nr, proc);
  /* cache 中的count －1 */
  desc_cache_inode_count(inode_nr);

  //  opened_file->ext_flag = 0;
  /* memset(opened_file, 0, sizeof(OPEN_FILE)); */
  m->close_RST = 0;

  return 0;
}


static int fs_unlink(MESSAGE * m)
{
  char * name = va2la(m->sender_pid, (char*)m->unlink_NAME);

  if (-1 != search_cache_inode(name)) { /* 在cache中找到,说明有进程开着这个文件，不能unlink */
    m->unlink_RST = -1;
    m->unlink_ERRNO = SOME_PROC_OPENED_IT;
    return 0;
  }

  int inode_nr;
  if (-1 == (inode_nr = get_inode_nr(name))) { /* 没有这个文件 */
    m->unlink_RST = -1;
    m->unlink_ERRNO = FILE_NOT_FOUND;
    return 0;
  }
  INODE inode;
  get_inode(inode_nr, &inode);
  /* inode map 对应位置 置 0 */
  unset_inode_map(inode_nr);
  /* sect map 置 0 */
  unset_sect_map(inode.start_sect, FILE_DEFAULT_SECTOR_NUM);
  /* root dir 清理 */
  unset_root_dir(inode_nr);
  m->unlink_RST = 0;
  return 0;
}


static int fs_mm2fs_fork(MESSAGE * m)
{
  u32 par_pid = m->MM2FS_fork_PARPID;
  u32 kid_pid = m->MM2FS_fork_KIDPID;

  PROCESS * p_proc_p = get_process(par_pid);
  assert(p_proc_p != NULL);

  /* 在cache中增加对应文件的计数 */
  int i;
  for (i = 0; i < MAX_FILE_OPEN_PER_PROC; i++) {
    if (p_proc_p->open_files[i].ext_flag) {
      OPEN_FILE * f = &(p_proc_p->open_files[i]);
      assert(-1 != search_cache_inode(f->name));
      incr_cache_inode_count(f->inode);
    }
  }

  /* 复制父进程的open_files */
  PROCESS * kid_p;

  for (i = 0; i < PROCESS_NUM; i++) {
    if (process_table[i].pid == kid_pid && process_table[i].ext_flag == 0) { /* 这时ext_FLAG 还是0 */
      kid_p = &process_table[i];
      break;
    }
  }
  
  memcpy(kid_p->open_files, p_proc_p->open_files, sizeof(OPEN_FILE) * MAX_FILE_OPEN_PER_PROC);

  m->MM2FS_fork_RST = 0;
  return 0;
}

static int fs_mm2fs_exit(MESSAGE * m)
{
  u32 par_pid = m->MM2FS_exit_PARPID;
  u32 kid_pid = m->MM2FS_exit_KIDPID;

  PROCESS * kid_proc = get_process(kid_pid);
  assert(kid_proc != NULL);

  int i;
  for (i = 0; i < MAX_FILE_OPEN_PER_PROC; i++) {
    if (kid_proc->open_files[i].ext_flag) {
      OPEN_FILE * f = &(kid_proc->open_files[i]);
      assert(-1 != search_cache_inode(f->name));
      desc_cache_inode_count(f->inode);
    }
  }
  m->MM2FS_exit_RST = 0;
  return 0;
}


/* ------------------------------------------- */
/* 
   hd RW
 */
static void hd_read(u32 sect, u32 len)
{
  MESSAGE m;
  m.is_int_msg = 0;		/* 不要忘记这个 */
  m.type = HD_READ;
  m.POSITION = ((u64)sect)<<SECTOR_SIZE_SHIFT;
  m.CNT = len;
  m.DEVICE = HD_ROOT_MINOR;
  m.BUF = (u32)fsbuf;
  send(&m, TASK_HD);
}

static void hd_write(u32 sect, u32 len)
{
  MESSAGE m;
  m.is_int_msg = 0; 		/* 不要忘记这个 */
  m.type = HD_WRITE;
  m.POSITION = ((u64)sect)<<SECTOR_SIZE_SHIFT;
  m.CNT = len;
  m.DEVICE = HD_ROOT_MINOR;
  m.BUF = (u32)fsbuf;
  send(&m, TASK_HD);
}
/* TODO:  hd 的 close还没有写 */

//-----------------------------------------------syscall -----------------------------------

#define FS_PID      TASK_FS
int open(char * name, int mode)
{
  assert((mode&(~(FILE_RD|FILE_WR)))==0);
  
  MESSAGE msg;
  msg.type = MSG_open;
  msg.is_int_msg = 0;
  msg.open_NAME = (u32)name;
  msg.open_MODE = (u32)mode;
  while(1) {
    if (!send(&msg, FS_PID)) {
      fs_errno = msg.open_ERRNO;
      return msg.open_RST;
    }
  }
}

int read(int inode_nr, void * buf, int nbytes)
{
  assert(nbytes >= 0);

  MESSAGE msg;
  msg.type = MSG_read;
  msg.is_int_msg = 0;
  msg.read_INODE = inode_nr;
  msg.read_BUF = (u32)buf;
  msg.read_NBYTES = nbytes;
  while(1) {
    if (! send(&msg, FS_PID)) {
      fs_errno = msg.read_ERRNO;      
      return msg.read_RST;
    }
  }
}

int create(char * name)
{

  MESSAGE msg;
  msg.type = MSG_create;
  msg.is_int_msg = 0;
  msg.create_NAME = (u32)name;
  while(1) {
    if (! send(&msg, FS_PID)) {
      fs_errno = msg.create_ERRNO;
      return msg.create_RST;
    }
  }  
}

int write(int inode_nr, void * buf, int nbytes)
{
  assert(nbytes>=0);

  MESSAGE msg;
  msg.type = MSG_fwrite;
  msg.is_int_msg = 0;
  msg.write_INODE = inode_nr;
  msg.write_BUF = (u32)buf;
  msg.write_NBYTES = nbytes;
  while(1) {
    if (! send(&msg, FS_PID)) {
      fs_errno = msg.write_ERRNO;
      return msg.write_RST;
    }
  }
  
}

int close(int inode_nr)
{
  assert(inode_nr >= 0);

  MESSAGE msg;
  msg.type = MSG_close;
  msg.is_int_msg = 0;
  msg.close_INODE = inode_nr;
  while(1) {
    if (! send(&msg, FS_PID)) {
      fs_errno = msg.close_ERRNO;
      return msg.close_RST;
    }
  }
}

int unlink(char * name)
{
  MESSAGE msg;
  msg.type = MSG_unlink;
  msg.is_int_msg = 0;
  msg.unlink_NAME = (u32)name;
  while(1) {
    if (! send(&msg, FS_PID)) {
      fs_errno = msg.unlink_ERRNO;
      return msg.unlink_RST;
    }
  }  
}
