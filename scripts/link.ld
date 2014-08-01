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

		KEEP (src/kernel/bootstrap/*.o (.multiboot))
		KEEP (src/kernel/bootstrap/*.o (.text))
		KEEP (src/kernel/bootstrap/*.o (.data))
		KEEP (src/kernel/bootstrap/*.o (.bss))

		KEEP (*(.kickstart))
		KEEP (*(.bootstrap))

		__ebootstrap = .;
	}
	
	. = ALIGN(4096);
	. += KERNEL_VMA;

	.text : AT(ADDR(.text) - KERNEL_VMA) {
		__text = .;
		*(EXCLUDE_FILE(src/kernel/boostrap/*.o) .text)

		PROVIDE_HIDDEN(__init_start = .);
		KEEP (*(.init))
		PROVIDE_HIDDEN(__init_end = .);

		PROVIDE_HIDDEN(__fini_start = .);
		KEEP (*(.fini))
		PROVIDE_HIDDEN(__fini_end = .);

		*(.text* .gnu.linkonce.t*)
		*(.rodata* .gnu.linkonce.r*)
		__etext = .;
	}
	.data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VMA) {
		__data = .;
		*(EXCLUDE_FILE(src/kernel/boostrap/*.o) .data)

		*(.data* .gnu.linkonce.d*)

		__edata = .;
	}
	.bss  ALIGN(4096) : AT(ADDR(.bss) - KERNEL_VMA) {
		__bss = .;
		
		/* reserve some additional space for bootstrap stack */
		__boot_stack_end = .;
		. = . + 1024;
		__boot_stack_top = .;

		*(EXCLUDE_FILE(src/kernel/boostrap/*.o) .bss)

		*(.bss* .gnu.linkonce.b*)
		*(.COMMON*)
		__ebss = .;
	}
	__end = .;

	/DISCARD/ : {
		*(.note*)
		*(.comment)
		*(.eh_frame)
		*(.gnu_debuglink)
	}

	/* export absolute addresses for the 32bit bootstrap / multiboot header */
	__MB_LOAD_ADDR      = KERNEL_LMA;
	__MB_LOAD_END       = __edata - KERNEL_VMA;
	__MB_BSS_END        = __end   - KERNEL_VMA;
	__MB_BOOT_STACK_END = __boot_stack_end - KERNEL_VMA;
	__MB_BOOT_STACK_TOP = __boot_stack_top - KERNEL_VMA;
}
