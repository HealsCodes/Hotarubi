 /* 
  * linker script for x86_64 target
  */

/*OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")*/
/*OUTPUT_ARCH(i386:x86-64)*/

ENTRY(kickstart)

KERNEL_LMA = 0x00100000;
KERNEL_VMA = 0xffffffff80000000;
SECTIONS {
	
	. = KERNEL_LMA;

	.bootstrap : {
		__bootstrap = .;
		/* bootstrap code is handled in a seperate object file
		 * and doesn't really know about 64bit or virtual addresses
		 */

		KEEP (src/kernel/boot.o (.multiboot))
		KEEP (src/kernel/boot.o (.text))
		KEEP (src/kernel/boot.o (.data))
		KEEP (src/kernel/boot.o (.bss))

		__ebootstrap = .;
	}
	
	. = ALIGN(4096);
	. += KERNEL_VMA;

	.text : AT(ADDR(.text) - KERNEL_VMA) {
		__text = .;
		*(EXCLUDE_FILE(src/kernel/boot.o) .text)

		KEEP (*(.init))
		KEEP (*(.fini))

		*(.text* .gnu.linkonce.t*)
		*(.rodata* .gnu.linkonce.r*)
		__etext = .;
	}
	.data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VMA) {
		__data = .;
		*(EXCLUDE_FILE(src/kernel/boot.o) .data)

		*(.data* .gnu.linkonce.d*)
		*(.eh_frame*)
		__edata = .;
	}
	.bss  ALIGN(4096) : AT(ADDR(.bss) - KERNEL_VMA) {
		__bss = .;
		
		/* reserve some additional space for bootstrap stack */
		__boot_stack_end = .;
		. = . + 4096;
		__boot_stack_top = .;

		*(EXCLUDE_FILE(src/kernel/boot.o) .bss)

		*(.bss* .gnu.linkonce.b*)
		*(.COMMON*)
		__ebss = .;
	}
	__end = .;

	/DISCARD/ : {
		*(.note*)
		*(.comment)
		*(.gnu_debuglink)
	}
}
