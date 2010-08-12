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
 * linux/arch/m68knommu/kernel/time.c
 *
 *  Copyright (C) 1991, 1992, 1995  Linus Torvalds
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/profile.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/interrupt.h>

//#include <asm/machdep.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq_regs.h>
#include <asm/setup.h>
#include <asm/uaccess.h>

#ifdef CONFIG_SMP
extern void smp_local_timer_interrupt(void);
#endif

#define CSR_TIMER0_CONTROL	0x80001010
#define CSR_TIMER0_COMPARE	0x80001014
#define CSR_TIMER0_COUNTER	0x80001018

#define TIMER_ENABLE		0x01
#define TIMER_AUTORESTART	0x02

cycles_t lm32_cycles = 0;

static irqreturn_t timer_interrupt(int irq, void *timer_idx);

/* irq action description */
static struct irqaction lm32_core_timer_irqaction = {
	.name = "LM32 Timer Tick",
	.flags = IRQF_DISABLED,
	.handler = timer_interrupt,
};

/*
 * timer_interrupt() needs to call the "do_timer()"
 * routine every clocktick
 */
static irqreturn_t timer_interrupt(int irq, void *arg)
{
	lm32_cycles += in_be32((u32 *)CSR_TIMER0_COMPARE);
	write_seqlock(&xtime_lock);

	do_timer(1);
#ifndef CONFIG_SMP
	update_process_times(user_mode(0));
#endif

#ifdef CONFIG_SMP
	smp_local_timer_interrupt();
	smp_send_timer();
#endif

	if (current->pid)
		profile_tick(CPU_PROFILING);

	write_sequnlock(&xtime_lock);
	return(IRQ_HANDLED);
}

void time_init(void)
{
	lm32_systimer_program(1, cpu_frequency / HZ);

	if( setup_irq(IRQ_SYSTMR, &lm32_core_timer_irqaction) )
		panic("could not attach timer interrupt!");

	lm32_irq_unmask(IRQ_SYSTMR);
}

static unsigned long get_time_offset(void)
{
	return in_be32((u32 *)CSR_TIMER0_COUNTER)/(cpu_frequency / HZ);
}

cycles_t get_cycles(void)
{
	return lm32_cycles +
		in_be32(CSR_TIMER0_COUNTER);
}

void lm32_systimer_program(int periodic, cycles_t cyc)
{
	/* stop timer */
	out_be32(CSR_TIMER0_CONTROL,0);
	/* reset/configure timer */
	out_be32(CSR_TIMER0_COUNTER,0);
	out_be32(CSR_TIMER0_COMPARE,(int)cyc);
	/* start timer */
	out_be32(CSR_TIMER0_CONTROL,periodic ? TIMER_ENABLE|TIMER_AUTORESTART : TIMER_ENABLE);
}
