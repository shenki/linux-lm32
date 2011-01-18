#ifndef __ASM_GPIO_H__
#define __ASM_GPIO_H__

#include <linux/errno.h>

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep
#define gpio_to_irq		__gpio_to_irq

static inline int irq_to_gpio(unsigned irq) { return -EINVAL; }


#include <asm-generic/gpio.h>

#endif
