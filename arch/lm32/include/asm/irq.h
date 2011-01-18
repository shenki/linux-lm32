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

#ifndef _LM32_ASM_IRQ_H
#define _LM32_ASM_IRQ_H

#include <asm/atomic.h>

#define NR_IRQS 32
#include <asm-generic/irq.h>

#define	NO_IRQ 0

static inline uint32_t lm32_irq_pending(void)
{
	uint32_t ip;
	__asm__ __volatile__("rcsr %0, IP" : "=r"(ip) : );
	return ip;
}

static inline void lm32_irq_ack(unsigned int irq)
{
	uint32_t mask = (1 << irq);
	__asm__ __volatile__("wcsr IP, %0" : : "r"(mask) );
}

#endif /* _LM32_ASM_IRQ_H_ */
