/* thread_info.h: LM32 low-level thread information
 * adapted from the h8300 versions by Takeshi Matsuya <macchan@sfc.wide.ad.jp>
 * adapted from the i386 and PPC versions by Yoshinori Sato <ysato@users.sourceforge.jp>
 * Copyright (C) 2002  David Howells (dhowells@redhat.com)
 * - Incorporating suggestions made by Linus Torvalds and Dave Miller
 */

#ifndef _ASM_LM32_THREAD_INFO_H
#define _ASM_LM32_THREAD_INFO_H

#include <asm/page.h>

#ifdef __KERNEL__

/*
 * Size of kernel stack for each process. This must be a power of 2...
 */
#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		8192	/* 2 pages */
#define PREEMPT_ACTIVE		0x10000000

#ifndef __ASSEMBLY__

typedef struct {
	unsigned long seg;
} mm_segment_t;

/*
 * low level task data.
 * If you change this, change the TI_* offsets below to match.
 */
struct thread_info {
	struct task_struct	*task;		/* main task structure */
	struct exec_domain	*exec_domain;	/* execution domain */
	unsigned long		flags;		/* low level flags */
	unsigned int		cpu;		/* cpu we're on */
	int			preempt_count;	/* 0 => preemptable, <0 => BUG */
	struct restart_block	restart_block;
	mm_segment_t		addr_limit;
};

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)


/* how to get the thread information struct from C */
static inline struct thread_info *current_thread_info(void) __pure;

extern struct thread_info* lm32_current_thread;

static inline struct thread_info *current_thread_info(void)
{
	register unsigned long sp asm ("sp");
	return (struct thread_info *)(sp & ~(THREAD_SIZE - 1));
}


#define INIT_THREAD_INFO(tsk)			\
{						\
	.task =		&init_task,		\
	.exec_domain =	&default_exec_domain,	\
	.flags =	0,			\
	.cpu =		0,			\
	.preempt_count = INIT_PREEMPT_COUNT,	\
	.addr_limit = KERNEL_DS,		\
	.restart_block	= {			\
		.fn = do_no_restart_syscall,	\
	},					\
}

#endif /* __ASSEMBLY__ */

/*
 * thread information flag bit numbers
 */
#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_SIGPENDING		1	/* signal pending */
#define TIF_NEED_RESCHED	2	/* rescheduling necessary */
#define TIF_POLLING_NRFLAG	3	/* true if poll_idle() is polling
					   TIF_NEED_RESCHED */
#define TIF_MEMDIE		4
#define TIF_RESTORE_SIGMASK	5	/* restore signal mask in do_signal() */
#define TIF_NOTIFY_RESUME	6	/* callback before returning to user */
#define TIF_FREEZE		16	/* is freezing for suspend */

/* as above, but as bit values */
#define _TIF_SYSCALL_TRACE	(1<<TIF_SYSCALL_TRACE)
#define _TIF_SIGPENDING		(1<<TIF_SIGPENDING)
#define _TIF_NEED_RESCHED	(1<<TIF_NEED_RESCHED)
#define _TIF_POLLING_NRFLAG	(1<<TIF_POLLING_NRFLAG)
#define _TIF_RESTORE_SIGMASK	(1<<TIF_RESTORE_SIGMASK)
#define _TIF_NOTIFY_RESUME	(1 << TIF_NOTIFY_RESUME)
#define _TIF_FREEZE		(1<<TIF_FREEZE)

#define _TIF_WORK_MASK		0x0000FFFE	/* work to do on interrupt/exception return */

#endif /* __KERNEL__ */

#endif /* _ASM_LM32_THREAD_INFO_H */
