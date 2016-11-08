/*
 * Procedures for creating, accessing and interpreting the device tree.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/memblock.h>
#include <linux/libfdt.h>

#include <asm/page.h>

void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
	if (!memory_end) {
		memory_start = base;
		memory_end = base + size;
	} else {
		printk(KERN_CRIT "Only one memory bank is supported. "
			"Ignoring memory at 0x%08llx\n", base);
	}
}

void * __init early_init_dt_alloc_memory_arch(u64 size, u64 align)
{
	return __alloc_bootmem(size, align, 0);
}

#ifdef CONFIG_BLK_DEV_INITRD
void __init early_init_dt_setup_initrd_arch(unsigned long start,
		unsigned long end)
{
	initrd_start = (unsigned long)__va(start);
	initrd_end = (unsigned long)__va(end);
	initrd_below_start_ok = 1;
}
#endif

void __init early_init_devtree(void *params)
{
	/* Setup flat device-tree pointer */
	initial_boot_params = params;

	/* Retrieve various informations from the /chosen node of the
	 * device-tree, including the platform type, initrd location and
	 * size, and more ...
	 */
	of_scan_flat_dt(early_init_dt_scan_chosen, cmd_line);

	/* Scan memory nodes */
	of_scan_flat_dt(early_init_dt_scan_root, NULL);
	of_scan_flat_dt(early_init_dt_scan_memory, NULL);
}

void __init device_tree_init(void)
{
	unsigned long base, size;
	struct device_node *cpu;
	int ret;

	if (!initial_boot_params)
		return;

	base = __pa(initial_boot_params);
	size = fdt_totalsize(initial_boot_params);

	/* Before we do anything, lets reserve the dt blob */
	memblock_reserve(base, size);

	unflatten_device_tree();

	/* free the space reserved for the dt blob */
	memblock_free(base, size);

	cpu = of_find_compatible_node(NULL, NULL, "lattice,mico32");
	if (!cpu)
		panic("No compatible CPU found in device tree\n");

	ret = of_property_read_u32(cpu, "clock-frequency", &cpu_frequency);
	if (ret)
		cpu_frequency = (unsigned long)CONFIG_CPU_CLOCK;

	of_node_put(cpu);
}
