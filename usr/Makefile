CC=gcc -specs=landaux.spec -m32 -static

all: initrd.tar

initrd.tar: prog
	tar cf $@ $<

prog: prog.c
