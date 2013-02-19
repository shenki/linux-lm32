/*
 * (C) Copyright 2010 Takeshi Matsuya
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/keyboard.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/hw/milkymist.h>

static const unsigned char input_firmware[] = {
#include "asm/hw/softusb-input.h"
};

#define COMLOC(x)       (*(unsigned char *)(x))
#define COMLOCV(x)      (*(volatile unsigned char *)(x))

#define COMLOC_DEBUG_PRODUCE    COMLOCV(SOFTUSB_DMEM_BASE+0x1000)
#define COMLOC_DEBUG(offset)    COMLOCV(SOFTUSB_DMEM_BASE+0x1001+offset)
#define COMLOC_MEVT_PRODUCE     COMLOCV(SOFTUSB_DMEM_BASE+0x1101)
#define COMLOC_MEVT(offset)     COMLOCV(SOFTUSB_DMEM_BASE+0x1102+offset)
#define COMLOC_KEVT_PRODUCE     COMLOCV(SOFTUSB_DMEM_BASE+0x1142)
#define COMLOC_KEVT(offset)     COMLOCV(SOFTUSB_DMEM_BASE+0x1143+offset)

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Milkymist softusb driver");
MODULE_LICENSE("GPL");

static const unsigned char softusb_keycode[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
	191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
	122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140
};


static int keyboard_consume;

static irqreturn_t softusb_rx(int irq, void *dev_id)
{
	struct input_dev *dev = dev_id;
	unsigned int scancode;
	int	modifiers, modifiers_xor;
	static int previous_modifiers = 0, previous_scancode;

	while(keyboard_consume != COMLOC_KEVT_PRODUCE) {
		modifiers = COMLOC_KEVT(8*keyboard_consume+0);
		modifiers_xor = modifiers ^ previous_modifiers;
		scancode = COMLOC_KEVT(8*keyboard_consume+2);
#ifdef DEBUG
{
int i;
for (i=0;i<8;++i)
printk("%02x", COMLOC_KEVT(8*keyboard_consume+i));
printk(" ");
}
#endif
		keyboard_consume = (keyboard_consume + 1) & 0x07;
		if (modifiers_xor & 0x1) {
			input_report_key(dev, KEY_LEFTCTRL, (modifiers & 0x1) != 0);
		} else if (modifiers_xor & 0x2) {
			input_report_key(dev, KEY_LEFTSHIFT, (modifiers & 0x2) != 0);
		} else if (modifiers_xor & 0x4) {
			input_report_key(dev, KEY_LEFTALT, (modifiers & 0x4) != 0);
		} else if (modifiers_xor & 0x8) {
			input_report_key(dev, KEY_LEFTMETA, (modifiers & 0x8) != 0);
		} else if (modifiers_xor & 0x10) {
			input_report_key(dev, KEY_RIGHTCTRL, (modifiers & 0x10) != 0);
		} else if (modifiers_xor & 0x20) {
			input_report_key(dev, KEY_RIGHTSHIFT, (modifiers & 0x20) != 0);
		} else if (modifiers_xor & 0x40) {
			input_report_key(dev, KEY_RIGHTALT, (modifiers & 0x40) != 0);
		} else if (modifiers_xor & 0x80) {
			input_report_key(dev, KEY_RIGHTCTRL, (modifiers & 0x80) != 0);
		} else {
			if (scancode) {
				input_report_key(dev, softusb_keycode[scancode], 1);
				previous_scancode = scancode;
			} else {
				input_report_key(dev, softusb_keycode[previous_scancode], 0);
				previous_scancode = 0;
			}
		}
		previous_modifiers = modifiers;
	}
	input_sync(dev);

	return IRQ_HANDLED;
}

static int softusb_probe(struct platform_device *pdev)
{
	struct input_dev *dev;
	int i, nwords, err;
	unsigned int *usb_dmem = (unsigned int *)SOFTUSB_DMEM_BASE;
	unsigned int *usb_pmem = (unsigned int *)SOFTUSB_PMEM_BASE;

	if(!(ioread32be(CSR_CAPABILITIES) & CAP_USB)) {
		printk("USB: not supported by SoC, giving up.\n");
		return -ENODEV;
	}

	printk("USB: loading Navre firmware\n");
	iowrite32be(SOFTUSB_CONTROL_RESET, CSR_SOFTUSB_CONTROL);
	for(i=0;i<SOFTUSB_DMEM_SIZE/4;i++)
		usb_dmem[i] = 0;
	for(i=0;i<SOFTUSB_PMEM_SIZE/2;i++)
		usb_pmem[i] = 0;
	nwords = (sizeof(input_firmware)+1)/2;
	for(i=0;i<nwords;i++)
		usb_pmem[i] = ((unsigned int)(input_firmware[2*i]))
			|((unsigned int)(input_firmware[2*i+1]) << 8);
	printk("USB: starting host controller\n");
	iowrite32be(0, CSR_SOFTUSB_CONTROL);

	keyboard_consume = 0;

	dev = input_allocate_device();
	if (!dev) {
		dev_err(&pdev->dev, "Not enough memory for input device\n");
		return -ENOMEM;
	}

	dev->name = pdev->name;
	dev->phys = "softusb/input0";
	dev->id.bustype = 0;
	dev->id.product = 0x0001;
	dev->id.version = 0x0100;
	dev->dev.parent = &pdev->dev;
	dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	dev->keycode = (void *)softusb_keycode;
	dev->keycodesize = sizeof(unsigned char);
	dev->keycodemax = ARRAY_SIZE(softusb_keycode);

	for (i = 0; i < ARRAY_SIZE(softusb_keycode); ++i)
		set_bit(i, dev->keybit);

	if (request_irq(IRQ_USB, softusb_rx, 0, "softusb", dev) != 0) {
		printk(KERN_ERR "softusb.c: Could not allocate keyboard receive IRQ\n");
		return -EBUSY;
	}

	printk(KERN_INFO "milkymist_softusb: softusb at 0x%08x irq %d\n",
		SOFTUSB_PMEM_BASE,
		IRQ_USB);

	err = input_register_device(dev);;

	platform_set_drvdata(pdev, dev);

	return 0;
}

static int softusb_remove(struct platform_device *pdev)
{
	struct input_dev *dev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	free_irq(IRQ_USB, dev);
	input_unregister_device(dev);
	return 0;
}

static const struct of_device_id softusb_of_ids[] = {
	{ .compatible = "milkymist,softusb", },
	{}
};


static struct platform_driver softusb_driver = {
	.probe		= softusb_probe,
	.remove		= softusb_remove,
	.driver		= {
		.name		= "softusb",
		.of_match_table = softusb_of_ids,
	},
};

static int __init softusb_init(void)
{
	return platform_driver_register(&softusb_driver);
}

static void __exit softusb_exit(void)
{
	platform_driver_unregister(&softusb_driver);
}

module_init(softusb_init);
module_exit(softusb_exit);
