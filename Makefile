#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
#ENTRYPOINT	= 0x30400
ENTRYPOINT	= 0x1000
# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -I include/ -c -fno-builtin -m32 -fno-stack-protector -g
LDFLAGS		= -Ttext $(ENTRYPOINT) -m elf_i386 #-s #// gdb 调试需要symbol
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)



COMMON_FUNC     = lib/printf.o lib/itoa.o lib/xstring_c.o lib/addr_trans.o lib/assert.o kernel/syscall_c.o kernel/syscall_proc.o lib/memset.o lib/bit.o

FS              = fs/fs.o fs/driver.o
MM		= mm/mm.o
# This Program
ORANGESBOOT	= boot/boot.bin boot/loader.bin
ORANGESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o  kernel/i8259.o kernel/global.o  lib/kliba.o lib/string.o lib/xstring.o kernel/xglobal.o kernel/xinit.o lib/xdisp.o lib/xklib.o  kernel/interrupt.o kernel/process.o kernel/kmain.o kernel/keyboard.o kernel/syscall.o kernel/syscall_implement.o lib/xdisp_2.o kernel/screen.o kernel/tty.o $(COMMON_FUNC) $(FS) lib/xklib_c.o $(MM) lib/get_gdtr_ldtr.o lib/tool.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
do : clean everything buildimg
	bochs

everything : $(ORANGESBOOT) $(ORANGESKERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean :
	rm -f $(OBJS)

realclean :
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# We assume that "a.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc \
			boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

# kernel/start.o: kernel/start.c include/type.h include/const.h include/protect.h \
# 	 		include/proto.h include/string.h
# 	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o : kernel/i8259.c include/type.h include/const.h include/protect.h \
	 			include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o : kernel/global.c
	$(CC) $(CFLAGS) -o $@ $<

# kernel/protect.o : kernel/protect.c
# 	$(CC) $(CFLAGS) -o $@ $<

# lib/klib.o : lib/klib.c
# 	$(CC) $(CFLAGS) -o $@ $<

lib/kliba.o : lib/kliba.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
# -----------------------------------------------------------------
lib/xstring.o : lib/xstring.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
kernel/xglobal.o : kernel/xglobal.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/xinit.o : kernel/xinit.c
	$(CC) $(CFLAGS) -o $@ $<
lib/xdisp.o : lib/xdisp.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
lib/xklib.o : lib/xklib.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
kernel/interrupt.o : kernel/interrupt.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/kmain.o : kernel/kmain.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/process.o : kernel/process.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/keyboard.o : kernel/keyboard.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/syscall.o : kernel/syscall.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
kernel/syscall_implement.o : kernel/syscall_implement.c
	$(CC) $(CFLAGS) -o $@ $<
lib/xdisp_2.o : lib/xdisp_2.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/screen.o : kernel/screen.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/tty.o : kernel/tty.c
	$(CC) $(CFLAGS) -o $@ $<
lib/printf.o : lib/printf.c
	$(CC) $(CFLAGS) -o $@ $<
lib/itoa.o : lib/itoa.c
	$(CC) $(CFLAGS) -o $@ $<
lib/xstring_c.o : lib/xstring_c.c
	$(CC) $(CFLAGS) -o $@ $<
lib/addr_trans.o : lib/addr_trans.c
	$(CC) $(CFLAGS) -o $@ $<
lib/assert.o : lib/assert.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/syscall_c.o : kernel/syscall_c.c
	$(CC) $(CFLAGS) -o $@ $<
kernel/syscall_proc.o : kernel/syscall_proc.c
	$(CC) $(CFLAGS) -o $@ $<
fs/fs.o : fs/fs.c
	$(CC) $(CFLAGS) -o $@ $<
fs/driver.o : fs/driver.c
	$(CC) $(CFLAGS) -o $@ $<
lib/xklib_c.o : lib/xklib_c.c
	$(CC) $(CFLAGS) -o $@ $<
lib/memset.o : lib/memset.c
	$(CC) $(CFLAGS) -o $@ $<
lib/bit.o : lib/bit.c
	$(CC) $(CFLAGS) -o $@ $<
mm/mm.o : mm/mm.c
	$(CC) $(CFLAGS) -o $@ $<
lib/get_gdtr_ldtr.o : lib/get_gdtr_ldtr.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<
lib/tool.o : lib/tool.c
	$(CC) $(CFLAGS) -o $@ $<
