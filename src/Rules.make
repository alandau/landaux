call_me = $(MAKE) -f $(TOPDIR)/Makefile.help dir=$(1) $(2)
recurse = set -e; for i in $(SUBDIRS); do $(call call_me,$(addprefix $(dir)/,$$i),$(1)); done

%.o: %.c
	echo "CC $(subst $(TOPDIR)/,,$<) -> $(subst $(TOPDIR)/,,$@)"
	$(CC) $(CFLAGS) -c -o $@ $<
	: > $(TOPDIR)/.changed

%.o: %.S
	echo "AS $(subst $(TOPDIR)/,,$<) -> $(subst $(TOPDIR)/,,$@)"
	$(AS) $(ASFLAGS) -c -o $@ $<
	: > $(TOPDIR)/.changed

%.s: %.c
	echo "CC $(subst $(TOPDIR)/,,$<) -> $(subst $(TOPDIR)/,,$@)"
	$(CC) $(CFLAGS) -S -o $@ $<

%.s: %.S
	echo "AS $(subst $(TOPDIR)/,,$<) -> $(subst $(TOPDIR)/,,$@)"
	$(AS) $(ASFLAGS) -E -o $@ $<
