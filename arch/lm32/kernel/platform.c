/*
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#include <linux/kernel.h>
#include <linux/of_platform.h>

static const struct of_device_id lm32_of_bus_ids[] __initdata = {
	{ .compatible = "simple-bus", },
	{ .compatible = "milkymist,csr-bus", },
	{}
};

static int __init lm32_device_probe(void)
{
	of_platform_populate(NULL, lm32_of_bus_ids, NULL, NULL);
	//of_platform_reset_gpio_probe();

	return 0;
}
arch_initcall(lm32_device_probe);
