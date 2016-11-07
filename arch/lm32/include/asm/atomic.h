#ifndef _ASM_LM32_ATOMIC_H
#define _ASM_LM32_ATOMIC_H

#include <linux/types.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic_set(v, i)	(((v)->counter) = i)

#include <asm/system.h>
#include <linux/kernel.h>

static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	int ret,flags;
	flags = arch_local_irq_save();
	ret = v->counter += i;
	arch_local_irq_restore(flags);
	return ret;
}

#define atomic_add(i, v) atomic_add_return(i, v)
#define atomic_add_negative(a, v)	(atomic_add_return((a), (v)) < 0)

static __inline__ int atomic_sub_return(int i, atomic_t *v)
{
	int ret,flags;
	flags = arch_local_irq_save();
	ret = v->counter -= i;
	arch_local_irq_restore(flags);
	return ret;
}

#define atomic_sub(i, v) atomic_sub_return(i, v)
#define atomic_sub_and_test(i,v) (atomic_sub_return(i, v) == 0)

static __inline__ int atomic_inc_return(atomic_t *v)
{
	int ret,flags;
	flags = arch_local_irq_save();
	v->counter++;
	ret = v->counter;
	arch_local_irq_restore(flags);
	return ret;
}

#define atomic_inc(v) atomic_inc_return(v)

/*
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
#define atomic_inc_and_test(v) (atomic_inc_return(v) == 0)

static __inline__ int atomic_dec_return(atomic_t *v)
{
	int ret,flags;
	flags = arch_local_irq_save();
	--v->counter;
	ret = v->counter;
	arch_local_irq_restore(flags);
	return ret;
}

#define atomic_dec(v) atomic_dec_return(v)

static __inline__ int atomic_dec_and_test(atomic_t *v)
{
	int ret,flags;
	flags = arch_local_irq_save();
	--v->counter;
	ret = v->counter;
	arch_local_irq_restore(flags);
	return ret == 0;
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	int ret;
	unsigned long flags;

	flags = arch_local_irq_save();
	ret = v->counter;
	if (likely(ret == old))
		v->counter = new;
	arch_local_irq_restore(flags);
	return ret;
}

#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	int ret;
	unsigned long flags;

	flags = arch_local_irq_save();
	ret = v->counter;
	if (ret != u)
		v->counter += a;
	arch_local_irq_restore(flags);
	return ret != u;
}
#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

static __inline__ void atomic_clear_mask(unsigned long mask, unsigned long *v)
{
	unsigned long flags;
	flags = arch_local_irq_save();
	*v &= mask;
	arch_local_irq_restore(flags);
}

static __inline__ void atomic_set_mask(unsigned long mask, unsigned long *v)
{
	unsigned long flags;
	flags = arch_local_irq_save();
	*v |= mask;
	arch_local_irq_restore(flags);
}

/* Atomic operations are already serializing */
#define smp_mb__before_atomic_dec()    barrier()
#define smp_mb__after_atomic_dec() barrier()
#define smp_mb__before_atomic_inc()    barrier()
#define smp_mb__after_atomic_inc() barrier()

#include <asm-generic/atomic-long.h>
#endif /* _ASM_LM32_ATOMIC __ */
