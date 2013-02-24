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

#include <linux/ptrace.h>
#include <asm/uaccess.h>

void ptrace_disable(struct task_struct *child)
{
	/* nothing todo - we have no single step */
}

static int ptrace_getregs(struct task_struct *child, unsigned long __user *data)
{
	struct pt_regs *regs = task_pt_regs(child);

	return copy_to_user(data, regs, sizeof(regs));
}

static int ptrace_setregs (struct task_struct *child, unsigned long __user *data)
{
	struct pt_regs *regs = task_pt_regs(child);

	return copy_from_user(regs, data, sizeof(regs));
}

long arch_ptrace(struct task_struct *child, long request, unsigned long addr,
	unsigned long data)
{
	unsigned long tmp = 0;

	switch (request) {
	/* Read the word at location addr in the USER area. */
	case PTRACE_PEEKUSR: {


		switch (addr) {
		case 0 ... 31:
			tmp = *(((unsigned long *)task_pt_regs(child)) + addr);
			break;
		case PT_TEXT_ADDR:
			tmp = child->mm->start_code;
			break;
		case PT_TEXT_END_ADDR:
			tmp = child->mm->end_code;
			break;
		case PT_DATA_ADDR:
			tmp = child->mm->start_data;
			break;
		default:
			printk("ptrace attempted to PEEKUSR at %lx\n", addr);
			return -EIO;
		}
		return put_user(tmp, (unsigned long __user *)data);
	}
	case PTRACE_POKEUSR:
		switch (addr) {
		case 0 ... 31:
			*(((unsigned long *)task_pt_regs(child)) + addr) = data;
			break;
		default:
			printk("ptrace attempted to POKEUSR at %lx\n", addr);
			return -EIO;
		}
		break;

	/* when I and D space are separate, this will have to be fixed. */
	case PTRACE_POKETEXT: /* write the word at location addr. */
	case PTRACE_POKEDATA:
		//printk("PTRACE_POKE* [%s] *0x%lx = 0x%lx\n", child->comm, addr, data);
		if (access_process_vm(child, addr, &data, sizeof(data), 1)
		    != sizeof(data))
			return -EIO;
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("wcsr ICC, r0");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("wcsr DCC, r0");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		break;

	/* TODO: Implement regset and use the generic PTRACE_{GET,SET}REGS instead */
	case PTRACE_GETREGS:
		return ptrace_getregs (child, (unsigned long __user *) data);
	case PTRACE_SETREGS:
		return ptrace_setregs (child, (unsigned long __user *) data);
	default:
		return ptrace_request(child, request, addr, data);
	}

	return 0;
}

