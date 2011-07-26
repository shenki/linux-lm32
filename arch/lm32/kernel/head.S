#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>

__INIT
ENTRY(_start)
	xor	r0, r0, r0      /* clear r0 */
	wcsr	IE, r0          /* disable interrupts */
	wcsr	IM, r0

	/* init stack pointer */
	mvhi	sp, hi(init_thread_union)
	ori	sp, sp, lo(init_thread_union)
	addi	sp, sp, THREAD_SIZE - 4

	/* terminate frame pointer */
	mvi	fp, 0

	/* clear bss */
	mvhi 	r10, hi(__bss_start)
	ori 	r10, r10, lo(__bss_start)
	mvhi 	r11, hi(__bss_stop)
	ori 	r11, r11, lo(__bss_stop)
1:
	be 	r10, r11, 2f
	sw 	(r10+0), r0
	addi 	r10, r10, 4
	bi 	1b
2:

	/*
	 * XXX: this will go away, if we have proper fdt support.
	 * Then, command line and initrd parameters are passed within the fdt
	 * blob
	 */
	/* save cmdline pointer */
	mvhi 	r10, hi(__cmdline)
	ori 	r10, r10, lo(__cmdline)
	sw	(r10+0), r1

	/* save initrd start and end pointer */
	mvhi 	r10, hi(initrd_start)
	ori 	r10, r10, lo(initrd_start)
	sw	(r10+0), r2

	mvhi 	r10, hi(initrd_end)
	ori 	r10, r10, lo(initrd_end)
	sw	(r10+0), r3

	bi	start_kernel
