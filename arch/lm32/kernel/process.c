/*
 * (C) Copyright 2007
 *     Theobroma Systems <www.theobroma-systems.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/user.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/io.h>

#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/hw/milkymist.h>

asmlinkage void ret_from_fork(void);
asmlinkage void syscall_tail(void);


struct thread_info* lm32_current_thread;

/*
 * The following aren't currently used.
 */
void (*pm_idle)(void);
EXPORT_SYMBOL(pm_idle);

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

/*
 * The idle loop on an LM32
 */
static void default_idle(void)
{
 	while(!need_resched())
		__asm__ __volatile__("and r0, r0, r0" ::: "memory");
}

void (*idle)(void) = default_idle;

/*
 * The idle thread. There's no useful work to be
 * done, so just try to conserve power and have a
 * low exit latency (ie sit in a loop waiting for
 * somebody to say that they'd like to reschedule)
 */
void cpu_idle(void)
{
	/* endless idle loop with no priority at all */
	while (1) {
		idle();
		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}

void machine_restart(char * __unused)
{
	/* Writing to CSR_SYSTEM_ID causes a system reset */
	iowrite32be(1, CSR_SYSTEM_ID);
	while(1);
}

void machine_halt(void)
{
	printk("%s:%d: machine_halt() is not possible on lm32\n", __FILE__, __LINE__);
	for (;;)
		cpu_relax();
}

void machine_power_off(void)
{
	printk("%s:%d: machine_poweroff() is not possible on lm32\n", __FILE__, __LINE__);
	for (;;)
		cpu_relax();
}

static const char * const lm32_reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "gp", "fp", "sp", "ra", "ea", "ba"
};

void show_regs(struct pt_regs *regs)
{
	unsigned long *reg = (unsigned long *)regs;
	unsigned int i;

	printk("Registers:\n");

	for (i = 0; i < 32; ++i)
		printk("%3s: 0x%lx\n", lm32_reg_names[i], reg[i]);
}

static void kernel_thread_helper(int (*fn)(void*), void* arg)
{
	do_exit(fn(arg));
}

/*
 * Create a kernel thread
 */
int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	/* prepare registers from which a child task switch frame will be copied */
	struct pt_regs regs;

	set_fs(KERNEL_DS);

	memset(&regs, 0, sizeof(regs));

	//printk("kernel_thread fn=%x arg=%x regs=%x\n", fn, arg, &regs);

	regs.r11 = (unsigned long)fn;
	regs.r12 = (unsigned long)arg;
	regs.r13 = (unsigned long)kernel_thread_helper;
	regs.pt_mode = PT_MODE_KERNEL;
	return do_fork(flags | CLONE_VM | CLONE_UNTRACED, 0, &regs, 0, NULL, NULL);
}

void flush_thread(void)
{
}

/* no stack unwinding */
unsigned long get_wchan(struct task_struct *p)
{
	return 0;
}

unsigned long thread_saved_pc(struct task_struct *tsk)
{
	return 0;
}

int copy_thread(unsigned long clone_flags,
		unsigned long usp, unsigned long stk_size,
		struct task_struct * p, struct pt_regs * regs)
{
	unsigned long child_tos = KSTK_TOS(p);
	struct pt_regs *childregs;

	if (!user_mode(regs)) {
		/* kernel thread */

		if( usp != 0 )
			panic("trying to start kernel thread with usp != 0");

		/* childregs = full task switch frame on kernel stack of child */
		childregs = (struct pt_regs *)(child_tos) - 1;
		*childregs = *regs;

		/* return via ret_from_fork */
		childregs->ra = (unsigned long)ret_from_fork;

		/* setup ksp/usp */
		p->thread.ksp = (unsigned long)childregs - 4; /* perhaps not necessary */
		childregs->sp = p->thread.ksp;
		p->thread.usp = 0;
		p->thread.which_stack = 0; /* kernel stack */

		//printk("copy_thread1: ->pid=%d tsp=%lx r5=%lx p->thread.ksp=%lx p->thread.usp=%lx\n",
		//		p->pid, task_stack_page(p), childregs->r5, p->thread.ksp, p->thread.usp);
	} else {
		/* userspace thread (vfork, clone) */

		struct pt_regs* childsyscallregs;

		/* childsyscallregs = full syscall frame on kernel stack of child */
		childsyscallregs = (struct pt_regs *)(child_tos) - 1; /* 32 = safety */
		/* child shall have same syscall context to restore as parent has ... */
		*childsyscallregs = *regs;

		/* childregs = full task switch frame on kernel stack of child below * childsyscallregs */
		childregs = childsyscallregs - 1;
		memset(childregs, 0, sizeof(childregs));

		/* user stack pointer is shared with the parent per definition of vfork */
		p->thread.usp = usp;

		/* kernel stack pointer is not shared with parent, it is the beginning of
		 * the just created new task switch segment on the kernel stack */
		p->thread.ksp = (unsigned long)childregs - 4;
		p->thread.which_stack = 0; /* resume from ksp */

		/* child returns via ret_from_fork */
		childregs->ra = (unsigned long)ret_from_fork;
		/* child shall return to where sys_vfork_wrapper has been called */
		childregs->r13 = (unsigned long)syscall_tail;
		/* child gets zero as return value from syscall */
		childregs->r11 = 0;
		/* after task switch segment return the stack pointer shall point to the
		 * syscall frame */
		childregs->sp = (unsigned long)childsyscallregs - 4;

		/*printk("copy_thread2: ->pid=%d p=%lx regs=%lx childregs=%lx r5=%lx ra=%lx "
				"dsf=%lx p->thread.ksp=%lx p->thread.usp=%lx\n",
				p->pid, p, regs, childregs, childregs->r5, childregs->ra,
				dup_syscallframe, p->thread.ksp, p->thread.usp);*/
	}

	return 0;
}

/* start userspace thread */
void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long usp)
{
	set_fs(USER_DS);

	memset(regs, 0, sizeof(regs));

	/* -4 because we will add 4 later in ret_from_syscall */
	regs->ea = pc - 4;
#ifdef CONFIG_BINFMT_ELF_FDPIC
	regs->r7 = current->mm->context.exec_fdpic_loadmap;
#endif
	regs->sp = usp;
	current->thread.usp = usp;
	regs->fp = current->mm->start_data;
	regs->pt_mode = PT_MODE_USER;

	/*printk("start_thread: current=%lx usp=%lx\n", current, usp);*/
}

