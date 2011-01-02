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
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/major.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/seq_file.h>
#include <linux/root_dev.h>
#include <linux/init.h>
#include <linux/linkage.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/thread_info.h>
#include <asm/sections.h>

#ifdef CONFIG_PLAT_MILKYMIST
#include <asm/hw/milkymist.h>
#endif

unsigned int kernel_mode = PT_MODE_KERNEL;

/* this is set first thing as the kernel is started
 * from the arguments to the kernel. */
unsigned long asmlinkage _kernel_arg_cmdline; /* address of the commandline parameters */
unsigned long asmlinkage _kernel_arg_initrd_start;
unsigned long asmlinkage _kernel_arg_initrd_end;

static char __initdata cmd_line[COMMAND_LINE_SIZE];


/* from mm/init.c */
extern void bootmem_init(void);
extern void paging_init(void);

unsigned int cpu_frequency;
//unsigned int sdram_start;
//unsigned int sdram_size;

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
}

void __init setup_arch(char **cmdline_p)
{
	/*
	 * init "current thread structure" pointer
	 */
	lm32_current_thread = (struct thread_info*)&init_thread_union;

	cpu_frequency = (unsigned long)CONFIG_CPU_CLOCK;
	//sdram_start = (unsigned long)CONFIG_MEMORY_START;
	//sdram_size = (unsigned long)CONFIG_MEMORY_SIZE;

	/* Save unparsed command line copy for /proc/cmdline */
	memcpy(boot_command_line, *cmdline_p, COMMAND_LINE_SIZE);
	*cmdline_p = cmd_line;

#ifdef CONFIG_DUMMY_CONSOLE
	conswitchp = &dummy_con;
#endif

#ifdef CONFIG_EARLY_PRINTK
	{
		extern void setup_early_printk(void);

		setup_early_printk();
	}
#endif

	/*
	 * Init boot memory
	 */
	bootmem_init();

	/*
	 * Get kmalloc into gear.
	 */
	paging_init();
}

/*
 *	Get CPU information for use by the procfs.
 */

static int show_cpuinfo(struct seq_file *m, void *v)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq;

    cpu = "lm32";
    mmu = "none";
    fpu = "none";

    clockfreq = (loops_per_jiffy*HZ)*5/4;

    seq_printf(m, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/1000000,(clockfreq/100000)%10,
		   (loops_per_jiffy*HZ)/500000,((loops_per_jiffy*HZ)/5000)%100,
		   (loops_per_jiffy*HZ));

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < NR_CPUS ? ((void *) 0x12345678) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};

