#ifndef _LM32_IRQFLAGS_H
#define _LM32_IRQFLAGS_H

static inline void arch_local_irq_disable(void)
{
	unsigned long ie;
	asm volatile (
		"rcsr %0, IE\n"
		"andi %0, %0, 0xfffe\n"
		"wcsr IE, %0\n"
		: "=r"(ie)
	);
}

static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile ("rcsr %0, IE\n" : "=r" (flags));
	return flags;
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;
	asm volatile (
		"rcsr %0, IE\n"
		"wcsr IE, r0\n"
		: "=r"(flags)
	);
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	unsigned long ie;
	asm volatile (
		"rcsr %0, IE\n"
		"ori %0, %0, 1\n"
		"wcsr IE, %0\n"
		: "=r"(ie));
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile ("wcsr IE, %0\n" : : "r"(flags));
}

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return ((flags & 0x1) == 0);
}

static inline int arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

#endif /* _LM32_IRQFLAGS_H */
