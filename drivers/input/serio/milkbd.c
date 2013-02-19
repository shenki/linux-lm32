/*
 * (C) Copyright 2009 Takeshi Matsuya
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
#include <linux/serio.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/hw/milkymist.h>

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Milkymist PS/2 keyboard connector driver");
MODULE_LICENSE("GPL");

static int milkbd_write(struct serio *port, unsigned char val)
{
	while(ioread32be(CSR_PS2_KEYBOARD_STATUS)&PS2_BUSY);

	iowrite32be(val, CSR_PS2_KEYBOARD_DATA);

	return 0;
}

static irqreturn_t milkbd_rx(int irq, void *dev_id)
{
	struct serio *port = dev_id;
	unsigned int byte;
	int handled = IRQ_NONE;

	byte = ioread32be(CSR_PS2_KEYBOARD_DATA);
	serio_interrupt(port, byte, 0);
	handled = IRQ_HANDLED;

	return handled;
}

static int milkbd_open(struct serio *port)
{

	if (request_irq(IRQ_PS2KEYBOARD, milkbd_rx, 0, "milkbd", port) != 0) {
		printk(KERN_ERR "milkbd.c: Could not allocate keyboard receive IRQ\n");
		return -EBUSY;
	}

	lm32_irq_unmask(IRQ_PS2KEYBOARD);

	printk(KERN_INFO "milkymist_ps2: Keyboard connector at 0x%08x irq %d\n",
		CSR_PS2_KEYBOARD_DATA,
		IRQ_PS2KEYBOARD);

	return 0;
}

static void milkbd_close(struct serio *port)
{
	free_irq(IRQ_PS2KEYBOARD, port);
}

/*
 * Allocate and initialize serio structure for subsequent registration
 * with serio core.
 */
static int milkbd_probe(struct platform_device *dev)
{
	struct serio *serio;

	serio = kzalloc(sizeof(struct serio), GFP_KERNEL);
	if (!serio)
		return -ENOMEM;

	serio->id.type		= SERIO_8042;
	serio->write		= milkbd_write;
	serio->open		= milkbd_open;
	serio->close		= milkbd_close;
	serio->dev.parent	= &dev->dev;
	strlcpy(serio->name, "Milkymist PS/2 Keyboard connector", sizeof(serio->name));
	strlcpy(serio->phys, "milkymist/ps2keyboard", sizeof(serio->phys));

	platform_set_drvdata(dev, serio);
	serio_register_port(serio);
	return 0;
}

static int milkbd_remove(struct platform_device *dev)
{
	struct serio *serio = platform_get_drvdata(dev);
	serio_unregister_port(serio);
	return 0;
}

static struct platform_driver milkbd_driver = {
	.probe		= milkbd_probe,
	.remove		= milkbd_remove,
	.driver		= {
	.name		= "milkbd",
	},
};

static int __init milkbd_init(void)
{
	return platform_driver_register(&milkbd_driver);
}

static void __exit milkbd_exit(void)
{
	platform_driver_unregister(&milkbd_driver);
}

module_init(milkbd_init);
module_exit(milkbd_exit);
