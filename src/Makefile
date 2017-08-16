VERSION_MAJOR   := 0
VERSION_MINOR   := 0
VERSION_RELEASE := 6
VERSION_BUILD   := $(shell cat .build 2>/dev/null)
VERSION_BUILD   := $(shell expr $(VERSION_BUILD) + 1)
VERSION         := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE).b$(VERSION_BUILD)

export TOPDIR=$(CURDIR)
dir=$(TOPDIR)

ifndef V
export MAKEFLAGS = -s
endif

ifndef D
OPT_CFLAGS = -O2
endif

CC=gcc
AS=gcc
LD=ld
INCLUDE=$(TOPDIR)/include
CFLAGS := -I$(INCLUDE) -finline-functions -fno-stack-protector -fno-builtin -nostdinc -Wall \
	  -Wstrict-prototypes -fno-omit-frame-pointer -fno-common -mcmodel=kernel -fno-pie -g \
	  -mno-red-zone $(OPT_CFLAGS)
ASFLAGS:= -DASM -I$(INCLUDE) -g
LDFLAGS:= -g

export CC AS LD CFLAGS ASFLAGS LDFLAGS

SUBDIRS:=kernel arch mm drivers fs lib
.PHONY: scripts
.PHONY: dep
.PHONY: tags

LIBGCC := $(shell $(CC) -print-libgcc-file-name)
OBJS := $(addsuffix /all.o,$(SUBDIRS)) $(LIBGCC)

all: image

dep:
	$(call recurse,dep)

version:
	@echo $(VERSION_BUILD) > .build
	@(exit 0;echo "#ifndef VERSION_H" > version.h;	\
	echo "#define VERSION_H" >> version.h;	\
	echo "#define VERSION_MAJOR $(VERSION_MAJOR)" >> version.h;	\
	echo "#define VERSION_MINOR $(VERSION_MINOR)" >> version.h;	\
	echo "#define VERSION_BUILD $(VERSION_BUILD)" >> version.h;	\
	echo "#endif" >> version.h)

image: dep scripts version
	$(call recurse,build)
	[ -e .changed ] && { $(MAKE) link && rm -f .changed || exit 1; } || { echo "Nothing changed, not linking"; exit 0; }

link:
	echo "Linking kernel image..."
	$(LD) -o image.unstripped -T script.ld -z max-page-size=0x1000 $(OBJS)
	nm image.unstripped | sort | scripts/syms > ksyms.c
	$(CC) $(CFLAGS) -c ksyms.c -o ksyms.o
	$(LD) -o image.unstripped -T script.ld -z max-page-size=0x1000 $(OBJS) ksyms.o
	nm image.unstripped | sort | scripts/syms > ksyms.c
	$(CC) $(CFLAGS) -c ksyms.c -o ksyms.o
	$(LD) -o image.unstripped -T script.ld -g -z max-page-size=0x1000 $(OBJS) ksyms.o
	nm image.unstripped | sort > system.map
	rm -f ksyms.c ksyms.o
	strip image.unstripped -o image.elf
	wc -c image.unstripped
	wc -c image.elf
	echo Build: `cat .build`

scripts:
	$(MAKE) -C $@

backup:
	cat > README && cd .. && tar czf landaux-$(VERSION).tar.gz src && mv landaux-$(VERSION).tar.gz backup

clean:
	find . -name '*.o' -o -name '.depend' | xargs rm -f

tags:
	ctags -R --extra=+f

debug: QFLAGS=-s -S
debug: qemu

qemu: image
	$(MAKE) -C ../usr
	qemu-system-x86_64 -M q35 -device nec-usb-xhci -m 32 -kernel image.elf -initrd ../usr/initrd.tar $(QFLAGS)

include $(TOPDIR)/Rules.make
