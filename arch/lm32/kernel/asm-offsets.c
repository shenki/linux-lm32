/*
 * This program is used to generate definitions needed by
 * assembly language modules.
 *
 * We use the technique used in the OSF Mach kernel code:
 * generate asm statements containing #defines,
 * compile this file to assembler, and then extract the
 * #defines from the assembly-language output.
 */

#define ASM_OFFSETS_C

#include <linux/kbuild.h>

#include <linux/sched.h>
#include <asm/processor.h>
#include <asm/thread_info.h>

int main(void)
{
	DEFINE(TASK_STATE, offsetof(struct task_struct, state));
	DEFINE(TASK_FLAGS, offsetof(struct task_struct, flags));
	DEFINE(TASK_PTRACE, offsetof(struct task_struct, ptrace));
	DEFINE(TASK_BLOCKED, offsetof(struct task_struct, blocked));
	DEFINE(TASK_THREAD, offsetof(struct task_struct, thread));
	DEFINE(TASK_THREAD_INFO, offsetof(struct task_struct, stack));
	DEFINE(TASK_MM, offsetof(struct task_struct, mm));
	DEFINE(TASK_ACTIVE_MM, offsetof(struct task_struct, active_mm));

	/* These are used with sp as the base address, hence the 4 byte offset */
	DEFINE(PT_R1, offsetof(struct pt_regs, r1) + 4);
	DEFINE(PT_R2, offsetof(struct pt_regs, r2) + 4);
	DEFINE(PT_R3, offsetof(struct pt_regs, r3) + 4);
	DEFINE(PT_R4, offsetof(struct pt_regs, r4) + 4);
	DEFINE(PT_R5, offsetof(struct pt_regs, r5) + 4);
	DEFINE(PT_R6, offsetof(struct pt_regs, r6) + 4);
	DEFINE(PT_R7, offsetof(struct pt_regs, r7) + 4);
	DEFINE(PT_R8, offsetof(struct pt_regs, r8) + 4);
	DEFINE(PT_R9, offsetof(struct pt_regs, r9) + 4);
	DEFINE(PT_R10, offsetof(struct pt_regs, r10) + 4);
	DEFINE(PT_R11, offsetof(struct pt_regs, r11) + 4);
	DEFINE(PT_R12, offsetof(struct pt_regs, r12) + 4);
	DEFINE(PT_R13, offsetof(struct pt_regs, r13) + 4);
	DEFINE(PT_R14, offsetof(struct pt_regs, r14) + 4);
	DEFINE(PT_R15, offsetof(struct pt_regs, r15) + 4);
	DEFINE(PT_R16, offsetof(struct pt_regs, r16) + 4);
	DEFINE(PT_R17, offsetof(struct pt_regs, r17) + 4);
	DEFINE(PT_R18, offsetof(struct pt_regs, r18) + 4);
	DEFINE(PT_R19, offsetof(struct pt_regs, r19) + 4);
	DEFINE(PT_R20, offsetof(struct pt_regs, r20) + 4);
	DEFINE(PT_R21, offsetof(struct pt_regs, r21) + 4);
	DEFINE(PT_R22, offsetof(struct pt_regs, r22) + 4);
	DEFINE(PT_R23, offsetof(struct pt_regs, r23) + 4);
	DEFINE(PT_R24, offsetof(struct pt_regs, r24) + 4);
	DEFINE(PT_R25, offsetof(struct pt_regs, r25) + 4);
	DEFINE(PT_GP, offsetof(struct pt_regs, gp) + 4);
	DEFINE(PT_FP, offsetof(struct pt_regs, fp) + 4);
	DEFINE(PT_SP, offsetof(struct pt_regs, sp) + 4);
	DEFINE(PT_RA, offsetof(struct pt_regs, ra) + 4);
	DEFINE(PT_EA, offsetof(struct pt_regs, ea) + 4);
	DEFINE(PT_BA, offsetof(struct pt_regs, ba) + 4);
	DEFINE(PT_MODE, offsetof(struct pt_regs, pt_mode));
	DEFINE(PT_ORIG_R1, offsetof(struct pt_regs, orig_r1));
	DEFINE(PT_SIZE, sizeof(struct pt_regs));

	DEFINE(TI_TASK, offsetof(struct thread_info, task));
	DEFINE(TI_EXECDOMAIN, offsetof(struct thread_info, exec_domain));
	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	DEFINE(TI_CPU, offsetof(struct thread_info, cpu));
	DEFINE(TI_ADDR_LIMIT, offsetof(struct thread_info, addr_limit));
	DEFINE(_THREAD_SIZE, THREAD_SIZE);

	DEFINE(TI_CC_R11, offsetof(struct thread_info, cpu_context.r11));
	DEFINE(TI_CC_R12, offsetof(struct thread_info, cpu_context.r12));
	DEFINE(TI_CC_R13, offsetof(struct thread_info, cpu_context.r13));
	DEFINE(TI_CC_R14, offsetof(struct thread_info, cpu_context.r14));
	DEFINE(TI_CC_R15, offsetof(struct thread_info, cpu_context.r15));
	DEFINE(TI_CC_R16, offsetof(struct thread_info, cpu_context.r16));
	DEFINE(TI_CC_R17, offsetof(struct thread_info, cpu_context.r17));
	DEFINE(TI_CC_R18, offsetof(struct thread_info, cpu_context.r18));
	DEFINE(TI_CC_R19, offsetof(struct thread_info, cpu_context.r19));
	DEFINE(TI_CC_R20, offsetof(struct thread_info, cpu_context.r20));
	DEFINE(TI_CC_R21, offsetof(struct thread_info, cpu_context.r21));
	DEFINE(TI_CC_R22, offsetof(struct thread_info, cpu_context.r22));
	DEFINE(TI_CC_R23, offsetof(struct thread_info, cpu_context.r23));
	DEFINE(TI_CC_R24, offsetof(struct thread_info, cpu_context.r24));
	DEFINE(TI_CC_R25, offsetof(struct thread_info, cpu_context.r25));
	DEFINE(TI_CC_GP, offsetof(struct thread_info, cpu_context.gp));
	DEFINE(TI_CC_FP, offsetof(struct thread_info, cpu_context.fp));
	DEFINE(TI_CC_SP, offsetof(struct thread_info, cpu_context.sp));
	DEFINE(TI_CC_RA, offsetof(struct thread_info, cpu_context.ra));
	DEFINE(TI_CC_EA, offsetof(struct thread_info, cpu_context.ea));
	DEFINE(TI_CC_BA, offsetof(struct thread_info, cpu_context.ba));

	DEFINE(STATE_CURRENT_THREAD, offsetof(struct lm32_state, current_thread));
	DEFINE(STATE_KERNEL_MODE, offsetof(struct lm32_state, kernel_mode));
	DEFINE(STATE_SAVED_R9, offsetof(struct lm32_state, saved_r9));
	DEFINE(STATE_SAVED_R10, offsetof(struct lm32_state, saved_r10));
	DEFINE(STATE_SAVED_R11, offsetof(struct lm32_state, saved_r11));

	return 0;
}
