#include "common.h"
#include "xtype.h"
#include "hd.h"
#include "xconst.h"
#include "syscall.h"
#include "xglobal.h"
#include "message.h"

static u8 hd_status;
static void hd_handler();
static void hd_open();
static void hd_ioctl(MESSAGE * m);
static void hd_rdwt(MESSAGE * m);
static void out_cmd(HD_CMD * cmd);
static int waitfor(int mask, int val, int timeout);
static void interrupt_wait();
static void print_identify_info(u16* hdinfo);
static u8 hdbuf[SECTOR_SIZE * 2];
static struct hd_info hd_info[1];




static void init_hd()
{
    printk(0, "init hd start...\n");
  
    set_irq_handler(HD_IRQ_NUM, hd_handler);
    enable_irq(2);   // 2 是8259主从连接的那个端口数字
    enable_irq(HD_IRQ_NUM);  //先注释掉，直接在kernel/xinit.c里改一下
  
    printk(0, "init hd end.\n");
}


void task_hd()
{
    init_hd();
    MESSAGE start_m;
    start_m.body.arg1 = 233;
    start_m.is_int_msg = 0;
    send(&start_m, 3); // 3 -> fs pid
    hd_init_done = 1;  /* hd 初始化完成*/
    while(1) {
	MESSAGE * m = recv(NORMAL_MSG);
	switch(m->type) {
	case HD_OPEN:
	    hd_open();
	    break;
	case HD_CLOSE:
	    break;
	case HD_READ:
	case HD_WRITE:
	    hd_rdwt(m);
	    break;
	case HD_IOCTL:
	    hd_ioctl(m);
	    break;
	default:
	    assert("wrong hd task msg type" == 0);
	    break;
	}
	unblock_process(m->sender_pid);
    }
}

void hd_cmd_out(HD_CMD* cmd)
{
    if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
	assert("hd error."==0);

    /* Activate the Interrupt Enable (nIEN) bit */
    xout_byte(REG_DEVICE_CONTROL, 0);
    /* Load required parameters in the Command Block Registers */
    xout_byte(REG_FEATURES,      cmd->features);
    xout_byte(REG_SECTOR_COUNT,  cmd->count);
    xout_byte(REG_LBA_LOW,       cmd->lba_low);
    xout_byte(REG_LBA_MID,       cmd->lba_mid);
    xout_byte(REG_LBA_HIGH,      cmd->lba_high);
    xout_byte(REG_DEVICE,        cmd->device);
    /* Write the command code to the Command Register */
    xout_byte(REG_COMMAND,       cmd->command);
}

static int waitfor(int mask, int val, int timeout)
{
    int t = get_ticks();
    while(((get_ticks() - t) * 1000 / HZ) < timeout) {
	if ((xin_byte(REG_STATUS) & mask) == val) return 1;
    }
    return 0;
}

static void wait_a_little()
{
  int i = 3000;
  while(i-->0) {};
}

static void interrupt_wait()
{
  wait_a_little();
  MESSAGE * m = recv(INT_MSG);
    
}


static void hd_identify(int drive)
{
    HD_CMD cmd;
    cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    hd_cmd_out(&cmd);
    interrupt_wait();
    port_read(REG_DATA, hdbuf, SECTOR_SIZE);

    print_identify_info((u16*)hdbuf);

    u16* hdinfo = (u16*)hdbuf;

    hd_info[drive].primary[0].base = 0;
    /* Total Nr of User Addressable Sectors */
    hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

static void print_identify_info(u16* hdinfo)
{
    int i, k;
    char s[64];

    struct iden_info_ascii {
	int idx;
	int len;
	char * desc;
    } iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
		 {27, 40, "HD Model"} /* Model number in ASCII */ };

    for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
	char * p = (char*)&hdinfo[iinfo[k].idx];
	for (i = 0; i < iinfo[k].len/2; i++) {
	    s[i*2+1] = *p++;
	    s[i*2] = *p++;
	}
	s[i*2] = 0;
	printk(0, "%s: %s\n", iinfo[k].desc, s);
    }

    int capabilities = hdinfo[49];
    printk(0, "LBA supported: %s\n",
	   (capabilities & 0x0200) ? "Yes" : "No");

    int cmd_set_supported = hdinfo[83];
    printk(0, "LBA48 supported: %s\n",
	   (cmd_set_supported & 0x0400) ? "Yes" : "No");

    int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
    printk(0, "HD size: %dMB\n", sectors * 512 / 1000000);
}

static void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	HD_CMD cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}


