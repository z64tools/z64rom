INCLUDE syms.ld
INCLUDE entry.ld

OUTPUT_ARCH( mips )
OUTPUT_FORMAT( "elf32-bigmips", "elf32-bigmips", "elf32-littlemips" )
ENTRY( ENTRY_POINT )

SECTIONS
{
	/* Program run addressed defined in `entry.ld` */
	. = ENTRY_POINT;

	/* Machine code */
	.text ALIGN( 4 ):
	{
		/* Pad with NULL */
		FILL( 0 );

		/* Address to start of text section */
		__text_start = . ;

		/* Data goes here */
		*(.init);
		*(.text);
		*(.ctors);
			*(.dtors);
			*(.fini);
		
		/* Address to end of text section */
		. = ALIGN( 16 );
		__text_end = . ;
	}
	
	/* Read-only data */
	.rodata ALIGN( 16 ):
	{
		/* Pad with NULL */
		FILL( 0 );
		
		/* Address to start of rodata section */
		__rodata_start = . ;
		
		/* Data goes here*/
		*(.rodata);
		*(.rodata.*);
		*(.eh_frame);
		
		/* Address to end of rodata section */
		. = ALIGN( 16 );
		__rodata_end = . ;
	}
	
	/* Initialized data */
	.data ALIGN( 16 ):
	{
		/* Pad with NULL */
		FILL( 0 );
		
		/* Address to start of data section */
		__data_start = . ;
		
		/* Data goes here*/
		*(.data);
		*(.data.*);
		
		/* Data pointer */
		. = ALIGN( 8 );
		_gp = . ;
		*(.sdata);
		
		/* Address to end of data section */
		. = ALIGN( 16 );
		__data_end = . ;

		/* Address to start of BSS section */
		__bss_start = . ;
		
		/* BSS data */
		*(.scommon);
		*(.sbss);
		*(.bss);
		
		/* Address to end of BSS section */
		. = ALIGN( 8 );
		__bss_end = . ;
		ASSERT ((__bss_end <= 0x807B5000), "Error: Not enough room");
	}
	
	/* End of our memory use */
	end = . ;
	/DISCARD/ : { *(*); }
}
