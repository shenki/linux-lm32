#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/hw/milkymist.h>

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

static int __init setup_devices(void) {
	int ret = 0;
	int err;

	unsigned long cap = ioread32be(CSR_CAPABILITIES);

	if (( err = platform_device_register(&milkymistuart_device))) {
		printk(KERN_ERR "could not register 'milkymist_uart'error:%d\n", err);
		ret = err;
	}

	if(cap & CAP_ETHERNET )
		if ((err = platform_device_register(&lm32milkether_device))) {
			printk(KERN_ERR "could not register 'milkymist_ethernet'error:%d\n", err);
			ret = err;
		}

	if (cap & CAP_AC97 )
		if ((err = platform_device_register(&ac97_device))) {
			printk(KERN_ERR "could not register 'milkymist_ac97'error:%d\n", err);
			ret = err;
		}

#ifdef CONFIG_XILINX_SYSACE
	if (cap & CAP_ACEUSB )
		if ((err = platform_device_register(&lm32sysace_device))) {
			printk(KERN_ERR "could not register 'milkymist_sysace'error:%d\n", err);
			ret = err;
		}
#endif

	if (cap & CAP_USB )
		if ((err = platform_device_register(&lm32softusb_device))) {
			printk(KERN_ERR "could not register 'milkymist_softusb'error:%d\n", err);
			ret = err;
		}

	if (cap & CAP_PS2KEYBOARD )
		if ((err = platform_device_register(&lm32milkbd_device))) {
			printk(KERN_ERR "could not register 'milkymist_ps2kbd'error:%d\n", err);
			ret = err;
		}

	if (cap & CAP_PS2MOUSE )
		if ((err = platform_device_register(&lm32milkmouse_device))) {
			printk(KERN_ERR "could not register 'milkymist_ps2mouse'error:%d\n", err);
			ret = err;
		}
	if ((err = platform_device_register(&milkymist_mmc_device))) {
		printk(KERN_ERR "could not register 'milkymist_mmc'error:%d\n", err);
		ret = err;
	}
	return ret;
}
arch_initcall(setup_devices);

/* default console - interface to milkymistuart.c serial + console driver */
struct platform_device* milkymistuart_default_console_device = &milkymistuart_device;

