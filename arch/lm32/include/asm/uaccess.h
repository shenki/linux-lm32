#ifndef _LM32_ASM_UACCESS_H
#define _LM32_ASM_UACCESS_H

extern unsigned long physical_memory_start;
extern unsigned long physical_memory_end;

static inline int __access_ok(unsigned long addr, unsigned long size)
{
	return (addr >= physical_memory_start) &&
		((addr + size) <= physical_memory_end);
}
#define __access_ok __access_ok

#include <asm-generic/uaccess.h>

#endif /* _LM32_ASM_UACCESS_H */
