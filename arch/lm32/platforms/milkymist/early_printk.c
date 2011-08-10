/*
 * (C) Copyright 2009
 *     Sebastien Bourdeauducq
 * (C) Copyright 2007
 *     Theobroma Systems <www.theobroma-systems.com>
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
 *
 * Based on
 *
 * arch/mips/kernel/early_printk.c
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2002, 2003, 06, 07 Ralf Baechle (ralf@linux-mips.org)
 * Copyright (C) 2007 MIPS Technologies, Inc.
 *   written by Ralf Baechle (ralf@linux-mips.org)
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/io.h>

#define UART_RXTX     (void*)0xe0000000
#define UART_DIVISOR  (void*)0xe0000004
#define UART_STAT     (void*)0xe0000008
#define UART_CTRL     (void*)0xe000000c
#define UART_DEBUG    (void*)0xe000000c

#define UART_STAT_THRE   (1<<0)
#define UART_STAT_RX_EVT (1<<1)
#define UART_STAT_TX_EVT (1<<2)

static void __init early_console_putc(char c)
{
	unsigned int timeout = 1000;
	uint32_t stat;

	iowrite32be(c, UART_RXTX);

	do {
		stat = ioread32be(UART_STAT);
	} while (!(stat & UART_STAT_THRE) && --timeout);
}

static void __init early_console_write(struct console *con, const char *s,
	unsigned n)
{
	while (n-- && *s) {
		early_console_putc(*s);
		s++;
	}
}

static struct console early_console __initdata = {
	.name	= "early",
	.write	= early_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT,
	.index	= -1
};

static bool early_console_initialized __initdata;

void __init milkymist_setup_early_printk(void)
{
	if (early_console_initialized)
		return;
	early_console_initialized = true;

	register_console(&early_console);
}
