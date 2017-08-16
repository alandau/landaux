#ifndef MULTIBOOT_H
#define MULTIBOOT_H

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC		0x1BADB002

/* The flags for the Multiboot header. */
#define MULTIBOOT_FLAG_MODULE_PAGEALIGN	(1<<0)
#define MULTIBOOT_FLAG_MEM_INFO		(1<<1)
#define MULTIBOOT_FLAG_EXPLICIT_ADDR	(1<<16)
#define MULTIBOOT_HEADER_FLAGS		(MULTIBOOT_FLAG_MODULE_PAGEALIGN | MULTIBOOT_FLAG_MEM_INFO | MULTIBOOT_FLAG_EXPLICIT_ADDR)

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC	0x2BADB002

#define MULTIBOOT_INFO_FLAG_MEMSIZE	(1<<0)

#ifndef ASM

/* The Multiboot header. */
typedef struct multiboot_header
{
  unsigned int magic;
  unsigned int flags;
  unsigned int checksum;
  unsigned int header_addr;
  unsigned int load_addr;
  unsigned int load_end_addr;
  unsigned int bss_end_addr;
  unsigned int entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  unsigned int tabsize;
  unsigned int strsize;
  unsigned int addr;
  unsigned int reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  unsigned int num;
  unsigned int size;
  unsigned int addr;
  unsigned int shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
  unsigned int flags;
  unsigned int mem_lower;
  unsigned int mem_upper;
  unsigned int boot_device;
  unsigned int cmdline;
  unsigned int mods_count;
  unsigned int mods_addr;
  union
  {
    aout_symbol_table_t aout_sym;
    elf_section_header_table_t elf_sec;
  } u;
  unsigned int mmap_length;
  unsigned int mmap_addr;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
  unsigned int mod_start;
  unsigned int mod_end;
  unsigned int string;
  unsigned int reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
  unsigned int size;
  unsigned int base_addr_low;
  unsigned int base_addr_high;
  unsigned int length_low;
  unsigned int length_high;
  unsigned int type;
} memory_map_t;

#endif /* ! ASM */

#endif
