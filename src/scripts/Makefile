all: cpboot patch_boot syms

CFLAGS=
include $(TOPDIR)/Rules.make
%: %.o
	$(CC) -o $@ $<
