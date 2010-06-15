#ifndef _ASM_LM32_BITOPS_H
#define _ASM_LM32_BITOPS_H

/*
 *  arch/lm32/include/asm/bitops.h
 *
 *  Copyright 1992, Linus Torvalds.
 *
 *  LM32 version:
 *    Copyright (C) 2010  Takeshi MATSUYA
 */

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <linux/compiler.h>
#include <asm/system.h>
#include <asm/byteorder.h>
#include <asm/types.h>

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void set_bit(int nr, volatile void * addr)
{
	__u32 mask;
	volatile __u32 *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);
	__asm__ __volatile__ (
		"lw %0, (%1+0);		\n\t"
		"or %0, %0, %2;		\n\t"
		"sw (%1+0),%0;		\n\t"
		: "=&r" (tmp)
		: "r" (a), "r" (mask)
		: "memory"
	);
	local_irq_restore(flags);
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void clear_bit(int nr, volatile void * addr)
{
	__u32 mask;
	volatile __u32 *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);

	__asm__ __volatile__ (
		"lw %0, (%1+0);		\n\t"
		"and %0, %0, %2;	\n\t"
		"sw (%1+0),%0;		\n\t"
		: "=&r" (tmp)
		: "r" (a), "r" (~mask)
		: "memory"
	);
	local_irq_restore(flags);
}

#define smp_mb__before_clear_bit()	barrier()
#define smp_mb__after_clear_bit()	barrier()

/**
 * change_bit - Toggle a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * change_bit() is atomic and may not be reordered.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void change_bit(int nr, volatile void * addr)
{
	__u32  mask;
	volatile __u32  *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);
	__asm__ __volatile__ (
		"lw  %0, (%1+0);	\n\t"
		"xor %0, %0, %2;	\n\t"
		"sw (%1+0), %0;		\n\t"
		: "=&r" (tmp)
		: "r" (a), "r" (mask)
		: "memory"
	);
	local_irq_restore(flags);
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_set_bit(int nr, volatile void * addr)
{
	__u32 mask, oldbit;
	volatile __u32 *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);
	__asm__ __volatile__ (
		"lw %0, (%2+0);		\n\t"
		"mv %1, %0;		\n\t"
		"and %0, %0, %3;	\n\t"
		"or %1, %1, %3;		\n\t"
		"sw (%2+0), %1;		\n\t"
		: "=&r" (oldbit), "=&r" (tmp)
		: "r" (a), "r" (mask)
		: "memory"
	);
	local_irq_restore(flags);

	return (oldbit != 0);
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile void * addr)
{
	__u32 mask, oldbit;
	volatile __u32 *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);

	__asm__ __volatile__ (
		"lw %0, (%3+0);		\n\t"
		"mv %1, %0;		\n\t"
		"and %0, %0, %2;	\n\t"
		"not %2, %2;		\n\t"
		"and %1, %1, %2;	\n\t"
		"sw (%3+0),%1;		\n\t"
		: "=&r" (oldbit), "=&r" (tmp), "+r" (mask)
		: "r" (a)
		: "memory"
	);
	local_irq_restore(flags);

	return (oldbit != 0);
}

/**
 * test_and_change_bit - Change a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_change_bit(int nr, volatile void * addr)
{
	__u32 mask, oldbit;
	volatile __u32 *a = addr;
	unsigned long flags;
	unsigned long tmp;

	a += (nr >> 5);
	mask = (1 << (nr & 0x1F));

	local_irq_save(flags);
	__asm__ __volatile__ (
		"lw %0, (%2+0);		\n\t"
		"mv %1, %0;		\n\t"
		"and %0, %0, %3;	\n\t"
		"xor %1, %1, %3;	\n\t"
		"sw (%2+0), %1;		\n\t"
		: "=&r" (oldbit), "=&r" (tmp)
		: "r" (a), "r" (mask)
		: "memory"
	);
	local_irq_restore(flags);

	return (oldbit != 0);
}

#include <asm-generic/bitops/non-atomic.h>
#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/__ffs.h>
#include <asm-generic/bitops/fls.h>
#include <asm-generic/bitops/__fls.h>
#include <asm-generic/bitops/fls64.h>

#ifdef __KERNEL__

#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/find.h>
#include <asm-generic/bitops/ffs.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/lock.h>

#endif /* __KERNEL__ */

#ifdef __KERNEL__

#include <asm-generic/bitops/ext2-non-atomic.h>
#include <asm-generic/bitops/ext2-atomic.h>
#include <asm-generic/bitops/minix.h>

#endif /* __KERNEL__ */

#endif /* _ASM_LM32_BITOPS_H */
