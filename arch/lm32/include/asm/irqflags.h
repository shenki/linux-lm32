#ifndef _LM32_IRQFLAGS_H
#define _LM32_IRQFLAGS_H

static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile ("rcsr %0, IE\n" : "=r" (flags));
	return flags;
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags = arch_local_save_flags();
	arch_local_irq_disable();
	return flags;
}

static inline void arch_local_irq_disable(void)
{
	unsigned int old_ie, new_ie;
	asm volatile (
		"mvi %0,0xfffffffe\n" \
		"rcsr %1, IE\n" \
		"and %0, %1, %0\n" \
		"wcsr IE, %0\n" \
		"andi %1, %1, 1\n" \
		: "=r"(new_ie), "=r"(old_ie) \
	);
	return;
}

static inline void arch_local_irq_enable(void)
{
	unsigned int ie;
	asm volatile (
		"rcsr %0, IE\n"
		"ori %0, %0, 1\n"
		"wcsr IE, %0\n"
		: "=r"(ie));
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	unsigned int ie;
	asm volatile ( \
		"rcsr %0, IE\n" \
		"andi %0, %0, 0xfffe\n" \
		"andi %1, %1, 0x0001\n" \
		"or %0, %0, %1\n" \
		"wcsr IE, %0\n": \
		 "=&r" (ie): "r" (flags) );
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
