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

struct lm32_state lm32_state = {
	.current_thread = (struct thread_info*)&init_thread_union,
	.kernel_mode = PT_MODE_KERNEL,
};

char __initdata cmd_line[COMMAND_LINE_SIZE];

/* from mm/init.c */
extern void bootmem_init(void);

unsigned int cpu_frequency;
char *__cmdline;

void __init __weak plat_setup_arch(void)
{
}

void __init setup_arch(char **cmdline_p)
{
	/* populate memory_start and memory_end, needed for bootmem_init() */
	early_init_devtree(__dtb_start);

	/* a cmdline set by the bootloader overrides one defined in the dts */
	if (__cmdline && strlen(__cmdline))
		strlcpy(cmd_line, __cmdline, COMMAND_LINE_SIZE);

	*cmdline_p = cmd_line;
	strlcpy(boot_command_line, cmd_line, COMMAND_LINE_SIZE);

	/* early commandline needed by memblock */
	parse_early_param();

	bootmem_init();

	device_tree_init();

	paging_init();

	plat_setup_arch();

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif
}
