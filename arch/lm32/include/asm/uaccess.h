#ifndef _LM32_ASM_UACCESS_H
#define _LM32_ASM_UACCESS_H

#include <asm/page.h>

static inline int __access_ok(unsigned long addr, unsigned long size)
{
	return (addr >= PAGE_OFFSET) &&
		((addr + size) <= memory_end);
}
#define __access_ok __access_ok

#include <asm-generic/uaccess.h>

#endif /* _LM32_ASM_UACCESS_H */
