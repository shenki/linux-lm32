/*
 *  Copyright (C) 2011, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2011, Michael Walle <michael@walle.cc>
 *  GPIO driver for the Milkymist sysctl
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/module.h>

#define GPIO_IN     0x00
#define GPIO_OUT    0x04
#define GPIO_IRQEN  0x08

#define DRIVER_NAME "milkymist_gpio"

struct milkymist_gpio_chip {
	struct of_mm_gpio_chip mm_gc;
	unsigned int num_gpi;
	unsigned int num_gpo;
};

static int milkymist_gpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct of_mm_gpio_chip *mm_chip = to_of_mm_gpio_chip(gc);
	uint32_t mask = BIT(gpio);

	return ioread32be(mm_chip->regs + GPIO_IN) & mask ? 1 : 0;
}

static void milkymist_gpio_set(struct gpio_chip *gc, unsigned int gpio,
	int value)
{
	struct of_mm_gpio_chip *mm_chip = to_of_mm_gpio_chip(gc);
	struct milkymist_gpio_chip *chip =
	    container_of(mm_chip, struct milkymist_gpio_chip, mm_gc);
	uint32_t mask;
	uint32_t reg;

	gpio -= chip->num_gpi;
	mask = BIT(gpio);

	reg = ioread32be(mm_chip->regs + GPIO_OUT);

	if (value)
		reg |= mask;
	else
		reg &= ~mask;

	iowrite32be(reg, mm_chip->regs + GPIO_OUT);
}

static int milkymist_gpio_direction_input(struct gpio_chip *gc, unsigned gpio)
{
	struct of_mm_gpio_chip *mm_chip = to_of_mm_gpio_chip(gc);
	struct milkymist_gpio_chip *chip =
	    container_of(mm_chip, struct milkymist_gpio_chip, mm_gc);

	if (gpio >= chip->num_gpi)
		return -EINVAL;

	return 0;
}

static int milkymist_gpio_direction_output(struct gpio_chip *gc,
	unsigned gpio, int value)
{
	struct of_mm_gpio_chip *mm_chip = to_of_mm_gpio_chip(gc);
	struct milkymist_gpio_chip *chip =
	    container_of(mm_chip, struct milkymist_gpio_chip, mm_gc);

	if (gpio < chip->num_gpi)
		return -EINVAL;

	milkymist_gpio_set(gc, gpio, value);

	return 0;
}

static int __devinit milkymist_gpio_probe(struct platform_device *ofdev)
{
	struct device_node *np = ofdev->dev.of_node;
	struct milkymist_gpio_chip *chip;
	int ret;
	const unsigned int *num_gpi, *num_gpo;

	num_gpi = of_get_property(np, "num-gpi", NULL);
	if (!num_gpi) {
		dev_err(&ofdev->dev, "no num-gpi property set\n");
		return -ENODEV;
	}

	num_gpo = of_get_property(np, "num-gpo", NULL);
	if (!num_gpi) {
		dev_err(&ofdev->dev, "no num-gpo property set\n");
		return -ENODEV;
	}

	if (*num_gpi == 0 && *num_gpo == 0) {
		dev_err(&ofdev->dev, "invalid number of gpios\n");
		return -ENODEV;
	}

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->mm_gc.gc.direction_input = milkymist_gpio_direction_input;
	chip->mm_gc.gc.direction_output = milkymist_gpio_direction_output;
	chip->mm_gc.gc.get = milkymist_gpio_get;
	chip->mm_gc.gc.set = milkymist_gpio_set;

	chip->mm_gc.gc.ngpio = *num_gpi + *num_gpo;
	chip->num_gpi = *num_gpi;
	chip->num_gpo = *num_gpo;

	ret = of_mm_gpiochip_add(np, &chip->mm_gc);
	if (ret) {
		dev_err(&ofdev->dev, "error in probe function with status %d\n",
		       ret);
		goto err;
	}

	platform_set_drvdata(ofdev, chip);

	dev_info(&ofdev->dev, "registered\n");

	return 0;
err:
	kfree(chip);
	return ret;
}

static int __devexit milkymist_gpio_remove(struct platform_device *ofdev)
{
	struct milkymist_gpio_chip *chip = platform_get_drvdata(ofdev);

	if (gpiochip_remove(&chip->mm_gc.gc))
		dev_err(&ofdev->dev, "unable to remove gpio_chip?\n");
	kfree(chip);
	platform_set_drvdata(ofdev, NULL);

	return 0;
}

static const struct of_device_id milkymist_gpio_match[] = {
	{ .compatible = "milkymist,sysctl", },
	{},
};
MODULE_DEVICE_TABLE(of, milkymist_gpio_match);

static struct platform_driver milkymist_gpio_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = milkymist_gpio_match,
	},
	.probe		= milkymist_gpio_probe,
	.remove		= __devexit_p(milkymist_gpio_remove),
};

static int __init milkymist_gpio_init(void)
{
	return platform_driver_register(&milkymist_gpio_driver);
}

/* Make sure we get initialised before anyone else tries to use us */
subsys_initcall(milkymist_gpio_init);

/* No exit call at the moment as we cannot unregister of gpio chips */

MODULE_AUTHOR("Milkymist Project");
MODULE_DESCRIPTION("Milkymist GPIO driver");
MODULE_LICENSE("GPL");
