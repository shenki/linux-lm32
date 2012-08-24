#ifndef _ASM_LM32_CACHEFLUSH_H
#define _ASM_LM32_CACHEFLUSH_H

#include <asm-generic/cacheflush.h>

static inline void __flush_dcache_all(void)
{
	asm volatile (
			"wcsr DCC, r0\n"
			"nop\n"
	);
}

static inline void __flush_icache_all(void)
{
	asm volatile(
		"wcsr ICC, r0\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
	);
}

static inline void __flush_cache_all(void)
{
	__flush_dcache_all();
	__flush_icache_all();
}

#undef flush_cache_all
#undef flush_cache_range
#undef flush_dcache_range
#undef flush_icache_range

#define flush_cache_all()			__flush_cache_all()
#define flush_cache_range(vma, start, end)	__flush_cache_all()
#define flush_dcache_range(start,len)		__flush_dcache_all()
#define flush_icache_range(start,len)		__flush_icache_all()

#endif /* _ASM_LM32_CACHEFLUSH_H */
