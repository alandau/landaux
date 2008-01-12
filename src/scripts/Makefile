all: syms

CFLAGS=
include $(TOPDIR)/Rules.make
%: %.o
	$(CC) -o $@ $<
