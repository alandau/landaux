call_me = $(MAKE) -f $(TOPDIR)/Makefile.help dir=$(1) $(2)
recurse = set -e; for i in $(SUBDIRS); do $(call call_me,$(addprefix $(dir)/,$$i),$(1)); done

%.o: %.c
	echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm
	echo "Compiling $< ..."
	$(AS) $(ASFLAGS) -o $@ $<
