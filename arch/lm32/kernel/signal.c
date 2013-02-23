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
 * Based on
 *
 *  arch/v850/kernel/signal.c
 *  Copyright (C) 2001,02,03  NEC Electronics Corporation
 *  Copyright (C) 2001,02,03  Miles Bader <miles@gnu.org>
 *  Copyright (C) 1999,2000,2002  Niibe Yutaka & Kaz Kojima
 *  Copyright (C) 1991,1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of this
 * archive for more details.
 *
 * 1997-11-28  Modified for POSIX.1b signals by Richard Henderson
 */

#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/personality.h>
#include <linux/tty.h>
#include <linux/hardirq.h>
#include <linux/freezer.h>

#include <asm/uaccess.h>
#include <asm/ucontext.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/thread_info.h>
#include <asm/cacheflush.h>

#define DEBUG_SIG 0

asmlinkage int manage_signals(int retval, struct pt_regs* regs);

struct rt_sigframe {
	struct siginfo info;
	struct ucontext uc;
	unsigned long tramp[2]; /* signal trampoline */
};

static int restore_sigcontext(struct pt_regs *regs,
				struct sigcontext __user *sc)
{
	return __copy_from_user(regs, &sc->regs, sizeof(*regs));
}

asmlinkage int _sys_rt_sigreturn(struct pt_regs *regs)
{
	struct rt_sigframe __user *frame = (struct rt_sigframe __user *)(regs->sp + 4);
	sigset_t set;
	stack_t st;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;

	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->uc.uc_mcontext))
		goto badframe;

	if (__copy_from_user(&st, &frame->uc.uc_stack, sizeof(st)))
		goto badframe;
	do_sigaltstack(&st, NULL, regs->sp);

	return regs->r1;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Set up a signal frame.
 */
static int setup_sigcontext(struct sigcontext *sc, struct pt_regs *regs,
		 unsigned long mask)
{
	int err;

	err = __copy_to_user(&sc->regs, regs, sizeof(*regs));
	err |= __put_user(mask, &sc->oldmask);

	return err;
}

/*
 * Determine which stack to use..
 */
static inline void __user *get_sigframe(struct k_sigaction *ka,
		struct pt_regs *regs, size_t frame_size)
{
	unsigned long sp = regs->sp;

	if ((ka->sa.sa_flags & SA_ONSTACK) && !sas_ss_flags(sp))
		/* use stack set by sigaltstack */
		sp = current->sas_ss_sp + current->sas_ss_size;

	return (void __user *)((sp - frame_size) & ~7UL);
}

static int setup_rt_frame(int sig, struct k_sigaction *ka,
			sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	int err = 0;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	err |= __clear_user(&frame->uc, sizeof(frame->uc));
	err |= __save_altstack(&frame->uc.uc_stack, regs->sp);
	err |= setup_sigcontext(&frame->uc.uc_mcontext, regs, set->sig[0]);

	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));

	/* Set up to return from userspace. */
	/* mvi  r8, __NR_rt_sigreturn = addi  r8, r0, __NR_sigreturn */
	err |= __put_user(0x34080000 | __NR_rt_sigreturn, &frame->tramp[0]);

	/* scall */
	err |= __put_user(0xac000007, &frame->tramp[1]);

	if (err)
		goto give_sigsegv;

	/* flush instruction cache */
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("wcsr ICC, r0");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	/* set return address for signal handler to trampoline */
	regs->ra = (unsigned long)(&frame->tramp[0]);

	/* Set up registers for returning to signal handler */
	/* entry point */
	regs->ea = (unsigned long)ka->sa.sa_handler - 4;
	/* stack pointer */
	regs->sp = (unsigned long)frame - 4;
	/* Signal handler arguments */
	regs->r1 = sig;     /* first argument = signum */
	regs->r2 = (unsigned long)&frame->info;
	regs->r3 = (unsigned long)&frame->uc;

#if DEBUG_SIG
	printk("SIG deliver (%s:%d): frame=%p, sp=%p ra=%08lx ea=%08lx, signal(r1)=%d\n",
	       current->comm, current->pid, frame, regs->sp, regs->ra, regs->ea, sig);
#endif

	return regs->r1;

give_sigsegv:
	force_sigsegv(sig, current);

	return -1;
}

static void handle_signal(unsigned long sig, siginfo_t *info,
		struct k_sigaction *ka, struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();

	setup_rt_frame(sig, ka, oldset, regs);
	signal_delivered(sig, info, ka, regs, 0);
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
static int do_signal(int retval, struct pt_regs *regs)
{
	siginfo_t info;
	int signr;
	struct k_sigaction ka;

	/*
	 * We want the common case to go fast, which
	 * is why we may in certain cases get here from
	 * kernel mode. Just return without doing anything
	 * if so.
	 */
	if (!user_mode(regs))
		return 0;

	if (try_to_freeze())
		goto no_signal;

	signr = get_signal_to_deliver(&info, &ka, regs, NULL);

	if (signr > 0) {
		/* Whee!  Actually deliver the signal.  */
		handle_signal(signr, &info, &ka, regs);
		return signr;
	}

no_signal:
	/* Did we come from a system call? */
	if (regs->r8) {
		switch (retval) {
		case -ERESTART_RESTARTBLOCK:
			regs->r8 = __NR_restart_syscall;
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs->ea -= 4; /* Size of scall insn.  */
			break;
		default:
			break;
		}
	}
	restore_saved_sigmask();

	return retval;
}

asmlinkage int manage_signals(int retval, struct pt_regs *regs)
{
	unsigned long flags;

	if (!user_mode(regs))
		return retval;

	/* disable interrupts for sampling current_thread_info()->flags */
	local_irq_save(flags);
	while (current_thread_info()->flags & (_TIF_NEED_RESCHED | _TIF_SIGPENDING)) {
		if (current_thread_info()->flags & _TIF_NEED_RESCHED) {
			/* schedule -> enables interrupts */
			schedule();

			/* disable interrupts for sampling current_thread_info()->flags */
			local_irq_disable();
		}

		if (current_thread_info()->flags & _TIF_SIGPENDING) {
#if DEBUG_SIG
			/* debugging code */
			{
				register unsigned long sp asm("sp");
				printk("WILL process signal for %s with regs=%lx, ea=%lx, ba=%lx ra=%lx\n",
						current->comm, regs, regs->ea, regs->ba, *((unsigned long*)(sp+4)));
			}
#endif
			retval = do_signal(retval, regs);

			/* signal handling enables interrupts */

			/* disable irqs for sampling current_thread_info()->flags */
			local_irq_disable();
#if DEBUG_SIG
			/* debugging code */
			{
				register unsigned long sp asm("sp");
				printk("Processed Signal for %s with regs=%lx, ea=%lx, ba=%lx ra=%lx\n",
						current->comm, regs, regs->ea, regs->ba, *((unsigned long*)(sp+4)));
			}
#endif
		}
	}
	local_irq_restore(flags);

	return retval;
}

asmlinkage void manage_signals_irq(struct pt_regs *regs)
{
	unsigned long flags;

	if (!user_mode(regs))
		return;

	/* disable interrupts for sampling current_thread_info()->flags */
	local_irq_save(flags);

	if( current_thread_info()->flags & _TIF_NEED_RESCHED ) {
		/* schedule -> enables interrupts */
		schedule();
	}

	local_irq_restore(flags);
}
