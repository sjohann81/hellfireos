ENTRY(_entry)

/* specify HF-RISC memory areas */

MEMORY 
{
ram		: ORIGIN = 0x00010000, LENGTH = 128M	/* RAM area */
}

/* define a global symbol _stack  */

_stack = 0x07fffffc;

/* now define the output sections  */

SECTIONS 
{
	. = 0;

	.text :
	{
		*(.text.init)
	}
	
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
		_gp = . + 0x7ff0;
		*(.sdata)
		*(.sdata.*)
		. = ALIGN(16);
	} > ram

	_edata = .;

	.bss :
	{
		_bss_start = .;
		*(.sbss) 
		*(.sbss.*)
		*(.bss) 
		*(.bss.*)
		. = ALIGN(4);
		_bss_end = . ;
	} > ram

}
	_end = .;

