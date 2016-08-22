ENTRY(_entry)

/* specify Plasma CPU memory areas */

MEMORY 
{
rom		: ORIGIN = 0x00000000, LENGTH = 256M	/* ROM area */
ram		: ORIGIN = 0x10000000, LENGTH = 256M	/* RAM area */
ram_top		: ORIGIN = 0x1ffffc00, LENGTH = 0x400	/* RAM area, 1KB reserved for the bootloader .bss + stack */
}

/* define a global symbol _stack  */

_stack = 0x1ffffffc;

/* now define the output sections  */

SECTIONS 
{
	. = 0;

	.text.start : {
		crt0.o( .text )
	} > ram
	.text :
	{
		_text = .;
		*(.text)
	        *(.text.*)
		_etext = .;
	} > ram

	.rodata :
	{
		_rodata = .;
		*(.rodata)
		*(.rodata.*)
		_erodata = .;
	} > ram

	.data :
	{
		_data = .;
		*(.data)
		*(.data.*)
		_gp = .;
		*(.sdata)
		*(.sdata.*)
		. = ALIGN(16);  
	} > ram

        .got : { 
                *(.got) 
                *(.got.*) 
        } > ram

	_edata = .;

	.bss :
	{
		_bss_start = .;
		*(.bss) 
		*(.bss.*)
		*(.sbss) 
		*(.sbss.*) 
		. = ALIGN(4);
		_bss_end = . ;
	} > ram

}
	_end = .;

