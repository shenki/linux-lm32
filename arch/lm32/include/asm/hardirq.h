#ifndef _ASM_HARDIRQ_H_
#define _ASM_HARDIRQ_H_

#define ack_bad_irq ack_bad_irq
#include <asm-generic/hardirq.h>

extern atomic_t irq_err_count;
static inline void ack_bad_irq(int irq)
{
	atomic_inc(&irq_err_count);
}

#endif