static void partition(int device, int style)
{
    int drive = 0;
    struct part_ent part_tbl[NR_PART_PER_DRIVE];
    struct hd_info * hdinfo = hd_info;
    if (style == P_PRIMARY) {
	int i;
	get_part_table(drive, 0, part_tbl);
	for(i=1;i<NR_PRIM_PER_DRIVE;i++) {
	    if (part_tbl[i-1].sys_id == NO_PART) continue;
	    hdinfo->primary[i].base = part_tbl[i-1].start_sect;
	    hdinfo->primary[i].size = part_tbl[i-1].nr_sects;

	    if (part_tbl[i-1].sys_id == EXT_PART) {
		partition(device + i, P_EXTENDED);
	    }
	}
    }else if (style == P_EXTENDED){
	int father_index = device % NR_PRIM_PER_DRIVE;
	int last_part_tbl_sect = hdinfo->primary[father_index].base;
	int i;
	for (i=0;i<NR_SUB_PER_PART;i++) {
	    get_part_table(drive, last_part_tbl_sect, part_tbl);
	    if (part_tbl[0].sys_id == NO_PART) continue;
	    hdinfo->logical[i+(father_index-1)*NR_SUB_PER_PART].base = last_part_tbl_sect + part_tbl[0].start_sect;
	    hdinfo->logical[i+(father_index-1)*NR_SUB_PER_PART].size = part_tbl[0].nr_sects;
	    if (part_tbl[1].sys_id == NO_PART) break;
	    last_part_tbl_sect = part_tbl[1].start_sect + hdinfo->primary[father_index].base;
	}
    }else {
	assert("no such style."==0);
    }
}


static void hd_rdwt(MESSAGE * p)
{
    /* int drive = DRV_OF_DEV(p->DEVICE); */
    int drive = 0;

    u64 pos = p->POSITION;
    assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

    /**
     * We only allow to R/W from a SECTOR boundary:
     */
    assert((pos & 0x1FF) == 0);

    u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); /* pos / SECTOR_SIZE */
    int logidx = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
    sect_nr += p->DEVICE < MAX_PRIM ?
	hd_info[drive].primary[p->DEVICE].base :
	hd_info[drive].logical[logidx].base;

    HD_CMD cmd;
    cmd.features	= 0;
    cmd.count	= (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
    cmd.lba_low	= sect_nr & 0xFF;
    cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
    cmd.lba_high	= (sect_nr >> 16) & 0xFF;
    cmd.device	= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
    cmd.command	= (p->type == HD_READ) ? ATA_READ : ATA_WRITE;
    hd_cmd_out(&cmd);

    int bytes_left = p->CNT;
    void * la = (void*)va2la(p->sender_pid, (void*)p->BUF);

    while (bytes_left) {
	int bytes = min(SECTOR_SIZE, bytes_left);
	if (p->type == HD_READ) {
	    interrupt_wait();
	    port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	    xmemcpy(la, (void*)va2la(TASK_HD, hdbuf), bytes);
	}
	else {
	    if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
		assert("hd writing error."==0);

	    port_write(REG_DATA, la, bytes);
	    interrupt_wait();
	}
	bytes_left -= SECTOR_SIZE;
	la += SECTOR_SIZE;
    }

}

static void print_hdinfo()
{
    int i;
    struct part_info * primary_p = hd_info[0].primary;
    struct part_info * logical_p = hd_info[0].logical;    
    for (i=0;i<NR_PRIM_PER_DRIVE;i++) {
	if (primary_p[i].base == 0 &&
	    primary_p[i].size == 0) {
	    continue;
	}
	printk(0, "primary %d , base: %d, size %d\n", i, primary_p[i].base, primary_p[i].size);
    }
    for (i=0;i<NR_SUB_PER_DRIVE;i++) {
	if (logical_p[i].base == 0 &&
	    logical_p[i].size == 0) {
	    continue;
	}
	printk(0, "logical %d, base: %d, size %d\n", i, logical_p[i].base, logical_p[i].size);
    }
}


static void hd_open()
{
    int drive = 0;
    memset(hd_info, 0, sizeof(struct hd_info));
    hd_identify(drive);

    if (hd_info[drive].open_cnt++ == 0) {
	partition(drive * (NR_PART_PER_DRIVE+1), P_PRIMARY);
	print_hdinfo(&hd_info[drive]);
    }
}

static void hd_ioctl(MESSAGE * m)
{
    u32 dev = m->DEVICE;
    if (dev <= MAX_PRIM) {
	m->IOCTL_BASE = hd_info[0].primary[dev].base;
	m->IOCTL_SIZE = hd_info[0].primary[dev].size;
    } else {
	dev = (dev - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	m->IOCTL_BASE = hd_info[0].logical[dev].base;
	m->IOCTL_SIZE = hd_info[0].logical[dev].size;
    }
}

static void hd_close()
{
    ;				/* TODO */
}




/* ring 0 */
void hd_handler()
{
    hd_status = xin_byte(REG_STATUS); /* 还不知道这个干什么用 */
    MESSAGE m;
    m.type = MSG_HD;
    m.is_int_msg = 1;
    _send(&m, 2, NULL); // 2-> xglobal.c task_hd 的 pid，
    // null-> 因为本函数在ring0, 所以没有对应进程
    // 直接用 _send 而不是 系统调用 send
}
