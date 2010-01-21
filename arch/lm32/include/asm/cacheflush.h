#ifndef _ASM_LM32_CACHEFLUSH_H
#define _ASM_LM32_CACHEFLUSH_H

#include <linux/mm.h>

#define flush_cache_all()			__flush_cache_all()
#define flush_cache_mm(mm)			do { } while (0)
#define flush_cache_dup_mm(mm)			do { } while (0)
#define flush_cache_range(vma, start, end)	__flush_cache_all()
#define flush_cache_page(vma, vmaddr)		do { } while (0)
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 0
#define flush_dcache_range(start,len)		__flush_dcache_all()
#define flush_dcache_page(page)			do { } while (0)
#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)
#define flush_icache_range(start,len)		__flush_icache_all()
#define flush_icache_page(vma,pg)		do { } while (0)
#define flush_icache_user_range(vma,pg,adr,len) do { } while (0)
#define flush_cache_vmap(start, end)		do { } while (0)
#define flush_cache_vunmap(start, end)		do { } while (0)

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)
#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)

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

#endif /* _ASM_LM32_CACHEFLUSH_H */
