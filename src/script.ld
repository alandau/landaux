OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS
{
    . = 0xC0100000;
    .text : AT(0x100000)
    {
	_code = .;
	*(.text)
	_code_end = .;
	_rodata = .;
	*(.rodata*)
	_rodata_end = .;
	. = ALIGN(4);
    }
    .data :
    {
	 _data = .;
	*(.data)
	_data_end = .;
	. = ALIGN(4);
    }
    .ksymtab :
    {
	_ksymtab = .;
	PROVIDE(__ksymtab = .);	/* for the first link, when the symtab is not generated yey */
	*(.ksymtab)
	_ksymtab_end = .;
	. = ALIGN(4);
    }
    .bss :
    {
	_bss = .;
	*(.bss)
	. = ALIGN(4);
    }
    _bss_end = .;
    _end = .;
    /DISCARD/ :
    {
	    *(.comment)
    }
}
