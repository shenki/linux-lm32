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


asmlinkage long
sys_mmap2(unsigned long addr, unsigned long len, unsigned long prot,
	unsigned long flags, unsigned long fd, unsigned long pgoff)
{
	return sys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}

asmlinkage long
sys_mmap(unsigned long addr, unsigned long len, unsigned long prot,
	unsigned long flags, unsigned long fd, off_t offset)
{
	if (unlikely(offset & ~PAGE_MASK))
		return -EINVAL;
	return sys_mmap_pgoff(addr, len, prot, flags, fd, offset >> PAGE_SHIFT);
}

int kernel_execve(const char *filename, const char *const argv[], const char *const envp[])
{
    register unsigned int  r_syscall	asm("r8") = __NR_execve; 
    register long          r_a          asm("r1") = (unsigned long)filename; 
    register long          r_b          asm("r2") = (unsigned long)argv; 
    register long          r_c          asm("r3") = (unsigned long)envp; 
             long          __res; 

    asm volatile ( "scall\n" 
		   "mv %0, r1\n" 
		   : "=r"(__res) 
		   : "r"(r_syscall), 
		     "r"(r_a), "r"(r_b), "r"(r_c) );

    if(__res >=(unsigned long) -4095) {
      __res = (unsigned long) -1;
    }
    return (long) __res;
}
EXPORT_SYMBOL(kernel_execve);


extern asmlinkage int sys_execve(char *name, char **argv, char **envp, struct pt_regs* regs);

asmlinkage int sys_lm32_vfork(struct pt_regs *regs, unsigned long ra_in_syscall)
{
	int ret;

	//printk("do_fork regs=%lx ra=%lx usp=%lx\n", regs, ra_to_syscall_entry, usp);

	/* save ra_in_syscall to r1, this register will not be restored or overwritten (TODO find out)*/
	regs->r1 = ra_in_syscall;
	ret = do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, current->thread.usp, regs, 0, NULL, NULL);
	//printk("do_fork returned %d\n", ret);
	return ret;
}

/* the args to sys_lm32_clone try to match the libc call to avoid register
 * reshuffling:
 *   int clone(int (*fn)(void *arg), void *child_stack, int flags, void *arg); */
asmlinkage int sys_lm32_clone(
		int _unused_fn,
		unsigned long newsp,
		unsigned long clone_flags,
		int _unused_arg,
		unsigned long ra_in_syscall,
		int _unused_r6,
		struct pt_regs *regs)
{
	register unsigned long r_sp asm("sp");
	int ret;

	/* -12 because the function and the argument to the child is stored on
		 the stack, see clone.S in uClibc */
	if( !newsp ) {
	  newsp = r_sp - 12;
	}
	/* ret_from_fork will return to this address (in child), see copy_thread */
	regs->r1 = ra_in_syscall;
	ret = do_fork(clone_flags, newsp, regs, 0, NULL, NULL);
	return ret;
}

