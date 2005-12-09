OUTPUT_FORMAT("elf32-i386")
ENTRY(entry)
LS_Phys = 0xC0000000;
LS_Virt = 0xC0000000;
LS_16   = 0x1000;
SECTIONS
{
    .text LS_Virt : AT(LS_Phys)
    {
	LS_Code = .;
	code = .; _code = .;
	*(.text)
	code_end = .;
	rodata = .;
	*(.rodata*)
	rodata_end = .;
	. = ALIGN(4);
    }
    .data : AT(LS_Phys + (LS_Data - LS_Code))
    {
	LS_Data = .;
	data = .; _data = .;
	*(.data)
	data_end = .;
	. = ALIGN(4);
    }
    .bss : AT(LS_Phys + (LS_Bss - LS_Code))
    {
	LS_Bss = .;
	bss = .; _bss = .;
	*(.bss)
	. = ALIGN(4);
	bss_end = .;
    }
    .ksymtab : AT(LS_Phys + (LS_Ksymtab - LS_Code))
    {
	LS_Ksymtab = .;
	ksymtab = .; _ksymtab = .;
	PROVIDE(__ksymtab = .);
	*(.ksymtab)
	ksymtab_end = .;
	. = ALIGN(4);
    }
    end = .; _end = .;
}
