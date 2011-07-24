#ifndef _ASM_LM32_SETUP_H
#define _ASM_LM32_SETUP_H

#include <asm-generic/setup.h>

#ifndef __ASSEMBLY__
#ifdef __KERNEL__

extern char cmd_line[COMMAND_LINE_SIZE];
extern unsigned int cpu_frequency;

#endif /* __KERNEL__ */
#endif /* !__ASSEMLBLY__ */

#endif /* _ASM_LM32_SETUP_H */
