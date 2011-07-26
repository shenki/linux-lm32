/*
 *  Copyright (C) 2011, Lars-Peter Clausen <lars@metafoo.de>
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

#include <asm/hw/sysctl.h>

#define MILKYMIST_GPIO_NR_INPUTS 8
#define MILKYMIST_GPIO_NR_OUTPUTS 8

static int milkymist_gpio_get_value(struct gpio_chip *chip, unsigned gpio)
{
	uint32_t mask = BIT(gpio);

	return !!(ioread32be(CSR_GPIO_IN) & mask);
}

static void milkymist_gpio_set_value(struct gpio_chip *chip, unsigned gpio,
	int value)
{
	uint32_t mask;
	uint32_t reg;

	gpio -= MILKYMIST_GPIO_NR_INPUTS;
	mask = BIT(gpio);

	reg = ioread32be(CSR_GPIO_OUT);

	if (value)
		reg |= mask;
	else
		reg &= ~mask;

	iowrite32be(reg, CSR_GPIO_OUT);
}

static int milkymist_gpio_direction_input(struct gpio_chip *chip, unsigned gpio)
{
	if (gpio >= MILKYMIST_GPIO_NR_INPUTS)
		return -EINVAL;
	return 0;
}

static int milkymist_gpio_direction_output(struct gpio_chip *chip,
	unsigned gpio, int value)
{
	if (gpio < MILKYMIST_GPIO_NR_INPUTS)
		return -EINVAL;

	milkymist_gpio_set_value(chip, gpio, value);
	return 0;
}

static struct gpio_chip milkymist_gpio_chip = {
	.direction_input = milkymist_gpio_direction_input,
	.direction_output = milkymist_gpio_direction_output,
	.get = milkymist_gpio_get_value,
	.set = milkymist_gpio_set_value,
	.ngpio = MILKYMIST_GPIO_NR_INPUTS + MILKYMIST_GPIO_NR_OUTPUTS,
};

static int milkymist_gpio_init(void)
{
	gpiochip_add(&milkymist_gpio_chip);
	return 0;
}
arch_initcall(milkymist_gpio_init);
