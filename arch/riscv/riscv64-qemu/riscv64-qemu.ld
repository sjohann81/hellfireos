ENTRY(_entry)

MEMORY 
{
ram   (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

/* define a global symbol _stack  */

_stack = ORIGIN(ram) + LENGTH(ram);

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
		*(.rdata)
		*(.rodata)
		*(.rodata.*)
		_erodata = .;
	} > ram

	.data :
	{
		_data = .;
		*(.data)
		*(.data.*)
		_gp = . + 0x800;
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

	_end = .;
}

