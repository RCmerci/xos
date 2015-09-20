

all: string.o test.o
	ld  -m elf_i386 -e main string.o test.o

string.o: string.asm
	nasm -f elf -o $@ $<

test.o: test.c
	gcc -c -fno-builtin -m32 -fno-stack-protector -g  test.c

clean:
	rm test.o string.o
