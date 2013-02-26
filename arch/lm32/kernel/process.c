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

asmlinkage void ret_from_fork(void);
asmlinkage void ret_from_kernel_thread(void);
asmlinkage void syscall_tail(void);

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
		default_idle();
		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}

void __weak machine_restart(char * __unused)
{
	printk("machine_restart() is not possible on lm32\n");
	while (1)
		cpu_relax();
}

void __weak machine_halt(void)
{
	printk("machine_halt() is not possible on lm32\n");
	while (1)
		cpu_relax();
}

void __weak machine_power_off(void)
{
	printk("machine_poweroff() is not possible on lm32\n");
	while (1)
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

void flush_thread(void)
{
}

int copy_thread(unsigned long clone_flags, unsigned long usp_thread_fn,
	unsigned long thread_fn_arg, struct task_struct *p)
{
	struct pt_regs *childregs = task_pt_regs(p);
	struct cpu_context_save *cc = &task_thread_info(p)->cpu_context;

	memset(cc, 0, sizeof(*cc));
	cc->sp = (unsigned long)childregs - 4;

	if (p->flags & PF_KTHREAD) {
		memset(childregs, 0, sizeof(*childregs));
		childregs->pt_mode = PT_MODE_KERNEL;

		cc->r11 = usp_thread_fn;
		cc->r12 = thread_fn_arg;
		cc->ra = (unsigned long)ret_from_kernel_thread;
	} else {
		*childregs = *current_pt_regs();
		if (usp_thread_fn)
			childregs->sp = usp_thread_fn;

		cc->ra = (unsigned long)ret_from_fork;
	}

	return 0;
}

/* start userspace thread */
void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long usp)
{
	/* -4 because we will add 4 later in ret_from_syscall */
	regs->ea = pc - 4;
#ifdef CONFIG_BINFMT_ELF_FDPIC
	regs->r7 = current->mm->context.exec_fdpic_loadmap;
#endif
	regs->sp = usp;
	regs->fp = current->mm->start_data;
	regs->pt_mode = PT_MODE_USER;
}
