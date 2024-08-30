ENTRY(StrayldrMain)
OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
SECTIONS {
	__BASE_ADDRESS = 0x2000;
	__STACK_TOP = __BASE_ADDRESS;
	. = __BASE_ADDRESS;

	.descr BLOCK(512) : ALIGN(512) {
		SHORT(StrayldrMain);
		SHORT(__STACK_TOP);
	}

	.text : {
		*(.text)
	}

	.data BLOCK(512) : ALIGN(512) {
		*(.data)
	}

	.rodata BLOCK(512) : ALIGN(512) {
		*(.rodata)
	}

	.bss BLOCK(512) : ALIGN(512) {
		*(COMMON)
		*(.bss)
	}

	. = ALIGN(512);
	__END_ADDRESS = .;
}