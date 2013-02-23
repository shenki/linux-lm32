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

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/utsname.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>

/* the args to sys_lm32_clone try to match the libc call to avoid register
 * reshuffling:
 *   int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg); */
asmlinkage int sys_clone(
		int _unused_fn,
		unsigned long newsp,
		unsigned long clone_flags,
		int _unused_arg,
		unsigned long _unused_r5,
		int _unused_r6,
		struct pt_regs *regs)
{
	if (!newsp)
		newsp = current->thread.usp;

	return do_fork(clone_flags, newsp, regs, 0, NULL, NULL);
}

