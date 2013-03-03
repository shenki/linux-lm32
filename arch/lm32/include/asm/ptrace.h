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

#ifndef _ASM_LM32_PTRACE_H
#define _ASM_LM32_PTRACE_H

#define PT_MODE_KERNEL 1
#define PT_MODE_USER   0

#ifndef __ASSEMBLY__

typedef unsigned long lm32_reg_t;

struct pt_regs {
	lm32_reg_t r1;
	lm32_reg_t r2;
	lm32_reg_t r3;
	lm32_reg_t r4;
	lm32_reg_t r5;
	lm32_reg_t r6;
	lm32_reg_t r7;
	lm32_reg_t r8;
	lm32_reg_t r9;
	lm32_reg_t r10;
	lm32_reg_t r11;
	lm32_reg_t r12;
	lm32_reg_t r13;
	lm32_reg_t r14;
	lm32_reg_t r15;
	lm32_reg_t r16;
	lm32_reg_t r17;
	lm32_reg_t r18;
	lm32_reg_t r19;
	lm32_reg_t r20;
	lm32_reg_t r21;
	lm32_reg_t r22;
	lm32_reg_t r23;
	lm32_reg_t r24;
	lm32_reg_t r25;
	lm32_reg_t gp;
	lm32_reg_t fp;
	lm32_reg_t sp;
	lm32_reg_t ra;
	lm32_reg_t ea;
	lm32_reg_t ba;
	unsigned int pt_mode;
	lm32_reg_t orig_r1;
};

#ifdef __KERNEL__
#define user_mode(regs) ((regs)->pt_mode == PT_MODE_USER)

#define user_stack_pointer(regs) ((regs)->sp)
#define instruction_pointer(regs) ((regs)->ea)
#define profile_pc(regs) instruction_pointer(regs)

void show_regs(struct pt_regs *);

#else /* !__KERNEL__ */

/* TBD (gdbserver/ptrace) */

#endif /* !__KERNEL__ */

/* FIXME: remove this ? */
/* Arbitrarily choose the same ptrace numbers as used by the Sparc code. */
#define PTRACE_GETREGS            12
#define PTRACE_SETREGS            13

#define PTRACE_GETFDPIC	          31

#define PTRACE_GETFDPIC_EXEC      0
#define PTRACE_GETFDPIC_INTERP    1

/* for gdb */
#define PT_TEXT_ADDR	50
#define PT_TEXT_END_ADDR	51
#define PT_DATA_ADDR	52

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_LM32_PTRACE_H */
