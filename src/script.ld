OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)
SECTIONS
{
    . = 0xffffffff80100000 + SIZEOF_HEADERS;
    .text : AT(0x100000 + SIZEOF_HEADERS + 8)
    {
	_code = .;
	*(.text)
	_code_end = .;
	_rodata = .;
	*(.rodata*)
	_rodata_end = .;
	_init_funcs = .;
	*(.initfuncs)
	_init_funcs_end = .;
	. = ALIGN(8);
    }
    .ksymtab :
    {
	_ksymtab = .;
	PROVIDE(__ksymtab = .);	/* for the first link, when the symtab is not generated yet */
	*(.ksymtab)
	_ksymtab_end = .;
	. = ALIGN(8);
    }
    .data :
    {
	 _data = .;
	*(.data)
	_data_end = .;
	. = ALIGN(8);
    }
    .bss :
    {
	_bss = .;
	*(.bss)
	. = ALIGN(8);
    }
    _bss_end = .;
    _end = .;
    /DISCARD/ :
    {
	    *(.comment)
	    *(.eh_frame)
    }
}
