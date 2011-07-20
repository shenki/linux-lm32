#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/of_fdt.h>
#include <linux/module.h>

atomic_t irq_err_count;

static inline uint32_t lm32_pic_get_irq_mask(struct irq_data *data)
{
	return (uint32_t)irq_data_get_irq_chip_data(data);
}

static void lm32_pic_irq_mask(struct irq_data *data)
{
	uint32_t mask = lm32_pic_get_irq_mask(data);
	uint32_t im;

	__asm__ __volatile__("rcsr %0, IM\n"
			     "not %0, %0\n"
			     "nor %0, %0, %1\n"
			     "wcsr IM, %0"
			      : "=&r"(im) : "r"(mask));
}

static void lm32_pic_irq_unmask(struct irq_data *data)
{
	uint32_t mask = lm32_pic_get_irq_mask(data);
	uint32_t im;

	__asm__ __volatile__("rcsr %0, IM\n"
			     "or %0, %0, %1\n"
			     "wcsr IM, %0\n"
			     : "=&r"(im) : "r"(mask));
}

static void lm32_pic_irq_ack(struct irq_data *data)
{
	uint32_t mask = lm32_pic_get_irq_mask(data);

	__asm__ __volatile__("wcsr IP, %0" : : "r"(mask));
}

static void lm32_pic_irq_mask_ack(struct irq_data *data)
{
	uint32_t mask = lm32_pic_get_irq_mask(data);
	uint32_t im;

	__asm__ __volatile__("rcsr %0, IM\n"
			     "not %0, %0\n"
			     "nor %0, %0, %1\n"
			     "wcsr IM, %0\n"
			     "wcsr IP, %1\n"
			     : "=&r"(im) : "r"(mask) );
}

static struct irq_chip lm32_irq_chip = {
	.name		= "LM32 PIC",
	.irq_ack	= lm32_pic_irq_ack,
	.irq_mask	= lm32_pic_irq_mask,
	.irq_mask_ack	= lm32_pic_irq_mask_ack,
	.irq_unmask	= lm32_pic_irq_unmask,
};

void __init init_IRQ(void)
{
	unsigned int irq;

	local_irq_disable();
	__asm__ __volatile__("wcsr IM, r0");

	for (irq = 0; irq < NR_IRQS; irq++) {
		irq_set_chip(irq, &lm32_irq_chip);
		irq_set_chip_data(irq, (void *)(1 << irq));
		irq_set_handler(irq, handle_level_irq);
	}
}

int arch_show_interrupts(struct seq_file *p, int prec)
{
	seq_printf(p, "%*s: %10u\n", prec, "ERR", atomic_read(&irq_err_count));
	return 0;
}

/*
 * irq_create_of_mapping - Hook to resolve OF irq specifier into a Linux irq#
 *
 * Currently the mapping mechanism is trivial; simple flat hwirq numbers are
 * mapped 1:1 onto Linux irq numbers.  Cascaded irq controllers are not
 * supported.
 */
unsigned int irq_create_of_mapping(struct device_node *controller,
				   const u32 *intspec, unsigned int intsize)
{
	return intspec[0];
}
EXPORT_SYMBOL_GPL(irq_create_of_mapping);

asmlinkage void asm_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	generic_handle_irq(irq);
	irq_exit();

	set_irq_regs(old_regs);
}
