#ifndef __LM32_SYSTEM_H
#define __LM32_SYSTEM_H

#include <linux/thread_info.h>
#include <linux/linkage.h>

struct task_struct;
extern asmlinkage struct task_struct* resume(struct task_struct* last, struct task_struct* next);

#define switch_to(prev, next, last)					\
	do {								\
		lm32_current_thread = task_thread_info(next); \
		((last) = resume((prev), (next)));			\
	} while (0)

#endif
