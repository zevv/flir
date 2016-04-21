OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_entry_rom)

SECTIONS {
	. = 0x0;
	_srom = .;

	.vectors : { 
		*(.vectors); 
	}

	.text : { 
		*(.text)
	}

	.rodata : { 
		*(.rodata) 
	}

	. = ALIGN(16);
	_erom = .;

	.data 0x10000400: AT(_erom) {
		_sdata = .;
		*(.data);
		_edata = .;
	}

	.bss : {
		_sbss = .;
		*(.bss*);
		*(COMMON);
		_ebss = .;
	}

	/DISCARD/ : { 
		*(.text.ramboot); 
	}
}

PROVIDE(_estack = 0x10000400);

