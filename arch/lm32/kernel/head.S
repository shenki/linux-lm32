#include <linux/init.h>
#include <linux/linkage.h>
#include <asm/thread_info.h>

__INIT
ENTRY(_stext)
	xor	r0, r0, r0      /* clear r0 */
	wcsr	IE, r0          /* disable interrupts */
	wcsr	IM, r0

	/* init stack pointer */
	mvhi	sp, hi(init_thread_union)
	ori	sp, sp, lo(init_thread_union)
	addi	sp, sp, THREAD_SIZE - 4

	/* terminate frame pointer */
	mvi	fp, 0

	calli	machine_early_init
	bi	start_kernel