#ifdef CONFIG_PLAT_MILKYMIST
static struct resource milkymist_mmc_resources[] = {
	{
		.start = CSR_MEMCARD_CLK2XDIV,
		.end = CSR_MEMCARD_CLK2XDIV + 0xff,
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device milkymist_mmc_device = {
	.name = "milkymist-mmc",
	.id = -1,
	.num_resources = ARRAY_SIZE(milkymist_mmc_resources),
	.resource = milkymist_mmc_resources,
};

static struct resource milkymistuart_resources[] = {
#if 0
	[0] = {
		.start = CSR_UART_RXTX,
		.end = CSR_UART_RXTX+0xfff,
		.flags = IORESOURCE_MEM,
	},
#endif
	[1] = {
		.start = IRQ_UARTRX,
		.end = IRQ_UARTTX,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device milkymistuart_device = {
	.name = "milkymist_uart",
	.id = 0,
	.num_resources = ARRAY_SIZE(milkymistuart_resources),
	.resource = milkymistuart_resources,
};

static struct resource lm32milkether_resources[] = {
	[0] = {
		.start = CSR_MINIMAC_SETUP,
		.end = CSR_MINIMAC_SETUP+0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_ETHRX,
		.end = IRQ_ETHTX,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device lm32milkether_device = {
	.name = "minimac",
	.id = 0,
	.num_resources = ARRAY_SIZE(lm32milkether_resources),
	.resource = lm32milkether_resources,
};

static struct resource ac97_resources[] = {
	[0] = {
		.start = CSR_AC97_CRCTL,
		.end = CSR_AC97_CRCTL+0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_AC97CRREQUEST,
		.end = IRQ_AC97DMAW,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device ac97_device = {
	.name = "milkymist-ac97",
	.id = 0,
	.num_resources = ARRAY_SIZE(ac97_resources),
	.resource = ac97_resources,
};
#endif
#ifdef CONFIG_XILINX_SYSACE
static struct resource lm32sysace_resources[] = {
	[0] = {
		.start = 0x70000000,
		.end = 0x7000ffff,
		.flags = IORESOURCE_MEM,
        },
};

static struct platform_device lm32sysace_device = {
	.name = "xsysace",
	.id = 0,
	.num_resources = ARRAY_SIZE(lm32sysace_resources),
	.resource = lm32sysace_resources,
};
#endif
#ifdef CONFIG_KEYBOARD_SOFTUSB
static struct resource lm32softusb_resources[] = {
	[0] = {
		.start = SOFTUSB_PMEM_BASE,
		.end = SOFTUSB_DMEM_BASE + SOFTUSB_DMEM_SIZE,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_USB,
		.end = IRQ_USB,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device lm32softusb_device = {
	.name = "softusb",
	.id = 0,
	.num_resources = ARRAY_SIZE(lm32softusb_resources),
	.resource = lm32softusb_resources,
};
#endif
#ifdef CONFIG_SERIO_MILKBD
static struct resource lm32milkbd_resources[] = {
	[0] = {
		.start = CSR_PS2_KEYBOARD_DATA,
		.end = CSR_PS2_KEYBOARD_DATA+0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PS2KEYBOARD,
		.end = IRQ_PS2KEYBOARD,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device lm32milkbd_device = {
	.name = "milkbd",
	.id = 0,
	.num_resources = ARRAY_SIZE(lm32milkbd_resources),
	.resource = lm32milkbd_resources,
};
#endif
#ifdef CONFIG_SERIO_MILKMOUSE
static struct resource lm32milkmouse_resources[] = {
	[0] = {
		.start = CSR_PS2_MOUSE_DATA,
		.end = CSR_PS2_MOUSE_DATA+0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PS2MOUSE,
		.end = IRQ_PS2MOUSE,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device lm32milkmouse_device = {
        .name = "milkmouse",
        .id = 0,
        .num_resources = ARRAY_SIZE(lm32milkmouse_resources),
        .resource = lm32milkmouse_resources,
};
#endif

static int __init setup_devices(void) {
	int ret = 0;
	int err;

#ifdef CONFIG_PLAT_MILKYMIST
	int cap;
	cap = ioread32be(CSR_CAPABILITIES);

	if( ( err = platform_device_register(&milkymistuart_device)) ) {
		printk(KERN_ERR "could not register 'milkymist_uart'error:%d\n", err);
		ret = err;
	}

	if( cap & CAP_ETHERNET )
		if( (err = platform_device_register(&lm32milkether_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_ethernet'error:%d\n", err);
			ret = err;
		}

	if( cap & CAP_AC97 )
		if( (err = platform_device_register(&ac97_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_ac97'error:%d\n", err);
			ret = err;
		}
#endif
#ifdef CONFIG_XILINX_SYSACE
	if( cap & CAP_ACEUSB )
		if( (err = platform_device_register(&lm32sysace_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_sysace'error:%d\n", err);
			ret = err;
		}
#endif
#ifdef CONFIG_KEYBOARD_SOFTUSB
	if( cap & CAP_USB )
		if( (err = platform_device_register(&lm32softusb_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_softusb'error:%d\n", err);
			ret = err;
		}
#endif
#ifdef CONFIG_SERIO_MILKBD
	if( cap & CAP_PS2KEYBOARD )
		if( (err = platform_device_register(&lm32milkbd_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_ps2kbd'error:%d\n", err);
			ret = err;
		}
#endif
#ifdef CONFIG_SERIO_MILKMOUSE
	if( cap & CAP_PS2MOUSE )
		if( (err = platform_device_register(&lm32milkmouse_device)) ) {
			printk(KERN_ERR "could not register 'milkymist_ps2mouse'error:%d\n", err);
			ret = err;
		}
#endif
	if( ( err = platform_device_register(&milkymist_mmc_device)) ) {
		printk(KERN_ERR "could not register 'milkymist_mmc'error:%d\n", err);
		ret = err;
	}
	return ret;
}
/* default console - interface to milkymistuart.c serial + console driver */
#ifdef CONFIG_PLAT_MILKYMIST
struct platform_device* milkymistuart_default_console_device = &milkymistuart_device;
#else
struct platform_device* milkymistuart_default_console_device = (struct platform_device *)NULL;
#endif

arch_initcall(setup_devices);
