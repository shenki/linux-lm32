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
#include <linux/tracehook.h>

#include <asm/uaccess.h>
#include <asm/ucontext.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/thread_info.h>
#include <asm/cacheflush.h>

#define DEBUG_SIG 0

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

asmlinkage int sys_rt_sigreturn(void)
{
	struct pt_regs *regs = current_pt_regs();
	struct rt_sigframe __user *frame = (struct rt_sigframe __user *)(regs->sp + 4);
	sigset_t set;

	current_thread_info()->restart_block.fn = do_no_restart_syscall;

	if (!access_ok(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;

	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	set_current_blocked(&set);

	if (restore_sigcontext(regs, &frame->uc.uc_mcontext))
		goto badframe;

	if (restore_altstack(&frame->uc.uc_stack))
		goto badframe;

	return regs->r1;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Set up a signal frame.
 */
static int setup_sigcontext(struct sigcontext __user *sc, struct pt_regs *regs,
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
static inline void __user *get_sigframe(struct ksignal *ksig,
		struct pt_regs *regs, size_t frame_size)
{
	unsigned long sp = sigsp(regs->sp, ksig);

	return (void __user *)((sp - frame_size) & ~7UL);
}

static int setup_rt_frame(struct ksignal *ksig, sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	int err = 0;

	frame = get_sigframe(ksig, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		return 1;

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
		return err;

	flush_icache_range(&frame->tramp, &frame->tramp + 2);

	/* set return address for signal handler to trampoline */
	regs->ra = (unsigned long)(&frame->tramp[0]);

	/* Set up registers for returning to signal handler */
	/* entry point */
	regs->ea = (unsigned long)ksig->ka.sa.sa_handler - 4;
	/* stack pointer */
	regs->sp = (unsigned long)frame - 4;
	/* Signal handler arguments */
	regs->r1 = ksig->sig;     /* first argument = signum */
	regs->r2 = (unsigned long)&frame->info;
	regs->r3 = (unsigned long)&frame->uc;

#if DEBUG_SIG
	printk("SIG deliver (%s:%d): frame=%p, sp=%p ra=%08lx ea=%08lx, signal(r1)=%d\n",
	       current->comm, current->pid, frame, regs->sp, regs->ra, regs->ea, sig);
#endif

	return 0;
}

static void handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	sigset_t *oldset = sigmask_to_save();
	int ret;

	ret = setup_rt_frame(ksig, oldset, regs);
	signal_setup_done(ret, ksig, 0);
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
static void do_signal(struct pt_regs *regs, unsigned int in_syscall)
{
	struct ksignal ksig;

	/* Did we come from a system call? */

	if (get_signal(&ksig)) {
		if (in_syscall) {
			switch (regs->r1) {
			case -ERESTART_RESTARTBLOCK:
			case -ERESTARTNOHAND:
				regs->r1 = -EINTR;
				break;
			case -ERESTARTSYS:
				if (!(ksig.ka.sa.sa_flags & SA_RESTART)) {
					regs->r1 = -EINTR;
					break;
				}
				/* fallthrough */
			case -ERESTARTNOINTR:
				regs->ea -= 4; /* Size of scall insn.  */
				regs->r1 = regs->orig_r1;
				break;
			default:
				break;
			}
		}
		handle_signal(&ksig, regs);
	} else {
		/* If there is no handler always restart */
		if (in_syscall) {
			switch (regs->r1) {
			case -ERESTART_RESTARTBLOCK:
				regs->r8 = __NR_restart_syscall;
				/* fallthrough */
			case -ERESTARTNOHAND:
			case -ERESTARTSYS:
			case -ERESTARTNOINTR:
				regs->ea -= 4; /* Size of scall insn.  */
				regs->r1 = regs->orig_r1;
				break;
			default:
				break;
			}
		}
		restore_saved_sigmask();
	}
}

asmlinkage void do_notify_resume(struct pt_regs *regs, unsigned int in_syscall,
	unsigned int thread_flags)
{
	local_irq_enable();

	if (thread_flags & _TIF_SIGPENDING) {
		do_signal(regs, in_syscall);
	} else {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}
