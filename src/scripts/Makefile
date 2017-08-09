all: syms

CFLAGS=
CC=gcc
include $(TOPDIR)/Rules.make
%: %.o
	$(CC) -o $@ $<
