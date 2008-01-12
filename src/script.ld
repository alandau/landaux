OUTPUT_FORMAT("elf32-i386")
ENTRY(entry)
SECTIONS
{
    . = 0xC0000000;
    .text :
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
}
