CC=gcc -specs=landaux.spec -static
CFLAGS=-g -O2 -fno-builtin

.PHONY: initfiles

all: initrd.tar

initrd.tar: prog
	tar cf $@ $<

initfiles: landaux_start.S landaux_end.S
	$(CC) -c landaux_start.S
	$(CC) -c landaux_end.S

prog: prog.c
