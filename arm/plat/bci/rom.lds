
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_entry_rom)


MEMORY
{
	FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
	RAM (xrw)       : ORIGIN = 0x20001000, LENGTH = 64K
}


SECTIONS {
	. = 0x0;

	.vectors : { 
		*(.vectors); 
	} > FLASH
	
	_srom = .;

	.text : { 
		*(.text)
	} > FLASH

	.rodata : { 
		*(.rodata) 
	} > FLASH

	. = ALIGN(16);

	_erom = .;

	.data :  AT ( _erom ) {
		_sdata = .;
		*(.data);
		_edata = .;
	} > RAM

	.bss : {
		_sbss = .;
		*(.bss*);
		*(COMMON);
		_ebss = .;
	} > RAM

	/DISCARD/ : { 
		*(.text.ramboot); 
	}
}

PROVIDE(_estack = 0x20001000);

