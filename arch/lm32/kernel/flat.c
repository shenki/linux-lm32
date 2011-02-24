/*
 * Copyright 2007 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/flat.h>

#define FLAT_LM32_RELOC_TYPE_32_BIT   0
#define FLAT_LM32_RELOC_TYPE_HI16_BIT 1
#define FLAT_LM32_RELOC_TYPE_LO16_BIT 2

unsigned long lm32_get_addr_from_rp(unsigned long *ptr,
		unsigned long relval,
		unsigned long flags,
		unsigned long *persistent)
{
	unsigned short *usptr = (unsigned short *)ptr + 1;
	int type = (relval >> 29) & 7;
	unsigned long val;

	switch (type) {
	case FLAT_LM32_RELOC_TYPE_32_BIT:
		pr_debug("*ptr = %lx", get_unaligned(ptr));
		val = get_unaligned(ptr);
		break;

	case FLAT_LM32_RELOC_TYPE_LO16_BIT:
	case FLAT_LM32_RELOC_TYPE_HI16_BIT:
		pr_debug("*usptr = %x", get_unaligned(usptr));
		val = get_unaligned(usptr) & 0xffff;
		val += *persistent;
		break;

	default:
		pr_debug("BINFMT_FLAT: Unknown relocation type %x\n", type);
		return 0;
	}

	return val;
}
EXPORT_SYMBOL(lm32_get_addr_from_rp);

/*
 * Insert the address ADDR into the symbol reference at RP;
 * RELVAL is the raw relocation-table entry from which RP is derived
 */
void lm32_put_addr_at_rp(unsigned long *ptr, unsigned long addr,
		unsigned long relval)
{
	unsigned short *usptr = (unsigned short *)ptr + 1;
	int type = (relval >> 29) & 7;

	switch (type) {
	case FLAT_LM32_RELOC_TYPE_32_BIT:
		put_unaligned(addr, ptr);
		pr_debug("new ptr =%lx", get_unaligned(ptr));
		break;

	case FLAT_LM32_RELOC_TYPE_LO16_BIT:
		put_unaligned(addr, usptr);
		pr_debug("new value %x at %p", get_unaligned(usptr), usptr);
		break;

	case FLAT_LM32_RELOC_TYPE_HI16_BIT:
		put_unaligned(addr >> 16, usptr);
		pr_debug("new value %x at %p", get_unaligned(usptr), usptr);
		break;

	}
}
EXPORT_SYMBOL(lm32_put_addr_at_rp);
