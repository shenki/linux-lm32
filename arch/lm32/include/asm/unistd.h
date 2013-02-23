#if !defined(_ASM_LM32_UNISTD_H) || defined(__SYSCALL)
#define _ASM_LM32_UNISTD_H

#define __ARCH_WANT_SYSCALL_NO_AT
#define __ARCH_WANT_SYSCALL_NO_FLAGS
#define __ARCH_WANT_SYSCALL_OFF_T
#define __ARCH_WANT_SYSCALL_DEPRECATED
#define __ARCH_WANT_SYS_EXECVE

#include <asm-generic/unistd.h>

#undef __NR_mmap

#define sys_vfork sys_ni_syscall
#define sys_mmap sys_ni_syscall
#define sys_mmap2 sys_mmap_pgoff

#endif /* _ASM_LM32_UNISTD_H */
