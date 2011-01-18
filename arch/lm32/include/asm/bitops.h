#ifndef _ASM_BITOPS_H_
#define _ASM_BITOPS_H_

#ifdef	__KERNEL__
#undef BIT
#undef BIT_MASK
#undef BIT_WORD
#undef BITS_TO_LONGS
#define BIT(nr)			(1UL << ((unsigned int)(nr)))
#define BIT_MASK(nr)		(1UL << (((unsigned int)(nr)) % BITS_PER_LONG))
#define BIT_WORD(nr)		(((unsigned int)(nr)) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(((unsigned int)(nr)), BITS_PER_BYTE * sizeof(long))
#endif

#include <asm-generic/bitops.h>


#endif
