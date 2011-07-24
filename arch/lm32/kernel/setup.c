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
 * Partially based on
 *
 * linux/arch/m68knommu/kernel/setup.c
 */

/*
 * This file handles the architecture-dependent parts of system setup
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/string.h>
#include <linux/major.h>
#include <linux/initrd.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>


#include <asm/sections.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/setup.h>

unsigned int kernel_mode = PT_MODE_KERNEL;

char cmd_line[COMMAND_LINE_SIZE];

extern void setup_early_printk(void);

/* from mm/init.c */
extern void bootmem_init(void);

unsigned int cpu_frequency;

void __init machine_early_init(char *cmdline, unsigned long p_initrd_start,
		unsigned long p_initrd_end)
{
	/* clear bss section */
	memset(__bss_start, 0, __bss_stop - __bss_start);

#ifndef CONFIG_CMDLINE_BOOL
	if (cmdline) {
		strlcpy(cmd_line, cmdline, COMMAND_LINE_SIZE);
	}
#else
	strlcpy(cmd_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#endif

	initrd_start = p_initrd_start;
	initrd_end = p_initrd_end;

	early_init_devtree(__dtb_start);
	printk("initrd: %lx %lx\n", initrd_start, initrd_end);
	memblock_reserve(__pa(initrd_start), initrd_end - initrd_start);
}

void __init setup_arch(char **cmdline_p)
{
	/*
	 * init "current thread structure" pointer
	 */
	lm32_current_thread = (struct thread_info*)&init_thread_union;

	strlcpy(boot_command_line, cmd_line, COMMAND_LINE_SIZE);
	*cmdline_p = cmd_line;

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#ifdef CONFIG_EARLY_PRINTK
	setup_early_printk();
#endif

	/*
	 * Init boot memory
	 */
	bootmem_init();
	device_tree_init();

	/*
	 * Get kmalloc into gear.
	 */
	paging_init();
}
