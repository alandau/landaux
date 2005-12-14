OUTPUT_FORMAT("elf32-i386")
ENTRY(entry)
SECTIONS
{
    . = 0xC0000000;
    .text :
    {
	code = .; _code = .;
	*(.text)
	code_end = .;
	rodata = .;
	*(.rodata*)
	rodata_end = .;
	. = ALIGN(4);
    }
    .data :
    {
	data = .; _data = .;
	*(.data)
	data_end = .;
	. = ALIGN(4);
    }
    .ksymtab :
    {
	ksymtab = .; _ksymtab = .;
	PROVIDE(__ksymtab = .);
	*(.ksymtab)
	ksymtab_end = .;
	. = ALIGN(4);
    }
    .bss :
    {
	bss = .; _bss = .;
	*(.bss)
	. = ALIGN(4);
	bss_end = .;
    }
    end = .; _end = .;
}
