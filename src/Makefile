VERSION_MAJOR   := 0
VERSION_MINOR   := 0
VERSION_RELEASE := 4
VERSION_BUILD   := $(shell cat .build 2>/dev/null)
VERSION_BUILD   := $(shell expr $(VERSION_BUILD) + 1)
VERSION         := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE).b$(VERSION_BUILD)

export TOPDIR=$(CURDIR)
dir=$(TOPDIR)

ifndef V
export MAKEFLAGS = -s
endif

CC=gcc
AS=nasm -f elf
INCLUDE=$(TOPDIR)/include
CFLAGS := -I$(INCLUDE) -finline-functions -O2 -nostdinc -Wall
ASFLAGS := 

export CC AS CFLAGS ASFLAGS

SUBDIRS=kernel arch mm
.PHONY: scripts
.PHONY: dep boot

OBJS := $(addsuffix /all.o,$(SUBDIRS))

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

image: boot scripts version
	$(call recurse,build)
	echo "Linking kernel image..."
	$(LD) -o image.elf -T script.ld $(OBJS)
	nm image.elf | sort | scripts/syms > ksyms.c
	$(CC) $(CFLAGS) -c ksyms.c -o ksyms.o
	$(LD) -o image.elf -T script.ld $(OBJS) ksyms.o
	nm image.elf | sort | scripts/syms > ksyms.c
	$(CC) $(CFLAGS) -c ksyms.c -o ksyms.o
	$(LD) -o image.elf -T script.ld $(OBJS) ksyms.o
	nm image.elf | sort > system.map
	objcopy -O binary image.elf image
	rm -f ksyms.c ksyms.o
	echo Build `cat .build`

scripts boot:
	$(MAKE) -C $@

backup:
	cat > README && cd .. && tar czf landaux-$(VERSION).tar.gz src && mv landaux-$(VERSION).tar.gz backup

clean:
	find . -name '*.o' -o -name '.depend' | xargs rm -f

install: image
	scripts/cpboot boot/boot.o ../bochs/floppya.img
	mount ../bochs/mnt
	cp image ../bochs/mnt
	umount ../bochs/mnt
	scripts/patch_boot image ../bochs/floppya.img

bochs: install
	cd ../bochs && bochs -q

debug: install
	cd ../bochs && bfe2

include $(TOPDIR)/Rules.make
