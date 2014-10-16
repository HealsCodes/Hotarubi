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

		KEEP (kernel/boot.o (.multiboot))
		KEEP (kernel/boot.o (.text))
		KEEP (kernel/boot.o (.data))
		KEEP (kernel/boot.o (.bss))
	}
	
	. = ALIGN(4096);
	__ebootstrap = .;
	
	. += KERNEL_VMA;

	.text : AT(ADDR(.text) - KERNEL_VMA) {
		__text = .;
		*(EXCLUDE_FILE(kernel/boot.o) .text)

		KEEP (*(.init))
		KEEP (*(.fini))
		KEEP (*(SORT(.ctors*)))
		KEEP (*(SORT(.dtors*)))

		*(.text*)
		*(.gnu.linkonce.t*)
		*(.rodata*)
		*(.gnu.linkonce.r*)

		. = ALIGN(4096);
		__irqstub = .;
		*(.irqstub*)

		__etext = .;
	}
	.data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VMA) {
		__data = .;
		*(EXCLUDE_FILE(kernel/boot.o) .data)

		*(.data*)
		*(.gnu.linkonce.d*)
		
		__edata = .;
	}
	.bss  ALIGN(4096) : AT(ADDR(.bss) - KERNEL_VMA) {
		__bss = .;
		*(EXCLUDE_FILE(kernel/boot.o) .bss)

		*(.bss)
		*(.gnu.linkonce.b*)
		*(COMMON)

		/* reserve some additional space for bootstrap stack */
		. = ALIGN(16);
		__boot_stack_end = .;
		. = . + 8192;
		__boot_stack_top = .;
		. = ALIGN(16);

		__ebss = .;
	}
	__end = .;

	/DISCARD/ : {
		*(.note)
		*(.comment)
		*(.group)
		*(.eh_frame)
		*(.jcr)
		*(.gnu_debuglink)
		/* these are only present in crt*-kernel.o */
		*(.rela*)
		*(.igot*)
		*(.iplt*)
	}
}
