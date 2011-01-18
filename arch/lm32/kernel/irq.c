#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <linux/seq_file.h>
#include <linux/types.h>

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
		set_irq_chip(irq, &lm32_irq_chip);
		set_irq_chip_data(irq, (void *)(1 << irq));
		set_irq_handler(irq, handle_level_irq);
	}
}

int show_interrupts(struct seq_file *p, void *v)
{
	int i = *(loff_t *)v;
	struct irqaction * action;
	unsigned long flags;

	if (i < NR_IRQS)
	{
		raw_spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (action)
		{
			seq_printf(p, "%3d: ", i);
			seq_printf(p, "%10u", kstat_irqs(i));
			seq_printf(p, "%14s ", get_irq_chip(i)->name ? : "-");
			seq_printf(p, " %s", action->name);
			for (action = action->next; action; action = action->next)
				seq_printf(p, ", %s", action->name);
			seq_putc(p, '\n');
		}
		raw_spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	} else if (i == NR_IRQS) {
		seq_printf(p, "\nErrors: %u\n", atomic_read(&irq_err_count));
	}

	return 0;
}

asmlinkage void asm_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	generic_handle_irq(irq);
	irq_exit();

	set_irq_regs(old_regs);
}
