call_me = $(MAKE) -f $(TOPDIR)/Makefile.help dir=$(1) $(2)
recurse = set -e; for i in $(SUBDIRS); do $(call call_me,$(addprefix $(dir)/,$$i),$(1)); done

%.o: %.c
	echo "CC $(subst $(TOPDIR)/,,$<)"
	$(CC) $(CFLAGS) -c -o $@ $<
	: > $(TOPDIR)/.changed

%.o: %.asm
	echo "AS $(subst $(TOPDIR)/,,$<)"
	$(AS) $(ASFLAGS) -o $@ $<
	: > $(TOPDIR)/.changed
