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

#include <asm-generic/sections.h>
#include <asm/setup.h>

unsigned long memory_start;
unsigned long memory_end;

void __init bootmem_init(void)
{
	unsigned long bootmap_size;
	unsigned long free_pfn, end_pfn;

	memory_start = CONFIG_KERNEL_RAM_BASE_ADDRESS;
	memory_end = CONFIG_KERNEL_RAM_BASE_ADDRESS + CONFIG_KERNEL_RAM_SIZE;

	if( ((unsigned long)_end < memory_start) || ((unsigned long)_end > memory_end) )
		printk("BUG: your kernel is not located in the ddr sdram");

	init_mm.start_code = (unsigned long)_stext;
	init_mm.end_code = (unsigned long)_etext;
	init_mm.end_data = (unsigned long)_edata;
	init_mm.brk = (unsigned long)_end;

	free_pfn = PFN_UP(__pa((unsigned long)_end));
	end_pfn = PFN_DOWN(__pa(memory_end));

	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	bootmap_size = init_bootmem(free_pfn, end_pfn);

	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(PFN_PHYS(free_pfn), PFN_PHYS(end_pfn - free_pfn));
	reserve_bootmem(PFN_PHYS(free_pfn), bootmap_size, BOOTMEM_DEFAULT);

	/*
	 * reserve initrd boot memory
	 */
#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start) {
		unsigned long reserve_start = initrd_start & PAGE_MASK;
		unsigned long reserve_end = (initrd_end + PAGE_SIZE-1) & PAGE_MASK;
		printk("reserving initrd memory: %lx size %lx\n", reserve_start, reserve_end-reserve_start);
		reserve_bootmem(__pa(reserve_start), reserve_end-reserve_start, BOOTMEM_DEFAULT);
	}
#endif
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

	max_mapnr = num_physpages = max_low_pfn;

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
