#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/mqueue.h>
#include <linux/hardirq.h>

#include <asm/thread_info.h>
#include <asm/uaccess.h>

static struct signal_struct init_signals = INIT_SIGNALS(init_signals);
static struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);

/*
 * Initial thread structure.
 *
 * We need to make sure that this is 8192-byte aligned due to the
 * way process stacks are handled. This is done by making sure
 * the linker maps this in the .text segment right after head.S,
 * and making head.S ensure the proper alignment.
 *
 * The things we do for performance..
 */
union thread_union init_thread_union __init_task_data =
{
{
	.task =		&init_task,
	.exec_domain =	&default_exec_domain,
	.flags =	0,
	.cpu =		0,
	.preempt_count = INIT_PREEMPT_COUNT,
	.restart_block	= {
		.fn = do_no_restart_syscall,
	},
	.addr_limit = KERNEL_DS,
}
};

/*
 * Initial task structure.
 *
 * All other task structs will be allocated on slabs in fork.c
 */
struct task_struct init_task = INIT_TASK(init_task);
EXPORT_SYMBOL(init_task);
