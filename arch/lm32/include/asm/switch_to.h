#ifndef __LM32_SYSTEM_H
#define __LM32_SYSTEM_H

#include <linux/linkage.h>
#include <linux/thread_info.h>

extern asmlinkage struct task_struct* _switch_to(struct task_struct *,
	struct thread_info *, struct thread_info *);

#define switch_to(prev,next,last) \
do  { \
	last = _switch_to(prev, task_thread_info(prev), task_thread_info(next)); \
} while (0)

#endif
