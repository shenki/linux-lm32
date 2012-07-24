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

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/initrd.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/linkage.h>
#include <linux/pfn.h>
#include <linux/memblock.h>

#include <asm-generic/sections.h>
#include <asm/page.h>

unsigned long memory_start;
unsigned long memory_end;

void __init bootmem_init(void)
{
	struct memblock_region *reg;
	unsigned long bootmap_size;
	unsigned long free_pfn, end_pfn, start_pfn;

	init_mm.start_code = (unsigned long)_stext;
	init_mm.end_code = (unsigned long)_etext;
	init_mm.end_data = (unsigned long)_edata;
	init_mm.brk = (unsigned long)_end;

	memblock_add(memory_start, memory_end);

	if(((unsigned long)__pa(_end) < memory_start) || ((unsigned long)__pa(_end) > memory_end))
		printk("BUG: your kernel is not located in the ddr sdram");

	start_pfn = PFN_UP(memory_start);
	free_pfn = PFN_UP(__pa((unsigned long)_end));
	end_pfn = PFN_DOWN(memory_end);

	//memblock_reserve(PFN_PHYS(start_pfn), PFN_PHYS(free_pfn - start_pfn));
	memblock_reserve(__pa(_stext), _end - _stext);

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && initrd_end && initrd_start < initrd_end) {
		memblock_reserve(initrd_start, initrd_end - initrd_start);
	}
#endif

	bootmap_size = init_bootmem(free_pfn, end_pfn);
	memblock_reserve(PFN_PHYS(free_pfn), bootmap_size);

	free_bootmem(PFN_PHYS(free_pfn), PFN_PHYS(end_pfn - free_pfn));

	for_each_memblock(reserved, reg)
		reserve_bootmem(reg->base, reg->size, BOOTMEM_DEFAULT);

	memory_start += PAGE_OFFSET;
	memory_end += PAGE_OFFSET;

	memblock_allow_resize();
	memblock_dump_all();
}

/*
 * paging_init() continues the virtual memory environment setup which
 * was begun by the code in arch/head.S.
 * The parameters are pointers to where to stick the starting and ending
 * addresses of available kernel virtual memory.
 */
void __init paging_init(void)
{
	unsigned long zones_size[MAX_NR_ZONES] = {0, };

	zones_size[ZONE_NORMAL] = max_low_pfn;
	free_area_init(zones_size);
}

void __init mem_init(void)
{
	high_memory = (void *)__va(max_low_pfn * PAGE_SIZE);

	max_mapnr = num_physpages = max_low_pfn - min_low_pfn;

	totalram_pages = free_all_bootmem();

	printk(KERN_INFO "Memory available: %luk/%luk RAM, (%dk kernel code, %dk data)\n",
	       nr_free_pages() << (PAGE_SHIFT - 10),
	       max_mapnr << (PAGE_SHIFT - 10),
	       (_etext - _stext) >> 10,
	       (_edata - _etext) >> 10
	       );
}

static void free_init_pages(const char *what, unsigned long start, unsigned long end)
{
	unsigned long addr;

	for (addr = start; addr < end; addr += PAGE_SIZE) {
		struct page* page = virt_to_page(addr);

		ClearPageReserved(page);
		init_page_count(page);
		__free_page(page);
		totalram_pages++;
	}

	printk("Freeing %s mem: %ldk freed\n", what, (end-start) >> 10);
}

void free_initmem(void)
{
	free_init_pages("unused kernel", (unsigned long)&__init_begin, (unsigned long)&__init_end);
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init free_initrd_mem(unsigned long start, unsigned long end) {
	free_init_pages("initrd", start, end);
}
#endif
