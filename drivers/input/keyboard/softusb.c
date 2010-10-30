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

static const unsigned char
softusb_keycode[] __devinitconst = {
	0, 0, 0, 0,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,				/* 0 - 9 */
	KEY_G,KEY_H,KEY_I,KEY_J,KEY_L,KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,		/* 10 - 19 */
	KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,KEY_Y,KEY_Z,		/* 20 - 29 */
	KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_0,		/* 30 - 39 */
	KEY_ENTER,KEY_ESC,KEY_BACKSPACE,KEY_TAB,KEY_SPACE,KEY_MINUS,KEY_EQUAL,KEY_LEFTBRACE,KEY_RIGHTBRACE,KEY_BACKSLASH,	/* 40 - 49 */
	0,KEY_SEMICOLON,KEY_APOSTROPHE,KEY_GRAVE,KEY_COMMA,KEY_DOT,KEY_SLASH,0,0,0,		/* 50 - 59 */
	0,0,0,0,0,0,0,0,0,0,		/* 60 - 69 */
	0,0,0,0,0,0,0,0,0,KEY_RIGHT,						/* 70 - 79 */
	KEY_LEFT,KEY_DOWN,KEY_UP,0,0,0,0,0,0,0,		/* 80 - 89 */
	0,0,0,0,0,0,0,0,0,0,		/* 90 - 99 */
	0,0,0,0,0,0,0,0,0,0,		/* 110 - 119 */
	0,0,0,0,0,0,0,0,0,0,		/* 100 - 109 */
	0,0,0,0,0,0,0,0								/* 120 - 127 */
};

static int keyboard_consume;

static irqreturn_t softusb_rx(int irq, void *dev_id)
{
	struct input_dev *dev = dev_id;
	unsigned int scancode;
	int	done = 0;

	while(keyboard_consume != COMLOC_KEVT_PRODUCE) {
		scancode = COMLOC_KEVT(2*keyboard_consume+1);
#ifdef DEBUG
scancode = COMLOC_KEVT(2*keyboard_consume)<<8 | COMLOC_KEVT(2*keyboard_consume+1);
printk("%04x ", scancode);
#endif
		keyboard_consume = (keyboard_consume + 1) & 07;
		input_report_key(dev, softusb_keycode[scancode], 1);
		input_report_key(dev, softusb_keycode[scancode], 0);
		done = 1;
	}
	if (done)
		input_sync(dev);

	return IRQ_HANDLED;
}

static int __devinit softusb_probe(struct platform_device *pdev)
{
	struct input_dev *dev;
	int i, nwords, err;
	unsigned int *usb_dmem = (unsigned int *)SOFTUSB_DMEM_BASE;
	unsigned int *usb_pmem = (unsigned int *)SOFTUSB_PMEM_BASE;

	if(!(in_be32(CSR_CAPABILITIES) & CAP_USB)) {
		printk("USB: not supported by SoC, giving up.\n");
		return -ENODEV;
	}

	printk("USB: loading Navre firmware\n");
	out_be32(CSR_SOFTUSB_CONTROL, SOFTUSB_CONTROL_RESET);
	for(i=0;i<SOFTUSB_DMEM_SIZE/4;i++)
		usb_dmem[i] = 0;
	for(i=0;i<SOFTUSB_PMEM_SIZE/2;i++)
		usb_pmem[i] = 0;
	nwords = (sizeof(input_firmware)+1)/2;
	for(i=0;i<nwords;i++)
		usb_pmem[i] = ((unsigned int)(input_firmware[2*i]))
			|((unsigned int)(input_firmware[2*i+1]) << 8);
	printk("USB: starting host controller\n");
	out_be32(CSR_SOFTUSB_CONTROL, 0);

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

	lm32_irq_unmask(IRQ_USB);

	printk(KERN_INFO "milkymist_softusb: softusb at 0x%08x irq %d\n",
		SOFTUSB_PMEM_BASE,
		IRQ_USB);

	err = input_register_device(dev);;

	platform_set_drvdata(pdev, dev);

	return 0;
}

static int __devexit softusb_remove(struct platform_device *pdev)
{
	struct input_dev *dev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	free_irq(IRQ_USB, dev);
	input_unregister_device(dev);
	return 0;
}

static struct platform_driver softusb_driver = {
	.probe		= softusb_probe,
	.remove		= __devexit_p(softusb_remove),
	.driver		= {
	.name		= "softusb",
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
