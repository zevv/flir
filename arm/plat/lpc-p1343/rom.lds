OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_entry_rom)


MEMORY {
	FLASH : org = 0x00000000, len = 32K
	RAM : org = 0x10000000, len = 8K
}

SECTIONS {
	. = 0x0;
	_srom = .;

	.vectors : { 
		KEEP(*(.vectors));
	} > FLASH

	.text : { 
		_stext = .;
		*(.text*)
		_etext = .;
	} > FLASH

	.rodata : { 
		*(.rodata*) 
	} > FLASH

	. = ALIGN(16);
	_erom = .;

	.data 0x10000400: AT(_erom) {
		_sdata = .;
		*(.data*);
		_edata = .;
	} > RAM

	.bss : {
		_sbss = .;
		*(.bss*);
		_ebss = .;
	} > RAM
}

PROVIDE(_estack = 0x10000400);

